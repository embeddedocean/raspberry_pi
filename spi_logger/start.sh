#!/bin/bash
#
# Rockhopper startup script.
# This script will run automatically on boot.
#

echo " EOS ROCKHOPPER V1.0"

#echo -n "Hit ENTER to stop data collection > "
#if read -t 5 response; then
#    echo "Exiting to console prompt."
#else
#fi

#echo -n "Hit ENTER to stop system shutdown > "
#if read -t 5 response; then
#    echo "Exiting to console prompt."
#else
#fi

echo " Starting Data Collection."
sudo /bin/mount /dev/sda1 /mnt
sudo cp /mnt/spi_logger /home/pi
#sudo /home/pi/spi_logger >> /home/pi/log.txt 
sudo /home/pi/spi_logger
echo "Shutting down system."
sudo /bin/umount /mnt
#sudo shutdown now

exit 0

