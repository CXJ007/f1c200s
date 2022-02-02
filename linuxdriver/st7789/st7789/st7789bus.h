#ifndef ST7789BUS_H
#define ST7789BUS_H

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>

struct st7789_dev {
	dev_t devid;
	int major;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct device_node *node;

	int res;
	int cs;
	int dc;
	int wr;
	int rd;
	int db[8];
};

extern struct st7789_dev st7789dev;

#define	LCD_RST_SET	 gpio_set_value(st7789dev.res, 1)
#define	LCD_CS_SET   gpio_set_value(st7789dev.cs, 1)
#define	LCD_DC_SET	 gpio_set_value(st7789dev.dc, 1)
#define	LCD_WR_SET	 gpio_set_value(st7789dev.wr, 1)
#define	LCD_RD_SET	 gpio_set_value(st7789dev.rd, 1)

#define	LCD_RST_CLR  gpio_set_value(st7789dev.res, 0)
#define	LCD_CS_CLR   gpio_set_value(st7789dev.cs, 0)
#define	LCD_DC_CLR	 gpio_set_value(st7789dev.dc, 0)
#define	LCD_WR_CLR	 gpio_set_value(st7789dev.wr, 0)
#define	LCD_RD_CLR	 gpio_set_value(st7789dev.rd, 0)

#define LCD_W 240
#define LCD_H 240

int st7789_gpio_request(struct st7789_dev *dev);
void Lcd_Init(void);
void LCD_Clear(uint16_t Color);


#endif