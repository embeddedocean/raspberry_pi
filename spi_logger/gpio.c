/* 
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#include "gpio.h"

#define MAX_BUF 64

/*
* gpio_export
*/
int gpio_export(unsigned int gpio)
{
	int fd, len;
	char buf[MAX_BUF];
 
	fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
	if (fd < 0) {
		perror("gpio/export");
		return fd;
	}
 
	len = snprintf(buf, sizeof(buf), "%d", gpio);
	write(fd, buf, len);
	close(fd);
 
	return 0;
}

/*
 * gpio_unexport
 */
int gpio_unexport(unsigned int gpio)
{
	int fd, len;
	char buf[MAX_BUF];
 
	fd = open(SYSFS_GPIO_DIR "/unexport", O_WRONLY);
	if (fd < 0) {
		perror("gpio/export");
		return fd;
	}
 	len = snprintf(buf, sizeof(buf), "%d", gpio);
	write(fd, buf, len);
	close(fd);
	return 0;
}

/* 
 * gpio_set_dir 
 */
int gpio_set_dir(unsigned int gpio, unsigned int out_flag)
{
	int fd, len;
	char buf[MAX_BUF];
 
	len = snprintf(buf, sizeof(buf), SYSFS_GPIO_NAME  "%d/direction", gpio);
 
	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror(buf);
		return fd;
	}
 
	if (out_flag)
		write(fd, "out", 4);
	else
		write(fd, "in", 3);
 
	close(fd);
	return 0;
}

/*
 * gpio_set_value
 */
int gpio_set_value(unsigned int gpio, unsigned int value)
{
	int fd, len;
	char buf[MAX_BUF];
 
	len = snprintf(buf, sizeof(buf), SYSFS_GPIO_NAME "%d/value", gpio);

	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror(buf);
		return fd;
	}
 
	if (value)
		write(fd, "1", 2);
	else
		write(fd, "0", 2);
 
	close(fd);
	return 0;
}

/*
 * gpio_get_value
 */
int gpio_get_value(unsigned int gpio, unsigned int *value)
{
	int fd, len;
	char buf[MAX_BUF];
	char ch;

	len = snprintf(buf, sizeof(buf), SYSFS_GPIO_NAME "%d/value", gpio);
 
	fd = open(buf, O_RDONLY);
	if (fd < 0) {
		perror(buf);
		return fd;
	}
 
	read(fd, &ch, 1);

	if (ch != '0') {
		*value = 1;
	} else {
		*value = 0;
	}
 
	close(fd);
	return 0;
}


/*
 * gpio_set_edge
 */

int gpio_set_edge(unsigned int gpio, char *edge)
{
	int fd, len;
	char buf[MAX_BUF];

	len = snprintf(buf, sizeof(buf), SYSFS_GPIO_NAME "%d/edge", gpio);
 
	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror(buf);
		return fd;
	}
 
	write(fd, edge, strlen(edge) + 1); 
	close(fd);
	return 0;
}

/*
 * gpio_fd_open
 */

int gpio_fd_open(unsigned int gpio)
{
	int fd, len;
	char buf[MAX_BUF];

	len = snprintf(buf, sizeof(buf), SYSFS_GPIO_NAME "%d/value", gpio);
 
	//fd = open(buf, O_RDONLY | O_NONBLOCK );
	fd = open(buf, O_RDONLY );
	if (fd < 0) {
		perror(buf);
	}
	return fd;
}

/*
 * gpio_fd_close
 */

int gpio_fd_close(int fd)
{
	return close(fd);
}

int gpio_wait_for_edge(int fd, int timeout_ms)
{
   struct pollfd fdset;
   int nfds = 1;
   int rc;
   int rv = 0;

   memset((void*)&fdset, 0, sizeof(fdset));

   fdset.fd = fd;
   fdset.events = POLLIN | POLLPRI;
 
   rc = poll(&fdset, nfds, timeout_ms);
   //rv = rc;

   if (rc > 0) {
        if (fdset.revents & POLLPRI) {
            /* IRQ happened */
            char buf[2];
            //seek(fd, 0, SEEK_SET);
            int n = pread(fd, buf, sizeof(buf), 0);
            if (n > 0) {
                char gpio_val = ((buf[0]==48) || (buf[0]==49)) ? 0 : 1;
                /* falling edge irq should always read value '0' */
                if ((gpio_val == 1) || (n == 1)) {
                    printf("gpio_wait_for_edge: unexpected value: %d\n", buf[0]);
                }
                if (gpio_val == 0) {
		   rv = 1;
		}
            }
        }
   }
   return(rv);

}


