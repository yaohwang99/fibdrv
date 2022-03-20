#!/bin/bash
#This script tweak the system setting to minimize the unstable factors while
#analyzing the performance of fibdrv.
#

sudo insmod fibdrv_module.ko
sudo ./client_stat
sudo rmmod fibdrv_module
gnuplot plot_med.gp
gnuplot plot_med_k2u.gp
