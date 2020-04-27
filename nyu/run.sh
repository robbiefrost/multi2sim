#!/bin/bash

screen -d -m ./barnes.sh
screen -d -m ./fft.sh
screen -d -m ./lu-cont.sh
screen -d -m ./radix.sh
screen -ls

