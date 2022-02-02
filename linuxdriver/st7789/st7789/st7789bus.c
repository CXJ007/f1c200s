#include "st7789bus.h"

/*中景园复制粘贴修修改改*/

int st7789_gpio_request(struct st7789_dev *dev)
{
	int ret=1;
	int i;
	char name[10];
	dev->node = of_find_node_by_path("/st7789");
	if(dev->node == NULL){
		printk("tree_node is not find\r\n");
		return -EINVAL;
	}

	dev->res = of_get_named_gpio(dev->node, "res-gpios", 0);
	if(dev->res < 0){
		printk("can not get res-gpios\r\n");
	}
	dev->cs = of_get_named_gpio(dev->node, "cs-gpios", 0);
	if(dev->cs < 0){
		printk("can not get cs-gpios\r\n");
		ret = -EINVAL;
	}
	dev->dc = of_get_named_gpio(dev->node, "dc-gpios", 0);
	if(dev->dc < 0){
		printk("can not get dc-gpios\r\n");
		ret = -EINVAL;
	}
	dev->wr = of_get_named_gpio(dev->node, "wr-gpios", 0);
	if(dev->wr < 0){
		printk("can not get wr-gpios\r\n");
		ret = -EINVAL;
	}
	dev->rd = of_get_named_gpio(dev->node, "rd-gpios", 0);
	if(dev->rd < 0){
		printk("can not get rd-gpios\r\n");
		ret = -EINVAL;
	}
	for(i=0;i<8;i++){
		dev->db[i] = of_get_named_gpio(dev->node, "db-gpios", i);
		if(dev->db[i] < 0){
			printk("can not get db[%d]-gpios\r\n",i);
			ret = -EINVAL;
		}
	}
	if(ret != -EINVAL){
		printk("res:%d cs:%d dc:%d wr:%d rd:%d\r\n", dev->res, dev->cs, dev->dc, dev->wr, dev->rd);
		for(i=0;i<8;i++){
			printk("db[%d]:%d", i, dev->db[i]);
		}
	}

	gpio_request(dev->res, "res");
	gpio_direction_output(dev->res, 0);
	gpio_request(dev->cs, "cs");
	gpio_direction_output(dev->cs, 0);
	gpio_request(dev->dc, "dc");
	gpio_direction_output(dev->dc, 0);
	gpio_request(dev->wr, "wr");
	gpio_direction_output(dev->wr, 0);
	gpio_request(dev->rd, "rd");
	gpio_direction_output(dev->rd, 0);

	for(i=0;i<8;i++){
		sprintf(name,"db%d",i);
		gpio_request(dev->db[i], name);
		gpio_direction_output(dev->db[i], 0);
	}

	return ret;
}

void data_out(struct st7789_dev dev, uint8_t dat)
{
    int i;
    for(i=0;i<8;i++){
        gpio_set_value(dev.db[i],dat&0x01);
        dat = dat>>1;
    }
}

/******************************************************************************
      函数说明：LCD 8位并口数据写入函数
      入口数据：dat  要写入的并行数据
      返回值：  无
******************************************************************************/
void LCD_Writ_Bus(uint8_t dat) 
{	
	//LCD_CS_CLR;
    LCD_RD_SET;
	LCD_WR_CLR;
	data_out(st7789dev, dat);
	LCD_WR_SET;
	//LCD_CS_SET;
}


/******************************************************************************
      函数说明：LCD写入数据
      入口数据：dat 写入的数据
      返回值：  无
******************************************************************************/
void LCD_WR_DATA8(uint8_t dat)
{
	LCD_DC_SET;//写数据
	LCD_Writ_Bus(dat);
}


/******************************************************************************
      函数说明：LCD写入数据
      入口数据：dat 写入的数据
      返回值：  无
******************************************************************************/
void LCD_WR_DATA(uint16_t dat)
{
	LCD_DC_SET;//写数据
	LCD_Writ_Bus(dat>>8);
	LCD_Writ_Bus(dat);
}


/******************************************************************************
      函数说明：LCD写入命令
      入口数据：dat 写入的命令
      返回值：  无
******************************************************************************/
void LCD_WR_REG(uint8_t dat)
{
	LCD_DC_CLR;//写命令
	LCD_Writ_Bus(dat);
}

