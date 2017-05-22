//
// spi master with gpio handshaking
//
#ifndef _SPI_H
#define _SPI_H

extern void spi_read(int fd, uint8_t *buf, uint32_t len);
extern int spi_receive(int fd, uint8_t *buf, uint32_t len);
extern int spi_send(int fd, uint8_t *buf, uint32_t len);
extern void get_spi_config(int fd);
extern void spi_config(int fd, uint8_t lsb, uint8_t bits, uint8_t mode, uint32_t speed);

extern int spi_master_receive(int fd, uint8_t *buf, uint32_t len);
extern int spi_master_send(int fd, uint8_t *buf, uint32_t len);
extern int spi_master_open(char *device, uint32_t baud);
extern void spi_master_close(int fd);

#endif


