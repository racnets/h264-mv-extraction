#!/bin/bash

echo "h264 test data collection" 

path=$1
if [ ! -d "$path" ]; then
	$(mkdir $path)
else
	echo "abort: folder $path already exists"
	exit
fi

#resolutions 16:9
#widths=(320 640 960 1280 1600 1920)
#heights=(180 360 540 720 900 1080)
heights=(320)
widths=(180)

#framerate
#framerates=(30 60)
framerates=(30)

#encoding profile
#profiles=(baseline main high)
profiles=(high)

#servo stuff
#speeds=(0x00 0x01 0x02 0x04 0x06 0x08 0x10 0x12 0x14 0x16 0x18 0x20)
speeds=(0x00 0x10)
minPos=$(i2cget -y 0 0x18 0x34 w)

for res in "${!widths[@]}"
do
	w=${widths[$res]}
	h=${heights[$res]}
	
	for profile in "${!profiles[@]}"
	do
		pf=${profiles[$profile]}
		
		for framerate in "${!framerates[@]}"
		do
			f=${framerates[$framerate]}

			for speed in "${!speeds[@]}"
			do
		
				s=${speeds[$speed]}
				
				#set servo initial position and speed
				#$(i2cset -y 0 0x18 0x02 $minPos w)
				$(i2cset -y 0 0x18 0x32 0x00 w)
				$(i2cset -y 0 0x18 0x39 $s b)
				
				id="setRes$w""x$h""_speed$s""_$pf""_$f""fps"
			
				program="raspivid -n -v -vf -hf -pf $pf -fps $f -w $w -h $h -t 10000 -o - | ./h264-mv-extractor pipe: -o $path/$id.h264 -l $path/$id.dat"
				
				echo "$program"
				eval $program

				# add infos to outputfile
				$(sed -i "1i profile\tfps\twidth\theight\tspeed" $path/$id.dat)
				$(sed -i "2i $pf\t$f\t$w\t$h\t$s" $path/$id.dat)
				$(sed -i '3i # data follows:' $path/$id.dat)
			done

		done
	done
done

# stop servo
$(i2cset -y 0 0x18 0x39 0x00 b)

