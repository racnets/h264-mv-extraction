#!/bin/bash

echo "h264 long term data collection" 

path=$1
if [ ! -d "$path" ]; then
	$(mkdir $path)
else
	echo "abort: folder $path already exists"
#	exit
fi

#resolutions 16:9
height=180
width=320

#encoding profile
profile=high

#servo stuff
#speeds=(0x00 0x01 0x02 0x04 0x06 0x08 0x10 0x12 0x14 0x16 0x18 0x20)
speeds=(0x00 0x04 0x10)


while :
do
	for speed in "${!speeds[@]}"
	do
		s=${speeds[$speed]}

		#set servo initial position and speed
		$(i2cset -y 0 0x18 0x32 0x00 w)
		$(i2cset -y 0 0x18 0x39 $s b)
		t=$(date +%H%M%S)
		id="setRes$width""x$height""_speed$s""_$profile""_$t"

		program="raspivid -n -v -vf -hf -pf $profile -w $width -h $height -t 10000 -o - | ./h264-mv-extractor pipe: -o $path/$id.h264 -l $path/$id.dat"
	
		echo "$program"
		eval $program

		# add infos to outputfile
		$(sed -i "1i profile\tfps\twidth\theight\tspeed" $path/$id.dat)
		$(sed -i "2i $pf\t$f\t$w\t$h\t$s" $path/$id.dat)
		$(sed -i '3i # data follows:' $path/$id.dat)
	done

	# stop servo
	$(i2cset -y 0 0x18 0x39 0x00 b)

	echo "wait some seconds"
	$(sleep 30)
done
