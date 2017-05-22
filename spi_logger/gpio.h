/* 
 *  GPIO Utilities
 */

#ifndef GPIO_UTILS_H
#define GPIO_UTILS_H

#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define SYSFS_GPIO_NAME "/sys/class/gpio/gpio"

extern int gpio_export(unsigned int gpio);
extern int gpio_unexport(unsigned int gpio);
extern int gpio_set_dir(unsigned int gpio, unsigned int out_flag);
extern int gpio_set_value(unsigned int gpio, unsigned int value);
extern int gpio_get_value(unsigned int gpio, unsigned int *value);
extern int gpio_set_edge(unsigned int gpio, char *edge);
extern int gpio_fd_open(unsigned int gpio);
extern int gpio_fd_close(int fd);
extern int gpio_wait_for_edge(int fd, int timeout_ms);

#endif

