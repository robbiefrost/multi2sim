/*
 *  Multi2Sim
 *  Copyright (C) 2012  Rafael Ubal (ubal@ece.neu.edu)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Cache.h"
#include "System.h"


namespace mem
{

const misc::StringMap Cache::ReplacementPolicyMap =
{
	{ "LRU", ReplacementLRU },
	{ "FIFO", ReplacementFIFO },
	{ "Random", ReplacementRandom },
	{ "LRU-p", ReplacementLRUPartition},
	{ "FLRU", ReplacementFLRU},
	{ "FLRU-p", ReplacementFLRUPartition},
};


const misc::StringMap Cache::WritePolicyMap =
{
	{ "WriteBack", WriteBack },
	{ "WriteThrough", WriteThrough }
};


const misc::StringMap Cache::BlockStateMap =
{
	{ "N", BlockNonCoherent },
	{ "M", BlockModified },
	{ "O", BlockOwned },
	{ "E", BlockExclusive },
	{ "S", BlockShared },
	{ "I", BlockInvalid }
};


Cache::Cache(const std::string &name,
		unsigned num_sets,
		unsigned num_ways,
		unsigned block_size,
		ReplacementPolicy replacement_policy,
		WritePolicy write_policy)
		:
		name(name),
		num_sets(num_sets),
		num_ways(num_ways),
		block_size(block_size),
		replacement_policy(replacement_policy),
		write_policy(write_policy)
{
	// Derived fields
	assert(!(num_sets & (num_sets - 1)));
	assert(!(num_ways & (num_ways - 1)));
	assert(!(block_size & (block_size - 1)));
	num_blocks = num_sets * num_ways;
	log_block_size = misc::LogBase2(block_size);
	block_mask = block_size - 1;

	// Allocate blocks and sets
	blocks = misc::new_unique_array<Block>(num_blocks);
	sets = misc::new_unique_array<Set>(num_sets);
	
	// Initialize sets and blocks
	for (unsigned set_id = 0; set_id < num_sets; set_id++)
	{
		Set *set = getSet(set_id);
		for (unsigned way_id = 0; way_id < num_ways; way_id++)
		{
			Block *block = getBlock(set_id, way_id);
			block->way_id = way_id;
			if (replacement_policy != ReplacementLRUPartition)
			{
				set->lru_list.PushBack(block->lru_node);
			}
		}
	}
}

void Cache::DecodeAddress(unsigned address,
		unsigned &set_id,
		unsigned &tag,
		unsigned &block_offset) const
{
	set_id = (address >> log_block_size) % num_sets;
	tag = address & ~block_mask;
	block_offset = address & block_mask;
}


bool Cache::FindBlock(unsigned address,
		unsigned &set_id,
		unsigned &way_id,
		BlockState &state) const
{
	// Get set and tag
	set_id = (address >> log_block_size) % num_sets;
	unsigned tag = address & ~block_mask;

	// Find block
	for (way_id = 0; way_id < num_ways; way_id++)
	{
		Block *block = getBlock(set_id, way_id);
		if (block->tag == tag && block->state != BlockInvalid)
		{
			state = block->state;
			return true;
		}
	}

	// Block not found
	set_id = 0;
	way_id = 0;
	state = BlockInvalid;
	return false;
}


void Cache::setBlock(unsigned set_id,
		unsigned way_id,
		unsigned tag,
		BlockState state,
		int core_id)
{
	// Trace
	System::trace << misc::fmt("mem.set_block cache=\"%s\" "
			"set=%d way=%d tag=0x%x state=\"%s\"\n",
			name.c_str(),
			set_id,
			way_id,
			tag,
			BlockStateMap[state]);

	// Get set and block
	Set *set = getSet(set_id);
	Block *block = getBlock(set_id, way_id);

	// If the block is being brought to the cache now for the first time,
	// update the FIFO list.
	if (replacement_policy == ReplacementFIFO
			&& block->tag != tag)
	{
		set->lru_list.Erase(block->lru_node);
		set->lru_list.PushFront(block->lru_node);
	}

	// Increment counters for LRU with Partitioning
	if (replacement_policy == ReplacementLRUPartition) {
		set->core_access_count[core_id]++;
		set->core_access_count[num_cores]++;
	}

	// Increment frequency counter for FLRU
	if (replacement_policy == ReplacementFLRU){
        block->counter++;
    }

	// Increment frequency counter for FLRU
	if (replacement_policy == ReplacementFLRUPartition){
        block->counter++;
		set->core_access_count[core_id]++;
		set->core_access_count[num_cores]++;
    }

	// Set new values for block
	block->tag = tag;
	block->state = state;
}


void Cache::getBlock(unsigned set_id,
		unsigned way_id,
		unsigned &tag,
		BlockState &state) const
{
	Block *block = getBlock(set_id, way_id);
	tag = block->tag;
	state = block->state;
}


void Cache::AccessBlock(unsigned set_id, unsigned way_id, int core_id)
{
	// Get set and block
	Set *set = getSet(set_id);
	Block *block = getBlock(set_id, way_id);

	// A block is moved to the head of the list for LRU policy. It will also
	// be moved if it is its first access for FIFO policy, i.e., if the
	// state of the block was invalid.
	bool move_to_head = replacement_policy == ReplacementLRU ||
			(replacement_policy == ReplacementFIFO
			&& block->state == BlockInvalid);
	
	// Move to the head of the LRU list
	if (move_to_head)
	{
		set->lru_list.Erase(block->lru_node);
		set->lru_list.PushFront(block->lru_node);
	}
	
	if (replacement_policy == ReplacementLRUPartition) {
		// Incerement core counter
		set->core_access_count[core_id]++;
		set->core_access_count[num_cores]++;

		// Promote
		if (set->way_owner[way_id] == -1) {
			set->lru_list.Erase(block->lru_node);
			set->lru_list.PushFront(block->lru_node);
		// } else {
		} else if (set->core_lru_list[core_id].getSize() > 1) {
			set->core_lru_list[core_id].Erase(block->lru_node);
			set->core_lru_list[core_id].PushFront(block->lru_node);
		}
	}

    if (replacement_policy == ReplacementFLRU){
        // Increment frequency counter
        block->counter++;

        // Remove block from lru list
        set->lru_list.Erase(block->lru_node);

        // Promotion according to current position
        if (FLRUcheckMforBlock(set_id, way_id)){
            // Push to M position
            set->lru_list.Insert(set->lru_list.FLRUgetMIter(m_size), block->lru_node);
        } else {
            // Push to MRU position
            set->lru_list.PushFront(block->lru_node);
        }

    }


	if (replacement_policy == ReplacementFLRUPartition){
        // Increment frequency counter
        block->counter++;
		set->core_access_count[core_id]++;
		set->core_access_count[num_cores]++;

        // Remove block from lru list
        set->lru_list.Erase(block->lru_node);

        // Promotion according to current position
        if (FLRUPartcheckMforBlock(set_id, way_id)){
            // Push to M position
            set->lru_list.Insert(set->lru_list.FLRUgetMIter(m_size), block->lru_node);
        } else {
            // Push to MRU position
            set->lru_list.PushFront(block->lru_node);
        }

    }
}


unsigned Cache::ReplaceBlock(unsigned set_id, int core_id)
{
	// Get the set
	Set *set = getSet(set_id);

	// For LRU and FIFO replacement policies, return the block at the end of
	// the block list in the set.
	if (replacement_policy == ReplacementLRU ||
			replacement_policy == ReplacementFIFO)
	{
		// Get block at the end of LRU list
		Block *block = misc::cast<Block *>(set->lru_list.Back());
		assert(block);

		// Move it to the head to avoid making it a candidate in the
		// next call to getReplacementBlock().
		set->lru_list.Erase(block->lru_node);
		set->lru_list.PushFront(block->lru_node);

		// Return way index of the selected block
		return block->way_id;
	}

	// LRU with cache partitioning
	if (replacement_policy == ReplacementLRUPartition) {
		float access_ratio = (float)set->core_access_count[core_id] / (float)set->core_access_count[num_cores];
		Block *block;
		// Larger share of accesses and ways available to steal from shared
		// What is the right baseline ratio?
		if (set->lru_list.getSize() > 0
				&& access_ratio > 1.1 / (float)num_cores)
		{
			// Get block from the shared LRU list
			block = misc::cast<Block *>(set->lru_list.Back());

			// Erase from the shared list and add to the head of this core's list
			set->lru_list.Erase(block->lru_node);
			set->core_lru_list[core_id].PushFront(block->lru_node);
			
			// update way_owner
			set->way_owner[block->way_id] = core_id;

			// reset counters
			resetAccessCount(set_id, core_id);
		}
		// Smaller share of accesses and ways available to return to shared
		else if (set->core_lru_list[core_id].getSize() > 1
				&& access_ratio < 0.9 / (float)num_cores)
		{
			// Get block from own list
			block = misc::cast<Block *>(set->core_lru_list[core_id].Back());

			// Move to head of own list
			set->core_lru_list[core_id].Erase(block->lru_node);
			set->lru_list.PushFront(block->lru_node);

			//update way_owner
			set->way_owner[block->way_id] = -1;

			// reset counters
			resetAccessCount(set_id, core_id);
		}
		else {
			// Get block from core's list
			block = misc::cast<Block *>(set->core_lru_list[core_id].Back());

			// Move to head of own list (no need to move if we have only one node)
			if (set->core_lru_list[core_id].getSize() > 1) {
				set->core_lru_list[core_id].Erase(block->lru_node);
				set->core_lru_list[core_id].PushFront(block->lru_node);
			}
		}

		return block->way_id;
	}


	// FLRU Replacement Policy
	if (replacement_policy == ReplacementFLRU){

        Block *block = nullptr;
        if (block = misc::cast<Block *>(FLRUcheckForSelf(set_id, core_id))){
            // Check if belonging ways are in M
        } else if (block = misc::cast<Block *>(FLRUcheckForStolen(set_id, core_id))){
            // Check if own ways are stolen in cache and clear record
        } else {
            // Select lowest priority (lowest frequency) block as victim
            block = misc::cast<Block *>(FLRUgetLowFrequency(set_id));
        }

        // Reset frequency counter
        block->counter = 0;

        // Record new owner of block
        set->way_owner[block->way_id] = core_id;

        // Insert node into M position
        set->lru_list.Erase(block->lru_node);
        set->lru_list.Insert(set->lru_list.FLRUgetMIter(m_size),
							 block->lru_node);

        return block->way_id;
    }

	// FLRU with dynamic partitioning Replacement Policy
	if (replacement_policy == ReplacementFLRUPartition){

        Block *block = nullptr;
        if (block = misc::cast<Block *>(FLRUPartcheckForSelf(set_id, core_id))){
            // Check if belonging ways are in M
        } else if (block = misc::cast<Block *>(FLRUPartcheckForLowPriority(set_id, core_id))){
            // Check if own ways are stolen in cache and clear record
        } else {
            // Select lowest priority (lowest frequency) block as victim
            block = misc::cast<Block *>(FLRUPartgetLowFrequency(set_id));
        }

        // Reset frequency counter
		// if (block->counter == 1) {
		// 	//get core_id for original owner
		// 	int original_owner = set->way_owner[block->way_id];
		// 	set->core_access_count[original_owner]--;
		// 	set->core_access_count[num_cores]--;

		// }

		// int original_owner = set->way_owner[block->way_id];
		// resetAccessCount(set_id, core_id);
		// resetAccessCount(set_id, original_owner);
        block->counter = 0;

        // Record new owner of block
        set->way_owner[block->way_id] = core_id;

        // Insert node into M position
        set->lru_list.Erase(block->lru_node);
        set->lru_list.Insert(set->lru_list.FLRUgetMIter(m_size),
							 block->lru_node);

        return block->way_id;
    }

	// Random replacement policy
	assert(replacement_policy == ReplacementRandom);
	return random() % num_ways;
}


void Cache::setNumCores(int num_cores)
{
	this->num_cores = num_cores;
	this->m_size = num_cores;
	// if this is one of the partitioning policies
	if (replacement_policy == ReplacementLRUPartition) {
		for (unsigned set_id = 0; set_id < num_sets; set_id++) {
			Set *set = getSet(set_id);
			set->core_access_count = misc::new_unique_array<unsigned>(num_cores+1);
			// set->core_lru_list = misc::new_unique_array< misc::List<Block> >(num_cores);
			for (int core_id=0; core_id<num_cores; core_id++) {
				// Initialize LRU list for each core
				// misc::List<Block> core_list;
				// set->core_lru_list.push_back(core_list);
				// Initialize counts to 0
				set->core_access_count[core_id] = 0;
			}
			// Initialize overall count to a higher number so ways don't start stealing immediately
			set->core_access_count[num_cores] = num_cores;
			set->way_owner = misc::new_unique_array<int>(num_ways);
			for (unsigned way_id = 0; way_id < num_ways; way_id++) {
				Block *block = getBlock(set_id, way_id);
				block->way_id = way_id;
				if (way_id < num_cores) {
					set->way_owner[way_id] = way_id;
					set->core_lru_list[way_id].PushBack(block->lru_node);
				} else {
					set->way_owner[way_id] = -1;
					set->lru_list.PushBack(block->lru_node);
				}
			}
		}
	}
	
	if (replacement_policy == ReplacementFLRU) {
		for (unsigned set_id = 0; set_id < num_sets; set_id++) {
			Set *set = getSet(set_id);
			set->way_owner = misc::new_unique_array<int>(num_ways);
			for (unsigned way_id = 0; way_id < num_ways; way_id++) {
				set->way_owner[way_id] = way_id % num_cores;
			}
		}
	}

	if (replacement_policy == ReplacementFLRUPartition) {
		for (unsigned set_id = 0; set_id < num_sets; set_id++) {
			Set *set = getSet(set_id);
			set->way_owner = misc::new_unique_array<int>(num_ways);
			set->core_access_count = misc::new_unique_array<unsigned>(num_cores+1);
			for (unsigned way_id = 0; way_id < num_ways; way_id++) {
				if (way_id < num_cores) {
					set->way_owner[way_id] = way_id;
				} else {
					set->way_owner[way_id] = -1;
				}
			}
			for (int core_id=0; core_id<num_cores; core_id++) {
				set->core_access_count[core_id] = 0;
			}
			set->core_access_count[num_cores] = num_cores;
		}
	}
}


void Cache::resetAccessCount(int set_id, int core_id) {
	Set *set = getSet(set_id);
	unsigned new_count = set->core_access_count[num_cores] / num_cores;
	// set->core_access_count[num_cores] -= (set->core_access_count[core_id] - new_count);
	set->core_access_count[core_id] = new_count;
}


}  // namespace mem

