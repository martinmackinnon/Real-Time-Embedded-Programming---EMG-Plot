#ifndef ADCREADER
#define ADCREADER

#include <QThread>
#include <QtCore>
#include <QDebug>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
// sets the clock for the AD converter
#include "gz_clk.h"

// This class reads continously from the AD7705
// and stores the data in a ringbuffer
// which can be read with getSample() and
// checked if data is available with hasSample().
//
// The class needs to be started from the main
// program as a thread:
// adcreader->start();
// which is then running until the function
// adcreader->quit(); is called. 
// Then the standard method
// adcreader->wait(); needs to be called
// so that this thread has time to terminate.
// TODO: make ADC parameters configurable such as gain, sampling rate etc.
class ADCreader : public QThread //taking this class and inheriting QThread,
                                 //Qthread has a bpublic function called run
{
 public:
  ADCreader();
  
  // ring buffer functions
  int hasSample();
  int getSample();

 public:
  // thread functions
  void quit();
  void run();  //when we start thread, this is the function going to be called.
  bool running;

 protected:
  void writeReset(int fd);
  void writeReg(int fd, uint8_t v);
  uint8_t readReg(int fd);
  int readData(int fd);

 private:
  void pabort(const char *s);
  
  // file descriptor on the SPI interface
  int fd;

  // file descriptor on the interrupt pin
  int sysfs_fd;
  
  // data collected
  int *samples;

  // pointer to ringbuffer
  int *pIn;
  int *pOut;

  // spi constants for the ioctrl calls
  uint8_t mode;
  uint8_t bits;
  uint32_t speed;
  int drdy_GPIO;
  ADCreader *adcreader;

};

#endif
