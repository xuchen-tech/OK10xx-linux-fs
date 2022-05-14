#!/bin/bash

current_path=$PWD
work_path=$PWD/flexbuild

cd $work_path && \
. setup.env && \
flex-builder clean && \
flex-builder -a arm64 -m ls1046ardb -S 1040

cp -fr $work_path/build/images/* $current_path/Image_output

cd $current_path
