#include "adcreader.h"
#include <QDebug>
#include <QtCore>
#include "gpio-sysfs.h"
#include "gz_clk.h"
#include "bcm2835.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <assert.h>
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define MAX_SAMPLES 65536

ADCreader::ADCreader()
{
  int ret = 0;

  // set up ringbuffer
  samples = new int[MAX_SAMPLES];
  // pointer for incoming data
  pIn = samples;

  // pointer for outgoing data
  pOut = samples;

  // SPI constants
  static const char *device = "/dev/spidev0.0";
  static uint8_t mode = SPI_CPHA | SPI_CPOL;
  static uint8_t bits = 8;
  static int drdy_GPIO = 22;
  
  // open SPI device
  fd = open(device, O_RDWR);
  if (fd < 0)
    pabort("can't open device");
  
  /*
   * spi mode
   */
  ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
  if (ret == -1)
    pabort("can't set spi mode");
  
  ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
  if (ret == -1)
    pabort("can't get spi mode");
  
  /*
   * bits per word
   */
  ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
  if (ret == -1)
    pabort("can't set bits per word");
  
  ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
  if (ret == -1)
    pabort("can't get bits per word");
  
  fprintf(stderr, "spi mode: %d\n", mode);
  fprintf(stderr, "bits per word: %d\n", bits);
 
// bcm2835_init();  
  // enable master clock for the AD
  // divisor results in roughly 4.9MHz
  // this also inits the general purpose IO
//bcm2835_init(); 
  gz_clock_ena(GZ_CLK_5MHz,5);
  
  // enables sysfs entry for the GPIO pin
  gpio_export(drdy_GPIO);
  // set to input
  gpio_set_dir(drdy_GPIO,0);
  // set interrupt detection to falling edge
  gpio_set_edge(drdy_GPIO,"falling");
  // get a file descriptor for the GPIO pin
  sysfs_fd = gpio_fd_open(drdy_GPIO);
  
  // resets the AD7705 so that it expects a write to the communication register
  writeReset(fd);
  
  // tell the AD7705 that the next write will be to the clock register
  writeReg(fd,0x20);
  // write 00001100 : CLOCKDIV=1,CLK=1,expects 4.9152MHz input clock
  writeReg(fd,0x0C);
  
  //write to commuication register, to tell the AD7705 that the next write will be to the clock register
  writeReg(fd,0x20);
  
  //writing to clock register, telling ADC to make update rate 250Hz
  writeReg(fd,0x0E); 

 //write to commucation register, to tell AD7005 that next write will be to setup register
  writeReg(fd,0x10);

 //Set gain of ADC to 128
  writeReg(fd,0x38);
  // tell the AD7705 that the next write will be the setup register
  writeReg(fd,0x10);
  // intiates a self calibration and then after that starts converting
  writeReg(fd,0x40);
}

void ADCreader::writeReset(int fd)
{
      int ret;
      uint8_t tx1[5] = {0xff,0xff,0xff,0xff,0xff};
      uint8_t rx1[5] = {0};
      struct spi_ioc_transfer tr;
      
      memset(&tr,0,sizeof(struct spi_ioc_transfer));
      tr.tx_buf = (unsigned long)tx1;
      tr.rx_buf = (unsigned long)rx1;
      tr.len = sizeof(tx1);
	  tr.bits_per_word = bits;
      
      ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
      if (ret < 1) { 
	 printf("\nerr=%d when trying to reset. \n",ret);
	 pabort("Can't send spi message");
      }
}


void ADCreader::writeReg(int fd, uint8_t v)
{

 int ret;
 uint8_t tx1[1];
 tx1[0] = v;
 uint8_t rx1[1] = {0};
 struct spi_ioc_transfer tr;

  memset(&tr,0,sizeof(struct spi_ioc_transfer));
  tr.tx_buf = (unsigned long)tx1;
  tr.rx_buf = (unsigned long)rx1;
  tr.len = sizeof(tx1);
  tr.bits_per_word = bits;

  ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
  if (ret < 1)
     pabort("can't send spi message");  

}






uint8_t ADCreader::readReg(int fd)
{
	int ret;
	uint8_t tx1[1];
	tx1[0] = 0;
	uint8_t rx1[1] = {0};
	struct spi_ioc_transfer tr;

	memset(&tr,0,sizeof(struct spi_ioc_transfer));
	tr.tx_buf = (unsigned long)tx1;
	tr.rx_buf = (unsigned long)rx1;
	tr.len = sizeof(tx1);
	tr.bits_per_word = 8;

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
	  pabort("can't send spi message");
	  
	  return rx1[0];
	  
}


int ADCreader::readData(int fd)
{
      int ret;
      uint8_t tx1[2] = {0,0};
      uint8_t rx1[2] = {0,0};
      struct spi_ioc_transfer tr;
      
      memset(&tr,0,sizeof(struct spi_ioc_transfer));
      tr.tx_buf = (unsigned long)tx1;
      tr.rx_buf = (unsigned long)rx1;
      tr.len = sizeof(tx1);
	  tr.bits_per_word = 8;
      
      ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
       if (ret < 1)
	{
		printf("\n can't send spi message, ret = %d\n",ret);
		exit(1);

	}

	return (rx1[0]<<8)|(rx1[1]);
}





void ADCreader::run() //this is what triggered when thread is started
{ int no_tty = !isatty( fileno(stdout) );


  fprintf(stderr,"We are running!\n");

	
  while (1) {
    
    // let's wait for data for max one second
    int ret = gpio_poll(sysfs_fd,1000);
  //  printf("value of sysfs_fd is: %d\n", sysfs_fd);
//    printf(stderr,"value of return after polling is = %d       \r",ret);


        if (ret<1) {
           fprintf(stderr,"Poll error %d\n",ret);
    }
//    printf("Value of file descriptor just before writing which causes error is %d\n", fd);
    // tell the AD7705 to read the data register (16 bits)
           writeReg(fd,0x38);
    
    // read the data register by performing two 8 bit reads
     int value = readData(fd)-0x8000;
     fprintf(stderr,"data = %d       \r",value);

        *pIn = value;
      if (pIn == (&samples[MAX_SAMPLES-1])) 
      pIn = samples;
    else
     pIn++;


  if( no_tty)
    {
	     printf("%d\n", value);
	     fflush(stdout);
    }
  }
  
  close(fd);
  gpio_fd_close(sysfs_fd);
}


int ADCreader::getSample()
{
  assert(pOut!=pIn);
  int value = *pOut;
  if (pOut == (&samples[MAX_SAMPLES-1])) 
    pOut = samples;
  else
    pOut++;
  return value;
}


int ADCreader::hasSample()
{
  return (pOut!=pIn);
}
  

void ADCreader::quit()
{
	running = false;
	exit(0);
}

void ADCreader::pabort(const char *s)
{
        perror(s);
        abort();
}
