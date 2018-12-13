#!/bin/bash

if [ $# != 1 ]; then
    echo No destination file specified.
    echo USAGE: ./mk_mjpeg_file.sh '<your-motion-jpeg-file-name>'
    echo This tool will generate a simple Motion JPEG file based on all JEPG '(*.jpg)' in the currrent directory.
    exit 1
fi

dst_file=$1
if [ -f $dst_file ]
then
    echo Destinationf file $dst_file exists
    exit 1
else
    touch $dst_file
    if [ x"$?" != x"0" ]
    then
        echo Failed to touch destination file: $dst_file
        exit 1
    fi
fi

nr_frames=0
total_size=0
for file in `ls *.jpg`
do
    nr_frames=`expr $nr_frames + 1`
    size=`stat -c %s $file | tr -d '\n'`
    total_size=`expr $total_size + $size`
done

if [ x$nr_frames == 'x0' ]; then
    echo No any JPEG file in the current directory.
    exit 1
fi

echo total frames: $nr_frames
echo total size: $total_size

nr_frames_hex=`printf '%08x' $nr_frames`
echo -n -e "\x${nr_frames_hex:6:2}" >> $dst_file
echo -n -e "\x${nr_frames_hex:4:2}" >> $dst_file
echo -n -e "\x${nr_frames_hex:2:2}" >> $dst_file
echo -n -e "\x${nr_frames_hex:0:2}" >> $dst_file

frame_rate=20
frame_rate_hex=`printf '%04x' $frame_rate`
echo -n -e "\x${frame_rate_hex:2:2}" >> $dst_file
echo -n -e "\x${frame_rate_hex:0:2}" >> $dst_file

reserved=0
reserved_hex=`printf '%04x' $reserved`
echo -n -e "\x${reserved_hex:2:2}" >> $dst_file
echo -n -e "\x${reserved_hex:0:2}" >> $dst_file

offset_first_frame=12
offset_first_frame_hex=`printf '%08x' $offset_first_frame`
echo -n -e "\x${offset_first_frame_hex:6:2}" >> $dst_file
echo -n -e "\x${offset_first_frame_hex:4:2}" >> $dst_file
echo -n -e "\x${offset_first_frame_hex:2:2}" >> $dst_file
echo -n -e "\x${offset_first_frame_hex:0:2}" >> $dst_file

for file in `ls *.jpg`
do
    size=`stat -c %s $file | tr -d '\n'`
    size_hex=`printf '%08x' $size`
    echo -n -e "\x${size_hex:6:2}" >> $dst_file
    echo -n -e "\x${size_hex:4:2}" >> $dst_file
    echo -n -e "\x${size_hex:2:2}" >> $dst_file
    echo -n -e "\x${size_hex:0:2}" >> $dst_file
    cat $file >> $dst_file
done

