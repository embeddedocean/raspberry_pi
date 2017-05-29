//
// spi_logger.c
//

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/statfs.h>
#include <sys/sysinfo.h>
#include <linux/types.h>

#include "spi.h"
#include "gpio.h"

static int verbose;

#define SD_MMC_BLOCK_SIZE 512

#define SPI_DATA_NBLOCKS (6)
#define SPI_DATA_NBYTES (SPI_DATA_NBLOCKS * SD_MMC_BLOCK_SIZE)

#define SPI_HEADER_NBYTES 12
#define SPI_BUFFER_NBYTES (SPI_HEADER_NBYTES + SPI_DATA_NBYTES)

uint8_t spi_buffer[SPI_BUFFER_NBYTES];
uint8_t *spi_buffer_header = spi_buffer;
uint8_t *spi_buffer_data = &spi_buffer[SPI_HEADER_NBYTES];

#define ADC_HEADER_NBYTES (24)

uint8_t flags;
uint8_t settings;
uint8_t bytes_per_samp;
uint8_t nblocks_per_adc_buffer;
uint16_t nsamps;
uint32_t fs;
uint8_t year;
uint8_t month;
uint8_t day;
uint8_t hour;
uint8_t minute;
uint8_t sec;
uint32_t usec;
uint8_t hdr_chksum;
uint8_t dat_chksum;
uint8_t last_buffer;
uint8_t chksum;

uint8_t checksum(uint8_t *data, uint32_t len)
{
    uint32_t nbytes = len;
    uint8_t *buf = data;
    uint8_t sum = 0;
    while(nbytes--) sum += *buf++;
    return(sum);
}

void parse_adc_buffer_header(uint8_t *buf)
{

    flags = buf[1];
    settings = buf[2];

    nblocks_per_adc_buffer = buf[3];
    bytes_per_samp = buf[4];

    nsamps =  (uint16_t)buf[5];
    nsamps += (uint16_t)buf[6] << 8;

    fs =  (uint32_t)buf[7];
    fs += (uint32_t)buf[8] << 8;
    fs += (uint32_t)buf[9] << 16;
    fs += (uint32_t)buf[10] << 24;

    year = buf[11];
    month = buf[12];
    day = buf[13];
    hour = buf[14];
    minute = buf[15];
    sec = buf[16];

    usec =  buf[17];
    usec += buf[18] << 8;
    usec += buf[19] << 16;
    usec += buf[20] << 24;

    dat_chksum = buf[21];
    hdr_chksum = buf[22];

    if(hdr_chksum != checksum(&buf[1], 21)) {
        fprintf(stdout, "Header checksum error: %d\n", hdr_chksum);
    }

}

void print_adc_buffer_header(uint8_t *buf)
{
    int n;

    //for(n = 0; n < 36; n++) fprintf(stdout,"%d ", buf[n]);
    //fprintf(stdout,"\n");

    //printf("nblocks per spi buffer %d\n", nblocks_per_spi_buffer);
    fprintf(stdout,"nblocks per adc buffer %d\n", nblocks_per_adc_buffer);
    fprintf(stdout,"nbits %d\n", bytes_per_samp);
    fprintf(stdout,"nsamps %d\n", nsamps);
    fprintf(stdout,"fs %d\n", fs);
    fprintf(stdout, "time: %d/%d/%d %d:%d:%f\n",
      day, month, year, hour, minute, (float)sec + 0.000001*(float)usec);

    //uint8_t sum = 0;
    //for(n = 0; n < nsamps-2; n++) sum -= (uint8_t)n;
    //fprintf(stdout,"checksum for 0:%d is %d\n", nsamps-1, sum);

    //for(n = 0; n < 36; n++)
    //fprintf(stdout,"%d ", buf[n]);
    //fprintf(stdout,"\n");
    //for(n = SPI_BUFFER_NBYTES-36; n < SPI_BUFFER_NBYTES; n++)
    //fprintf(stdout,"%d ", buf[n]);
    //fprintf(stdout,"\n");

}

