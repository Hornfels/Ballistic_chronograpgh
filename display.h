/*
 *	LCD interface header file
 *	See lcd.c for more info
 */
 
/* write a byte to the LCD in 4 bit mode */
 
extern void lcd_write(unsigned char);
 
/* Clear and home the LCD */
 
extern void lcd_clear(void);
 
/* write a string of characters to the LCD */
 
extern void lcd_puts(const char * s);
 
/* Go to the specified position (second line starts at 40) */
 
extern void lcd_goto(unsigned char pos);
 
/* intialize the LCD - call before anything else */
 
extern void lcd_init(void);
 
/* intialize the LCD - 3.3V version */
 
extern void lcd_init33(void);
 
/* print a character  */
 
extern void lcd_putchar(char s);
 
/* print a byte in hexa */
 
extern void lcd_puthex(unsigned char i);