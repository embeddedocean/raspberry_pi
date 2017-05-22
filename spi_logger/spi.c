//
// spi master with gpio handshaking 
//
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/statfs.h>
#include <sys/sysinfo.h>

#include <linux/types.h>
#include <linux/spi/spidev.h>

#include "gpio.h"


void spi_read(int fd, uint8_t *buf, uint32_t len)
{
    int status;

    status = read(fd, buf, len);
    if (status < 0) {
        perror("read");
        return;
    }
    if (status != len) {
        fprintf(stderr, "short read\n");
        return;
    }
}

int spi_receive(int fd, uint8_t *buf, uint32_t len)
{
    struct spi_ioc_transfer xfer;
    int n, status;

    memset(&xfer, 0, sizeof xfer);

    xfer.rx_buf = (unsigned long)buf;
    xfer.len = len;
    xfer.bits_per_word = 8;

    status = ioctl(fd, SPI_IOC_MESSAGE(1), &xfer);
    if (status < 0) {
        perror("SPI_IOC_MESSAGE");
    }

    return(status);
}


int spi_send(int fd, uint8_t *buf, uint32_t len)
{
    struct spi_ioc_transfer xfer;
    int n, status;

    memset(&xfer, 0, sizeof xfer);

    xfer.tx_buf = (unsigned long)buf;
    xfer.len = len;
    xfer.bits_per_word = 8;

    status = ioctl(fd, SPI_IOC_MESSAGE(1), &xfer);
    if (status < 0) {
       perror("SPI_IOC_MESSAGE");
    }

    return(status);
}

void get_spi_config(int fd)
{
    uint8_t mode, lsb, bits;
    uint32_t speed;

    if (ioctl(fd, SPI_IOC_RD_MODE, &mode) < 0) {
        perror("SPI rd_mode");
        return;
    }
    if (ioctl(fd, SPI_IOC_RD_LSB_FIRST, &lsb) < 0) {
        perror("SPI rd_lsb_fist");
        return;
    }
    if (ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits) < 0) {
        perror("SPI bits_per_word");
        return;
    }
    if (ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed) < 0) {
        perror("SPI max_speed_hz");
        return;
    }

    printf("spi mode 0x%x, %d bits, %s, %u Hz max\n",
       mode, bits, lsb ? "LSB" : "MSB", speed);
}


void spi_config(int fd, uint8_t lsb, uint8_t bits, uint8_t mode, uint32_t speed)
{
    if (ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0) {
        perror("SPI wr_mode");
        return;
    }
    if (ioctl(fd, SPI_IOC_WR_LSB_FIRST, &lsb) < 0) {
        perror("SPI wr_lsb_fist");
        return;
    }
    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) {
        perror("SPI bits_per_word");
        return;
    }
    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
        perror("SPI max_speed_hz");
        return;
    }
}


//---------------------------------------------------------------
// spi master with handshaking gpio
//
// slave ready pin - active high
unsigned int slave_ready_gpio = 24;  // PA24
int slave_ready_fd;

// master ready pin - active high
unsigned int master_ready_gpio = 25;  // PA25
int master_ready_fd;

int spi_master_open(char *device, uint32_t baud)
{
    gpio_export(slave_ready_gpio);
    gpio_set_dir(slave_ready_gpio, 0); // input
    //gpio_set_edge(slave_ready_gpio, "rising");
    slave_ready_fd = gpio_fd_open(slave_ready_gpio);

    gpio_export(master_ready_gpio);
    gpio_set_dir(master_ready_gpio, 1); // output
    master_ready_fd = gpio_fd_open(master_ready_gpio);
    gpio_set_value(master_ready_gpio, 0);

    // open spi device
    int fd = open(device, O_RDWR);
    if (fd < 0) {
        perror("open");
    }

    spi_config(fd, 0, 8, 1, baud);
    
    //get_spi_config(fd);

    return(fd);
}

void spi_master_close(int fp)
{
    gpio_set_value(master_ready_gpio, 0);
    umount("/mnt");

    gpio_fd_close(slave_ready_fd);
    gpio_unexport(slave_ready_gpio);
    gpio_fd_close(master_ready_fd);
    gpio_unexport(master_ready_gpio);
    
    close(fp);
}

int spi_master_receive(int fd, uint8_t *buf, uint32_t len)
{
    int status = 0;

    // indicate to slave that master is ready to receive
    gpio_set_value(master_ready_gpio, 1);

    //rts = gpio_wait_for_edge(slave_ready_fd, 1000); // wait 1 second max
    //if( rts <= 0 ) continue;
    int rts = 0;
    while( rts == 0 ) gpio_get_value(slave_ready_gpio, &rts);

    status = spi_receive(fd, buf, len);
    
    gpio_set_value(master_ready_gpio, 0);

    return(status);
}

int spi_master_send(int fd, uint8_t *buf, uint32_t len)
{
    int status = 0;
    
    // indicate to slave that master is ready to receive
    gpio_set_value(master_ready_gpio, 1);

    //rts = gpio_wait_for_edge(slave_ready_fd, 1000); // wait 1 second max
    //if( rts <= 0 ) continue;
    int rts = 0;
    while( rts == 0 ) gpio_get_value(slave_ready_gpio, &rts);

    status = spi_send(fd, buf, len);
    
    gpio_set_value(master_ready_gpio, 0);
    return(status);
}