int main(int argc, char **argv)
{
    int c;
    int n;
    char device[32] = "/dev/spidev0.0";
    uint32_t baud = 24000000;

    char* mnt_src  = "/dev/sda1";
    char* mnt_trgt = "/mnt";
    char* mnt_type = "vfat";
    unsigned long mnt_flags = 0;
    char* mnt_opts = NULL;


    while ((c = getopt(argc, argv, "b:n:hv?")) != EOF) {
        switch (c) {
        case 'b':
           baud = (uint32_t)atoi(optarg);
           continue;
        case 'v':
           verbose = 1;
           continue;
        case 'h':
           continue;
        }
    }


    // Mount the SSD
    int result = mount(mnt_src, mnt_trgt, mnt_type, mnt_flags, mnt_opts);
    if (result == 0) {
      printf("Filesystem mounted %s\n", mnt_trgt);
    } else {
      printf("Failed to mount %s: %s [%d]\n", mnt_src, strerror(errno), errno);
    }

    // open spi device
    int spi_fd = spi_master_open(device, baud);
    if (spi_fd < 0) {
        perror("open");
    }
    get_spi_config(spi_fd);

    result = spi_receive(spi_fd, spi_buffer, SPI_BUFFER_NBYTES);

    // master on pin
    unsigned int on_gpio = 22;
    gpio_export(on_gpio);
    gpio_set_dir(on_gpio, 1); // output
    int on_fd = gpio_fd_open(on_gpio);
    gpio_set_value(on_gpio, 0);

    // indicate that the PI is ready
    gpio_set_value(on_gpio, 1);

    uint32_t nblocks_sent = 0;
    uint32_t nblocks_per_upload = 0;
    uint32_t count = 0;
    uint32_t nblocks_received = 0;
    uint32_t errors = 0;
    uint32_t adc_buffer_count = 0;

    FILE *out_fp = NULL;
    int nwrt, nrd;

    int go = 1;
    while( go ) {

        result = spi_master_receive(spi_fd, spi_buffer, SPI_BUFFER_NBYTES);
        if( result < 0 ) {
            fprintf(stdout,"spi_receive error: %d\n", result);
        }

        nblocks_received += SPI_DATA_NBLOCKS;

        uint8_t hdr_chksum = spi_buffer_header[3]; //

	nblocks_sent =	(uint32_t)spi_buffer_header[4] << 0;
	nblocks_sent +=	(uint32_t)spi_buffer_header[5] << 8;
	nblocks_sent +=	(uint32_t)spi_buffer_header[6] << 16;
	nblocks_sent +=	(uint32_t)spi_buffer_header[7] << 24;

	nblocks_per_upload =	(uint32_t)spi_buffer_header[8] << 0;
	nblocks_per_upload +=	(uint32_t)spi_buffer_header[9] << 8;
	nblocks_per_upload +=	(uint32_t)spi_buffer_header[10] << 16;
	nblocks_per_upload +=	(uint32_t)spi_buffer_header[11] << 24;

	uint8_t chksum = checksum(spi_buffer_data, SPI_DATA_NBYTES);
        if(chksum != hdr_chksum) {
           fprintf(stdout,"Checksum error\n" );
           errors++;
           continue;
        }
        //fprintf(stdout,"Buffer received: %d sent, %d received, %d total\n", 
        //   nblocks_sent, nblocks_received, nblocks_per_upload);
	
        //fprintf(stdout,"spi_receive: %d\n", nblocks_sent);

        if( out_fp == NULL ) {
            // get timestamg from adc buffer header
            parse_adc_buffer_header(spi_buffer_data);
            print_adc_buffer_header(spi_buffer_data);
            char filename[64];
            sprintf(filename,"/mnt/RH_%02d%02d%02d_%02d%02d%02d.dat",
               year,month,day,hour,minute,sec);
            fprintf(stdout,"Data file: %s\n", filename);
            out_fp = fopen(filename, "w");
        }

        if( out_fp != NULL ) {
            nwrt = fwrite(spi_buffer_data, 1, SPI_DATA_NBYTES, out_fp);
            if(nwrt != SPI_DATA_NBYTES) printf("write error\n");
        }

        count += SPI_DATA_NBLOCKS;

        if( count >= nblocks_per_adc_buffer ) {
            count = 0;
            adc_buffer_count++;
        }

        if( nblocks_sent >= nblocks_per_upload ) {
            go = 0;
        }

    }

    fclose(out_fp);

    fprintf(stdout,"Received: %lu buffers, %d errors\n", nblocks_received, errors);
    fprintf(stdout,"Received: %lu adc buffers\n", adc_buffer_count);

    umount("/mnt");

    spi_master_close(spi_fd);

    fprintf(stdout,"OK to Shutdown\n");
    gpio_set_value(on_gpio, 0);

    gpio_fd_close(on_fd);
    gpio_unexport(on_gpio);

    return 0;
}


