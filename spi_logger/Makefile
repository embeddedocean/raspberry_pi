#
# Makefile 
#

CC=gcc

KERN = ../../linux-3.16.1

INCS = -I/usr/include/linux 
#INCS = -I$(KERN)/include/

LIBS = 
FLAGS = -Wl,$(INCS) -O2 

OFILES = gpio.o spi.o

%.o: %.c %.h
	$(CC) $(FLAGS) -c $<

spi_logger:  spi_logger.c $(OFILES)
	$(CC) -o spi_logger spi_logger.c $(OFILES)


clean: 
	rm $(OFILES)
	rm spi_logger
	touch *

