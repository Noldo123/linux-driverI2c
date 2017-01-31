/*
**Información del driver
**date:    Jan 20 2017
**version: 1.0
**brief:  Driver para utilizar la estación meteorologica I2C sparkfun weathershield 
*/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/fs.h>
#include <linux/stddef.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/i2c.h>
#include <asm/atomic.h>
#include <asm/uaccess.h> // copy to user ; copy from user
#define DEVICE_NAME "spark"  // El dispositivo aparecera en /dev/spark/
#define CLASS_NAME "spk"    // Tipo de dispositivo

//Register functions
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Team StationX");
MODULE_DESCRIPTION("Homebrew driver for sparkfun weather station for embedded linux diploma curse");
MODULE_VERSION("0.1");

static int majorNumber;                     // Almacena el número de dispositivo
static char message[256] ={0};              // Memoria para la cadena que pasa del espacio de usuario
static short size_of_message;               // Utilizado para saber el tamaño de la cadena almacenada
static int numberOpens =0;                  // Contador de uso del dispositivo
static struct class* sparkClass = NULL;     // puntero de la clase del dispositivo
static struct device* sparkDevice = NULL;   // puntero a la estructura del dispositivo


/* Funciones prototipo para el driver */
static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *,char *, size_t,loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t*);

//MODULE_DEVICE_TABLE(i2c, spark_idtable);


/*Brief devices*/
static struct file_operations fops =
{
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .release = dev_release,
};


/*Iniciando el driver*/
static int __init spark_init(void)
{
  printk(KERN_INFO "Sparkfun: Initializing the Sparkfun \n");
  // Allocating a major number for the device
  majorNumber = register_chrdev(0,DEVICE_NAME,&fops);
  if (majorNumber<0)
    {
      printk(KERN_ALERT "Sparkfun failed to register a major number\n");
      return majorNumber;
    }
  printk(KERN_INFO "Sparkfun: registered correctly with major number %d\n",majorNumber);
  /*Register the device class*/
  sparkClass = class_create(THIS_MODULE,CLASS_NAME);
  if(IS_ERR(sparkClass)){
    class_destroy(sparkClass);
    unregister_chrdev(majorNumber,DEVICE_NAME);
    printk(KERN_ALERT "Failed to register the device class\n");
    return PTR_ERR(sparkClass);
  }
  printk(KERN_INFO "Sparkfun: device class created correctly\n");

  /*Register the device driver*/
  sparkDevice = device_create(sparkClass,NULL,MKDEV(majorNumber,0),NULL,DEVICE_NAME);
  if(IS_ERR(sparkDevice)){
    class_destroy(sparkClass);
    unregister_chrdev(majorNumber,DEVICE_NAME);
    printk(KERN_ALERT "Failed to create the device\n");
    return PTR_ERR(sparkDevice);
  }
  printk(KERN_INFO "Sparkfun: device class created correctly\n");
  return 0;
}

static void __exit spark_exit(void){
  device_destroy(sparkClass,MKDEV(majorNumber,0)); /*Removing the device*/
  class_unregister(sparkClass);
  class_destroy(sparkClass);
  unregister_chrdev(majorNumber,DEVICE_NAME);
  printk(KERN_INFO "Sparkfun: Removing from kernel\n");
}

static int dev_open(struct inode *inodep, struct file *filep){
  numberOpens++;
  printk(KERN_INFO "Sparkfun: Device has been opened %d time(s)\n");
  return 0;
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
  int error_count = 0;
  error_count = copy_to_user(buffer,message,size_of_message);

  if(error_count=0){
    printk(KERN_INFO "Sparkfun: Sent %d of chars to the user \n",size_of_message);
    return (size_of_message=0);
  }
  else{
    printk(KERN_INFO "Sparkfun: Failed to send info from sensors to the user\n",error_count);
    return -EFAULT;
  }
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
  sprintf(message, "%s(%zu letters)",buffer, len);
  size_of_message = strlen(message);
  printk(KERN_INFO "Sparkfun: Received %zu characters from the sensor\n",len);
  return len;
}

/*Comunicación básica*/

//int i2c_master_send(struct i2c_client *client, const char *buf, int count);
//int i2c_master_recv(struct i2c_client *client, char *buf, int count);

/*
static struct spark_adapter sp_adapter = {
  .owner = THIS_MODULE,
  .class = I2C_ADAP_CLASS_SMBUS
  .algo = &spark_algorithm,
  .name = "sp_adapter",
};

retval = i2c_add_adapter(&sp_adapter);


static struct spark_algorithm spfun_algorithm = {
  .name          = "spfun_algorithm",
  .id            = I2C_ALGO_SMBUS,
  .smbus_xfer    = spfun_access,
  .functionality = spfun_func,
};

static u32 spfun_func(struct spark_adapter *adapter)
{
  return I2C_FUNC_SMBUS_QUICK |
         I2C_FUNC_SMBUS_BYTE |
         I2C_FUNC_SMBUS_BYTE_DATA |
         I2C_FUNC_SMBUS_WORD_DATA |
         I2C_FUNC_SMBUS_BLOCK_DATA;
}

static s32 spfun_access(struct spark_adapter *adap,
			u16 addr,
			unsigned short flags,
			char read_write,
			u8 command,
			int size,
			union i2c_smbus_data *data)
{
  int i, len;
  dev_info(&adap->dev, "%s was called with the following parameters:\n",
	   __FUNCTION__);
  dev_info(&adap->dev, "addr = %.4x\n",addr);
  dev_info(&adap->dev, "flags = %.4x\n",flags);
  dev_info(&adap->dev, "read_write = %s\n",
	   read_write == I2C_SMBUS_WRITE ?
	   "write" : "read");
  dev_info(&adap->dev, "command = %d\n",
	   command);

  switch (size) {
  case I2C_SMBUS_PROC_CALL:
    dev_info(&adap->dev,
	     "size = I2C_SMBUS_PROC_CALL\n");
    break;
  case I2C_SMBUS_QUICK:
    dev_info(&adap->dev,
	     "size = I2C_SMBUS_QUICK");
    break;
  case I2C_SMBUS_BYTE:
    dev_info(&adap->dev,
	     "size = I2C_SMBUS_BYTE");
    break;
  case I2C_SMBUS_WORD_DATA:
    dev_info(&adap->dev,
	     "size = I2C_SMBUS_WORD_DATA\n");
    if(read_write == I2C_SMBUS_WRITE)
      dev_info(&adap->dev,
	       "data = %.4x\n",data->word);
    break;
  case I2C_SMBUS_BLOCK_DATA:
    dev_info(&adap->dev,
	     "size = I2C_SMBUS_BLOCK_DATA\n");
    if(read_write== I2C_SMBUS_WRITE){
      dev_info(&adap->dev,"data = %.4x\n",
	       data->word);
      if (len<0)
	len=0;
      if(len>32)
	len=32;
      for (i =1 ; i <= len; i++)
	dev_info(&adap->dev,
		 "data->block[%d]= %x\n",
		 i,data->block[i]);
    }
    break;
  }
  return 0;
}
*/
static int dev_release(struct inode *inodep,struct file *filep){
  printk(KERN_INFO "Sparkfun: Device successfully closed\n");
  return 0;
}

module_init(spark_init);
module_exit(spark_exit);