/******************************************************************************
      函数说明：设置起始和结束地址
      入口数据：x1,x2 设置列的起始和结束地址
                y1,y2 设置行的起始和结束地址
      返回值：  无
******************************************************************************/
void LCD_Address_Set(u16 x1,u16 y1,u16 x2,u16 y2)
{
	LCD_WR_REG(0x2a);//列地址设置
	LCD_WR_DATA(x1);
	LCD_WR_DATA(x2);
	LCD_WR_REG(0x2b);//行地址设置
	LCD_WR_DATA(y1);
	LCD_WR_DATA(y2);
	LCD_WR_REG(0x2c);//储存器写
}



// int Lcd_Test(struct st7789_dev dev)
// {
//     int ret;

//     LCD_WR_REG(0x04); //read id
//     /*移植中景园发现画屏
//     增加时序延迟不行画屏
//     怀疑时序出错但是这个是移植大概率不会有问题
//     阅读datasheet的8080时序发现好像没啥问题画屏就算有反应
//     准备读id看时序发现原来LCD_Writ_Bus里没有禁止读LCD_RD_SET;
//     加上就行了
//     */

//     return ret;
// }

/******************************************************************************
      函数说明：LCD初始化函数
      入口数据：无
      返回值：  无
******************************************************************************/
void Lcd_Init(void)
{
	LCD_RST_CLR;
	mdelay(200);
	LCD_RST_SET;
	mdelay(200);
	
//************* Start Initial Sequence **********// 

    LCD_CS_CLR;

    LCD_WR_REG(0x01);
    mdelay(200);

    LCD_WR_REG(0x36); 
    LCD_WR_DATA8(0x00);

    LCD_WR_REG(0x3A); 
    LCD_WR_DATA8(0x05);

    LCD_WR_REG(0xB2);
    LCD_WR_DATA8(0x0C);
    LCD_WR_DATA8(0x0C);
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0x33);
    LCD_WR_DATA8(0x33); 

    LCD_WR_REG(0xB7); 
    LCD_WR_DATA8(0x35);  

    LCD_WR_REG(0xBB);
    LCD_WR_DATA8(0x19);

    LCD_WR_REG(0xC0);
    LCD_WR_DATA8(0x2C);

    LCD_WR_REG(0xC2);
    LCD_WR_DATA8(0x01);

    LCD_WR_REG(0xC3);
    LCD_WR_DATA8(0x12);   

    LCD_WR_REG(0xC4);
    LCD_WR_DATA8(0x20);  

    LCD_WR_REG(0xC6); 
    LCD_WR_DATA8(0x0F);    

    LCD_WR_REG(0xD0); 
    LCD_WR_DATA8(0xA4);
    LCD_WR_DATA8(0xA1);

    LCD_WR_REG(0xE0);
    LCD_WR_DATA8(0xD0);
    LCD_WR_DATA8(0x04);
    LCD_WR_DATA8(0x0D);
    LCD_WR_DATA8(0x11);
    LCD_WR_DATA8(0x13);
    LCD_WR_DATA8(0x2B);
    LCD_WR_DATA8(0x3F);
    LCD_WR_DATA8(0x54);
    LCD_WR_DATA8(0x4C);
    LCD_WR_DATA8(0x18);
    LCD_WR_DATA8(0x0D);
    LCD_WR_DATA8(0x0B);
    LCD_WR_DATA8(0x1F);
    LCD_WR_DATA8(0x23);

    LCD_WR_REG(0xE1);
    LCD_WR_DATA8(0xD0);
    LCD_WR_DATA8(0x04);
    LCD_WR_DATA8(0x0C);
    LCD_WR_DATA8(0x11);
    LCD_WR_DATA8(0x13);
    LCD_WR_DATA8(0x2C);
    LCD_WR_DATA8(0x3F);
    LCD_WR_DATA8(0x44);
    LCD_WR_DATA8(0x51);
    LCD_WR_DATA8(0x2F);
    LCD_WR_DATA8(0x1F);
    LCD_WR_DATA8(0x1F);
    LCD_WR_DATA8(0x20);
    LCD_WR_DATA8(0x23);

    LCD_WR_REG(0x21); 

    LCD_WR_REG(0x11); 
    //Delay (120); 

    LCD_WR_REG(0x29); 
} 


/******************************************************************************
      函数说明：LCD清屏函数
      入口数据：无
      返回值：  无
******************************************************************************/
void LCD_Clear(uint16_t Color)
{
	u16 i,j;  	
	LCD_Address_Set(0,0,LCD_W-1,LCD_H-1);
    for(i=0;i<LCD_W;i++)
	 {
	  for (j=0;j<LCD_H;j++)
	   	{
        	LCD_WR_DATA(Color);	 			 
	    }
	  }
}




MODULE_LICENSE("GPL");
MODULE_AUTHOR("CXJ");
