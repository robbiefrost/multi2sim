/*
 *  Multi2Sim
 *  Copyright (C) 2012  Rafael Ubal (ubal@ece.neu.edu)
 *
 *  This module is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This module is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this module; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef RUNTIME_CUDA_STREAM_H
#define RUNTIME_CUDA_STREAM_H

#include <assert.h>
#include "../include/cuda.h"
#include "../include/cuda_runtime_api.h"

#include "mhandle.h"




struct CUstream_st
{
	unsigned int id;
	int ref_count;

	unsigned int device_id;
};

CUstream cuda_stream_create(void);
void cuda_stream_free(CUstream stream);

#endif

