#echo 8192 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages

[ "$gdb" ] && gdb="gdb --args"

sudo -E $gdb ./build/flow -c 0xf00 -n 4 \
	--socket-mem=2048,2048 \
	--file-prefix pf \
	-w 0000:81:00.0 \
	-- -i --txq=4 --rxq=4
