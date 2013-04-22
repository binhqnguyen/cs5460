sudo rmmod minix
sudo insmod minix.ko
sudo mkfs.minix -v /dev/ram1
sudo mount /dev/ram1 /mnt/minix
