#!/bin/bash
#This script tweak the system setting to minimize the unstable factors while
#analyzing the performance of fibdrv.
#

sudo insmod fibdrv.ko
#sudo taskset -c 7 ./client_stat
sudo ./client_stat
sudo rmmod fibdrv
gnuplot plot.gp

