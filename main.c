/*
 * File:   main.c
 * Author: M. Krejcir
 *
 * Created 27.6.2022
 */
#include <xc.h>
#include <stdio.h>
#include "display.h"

// minimal config bits setup
#pragma config WDTEN = OFF
#pragma config FOSC = INTIO7
#pragma config MCLRE = EXTMCLR
#pragma config FCMEN = ON

#define _XTAL_FREQ 128000

#define FRONT 0
#define BACK 1

int threshold_front = 0;
int threshold_back = 0;
int gate_distance = 23;
// 4MHz
#define TIMER_FREQ 4000000
int timer_overflows = 0;

int button_pushed = 0;


void __interrupt(high_priority) int0(void) {
    
    if (TMR0IE && TMR0IF) {
        TMR0IF = 0;
        timer_overflows += 1;
        return;
    }
    if (INT0IE && INT0IF) {
        button_pushed = 1;
        INT0IE = 0;
        INT0IF = 0;
    }
}


void setup_timer_int() {

    TMR0ON = 0;
    T08BIT = 0;
    T0CS = 0;
    T0SE = 0;
    PSA = 1;

    GIE = 0;
    PEIE = 1;
    TMR0IE = 1;
    IPEN = 1;
    //TMR0IP = 0;
    GIE = 1;
    return;
}

void setup_buttons_int() {
    GIE = 0;
    // int on falling edge
    INTEDG0 = 0;
    PEIE = 1;
    GIE = 1;
    INT0IF = 0;
    return;
}

int read_adc(void) {
    
    // turn on conversion
    ADCON0 = ADCON0 | 0x02;
    int result = 0;
    
    while(1) {
        
        if((ADCON0 & 0x02) == 0) {
            
            result = ADRESH * 4;
            result += ADRESL / 64;
            return result;
        }
        
    }
    
}


void change_position(int position) {
    
    if (position == FRONT) {
        
        ADCON0 = 0b00010001;
    } else {
        
        ADCON0 = 0b00001101;
    }
    return;
}


void calibrate(void) {
    
    PORTD = 0x11;
    lcd_puts("Calibrating...");
    int sum = 0;
    
    change_position(FRONT);
    
    for (int i = 0; i < 20; i++) {
        
        sum += read_adc();
    }
    
    threshold_front = sum / 21;
    sum = 0;
    change_position(BACK);
    
    for (int i = 0; i < 20; i++) {
        
        sum += read_adc();
    }
    
    threshold_back = sum / 21;
    
    
    __delay_ms(100000);
    __delay_ms(100000);
    PORTD = 0x00;
    lcd_clear();
    lcd_puts("Calibration done");
    return;
}


void debug_print() {
    
    change_position(FRONT);
    //change_position(BACK);
    char message[10] = {0};
    
    while (1) {
        
        int front = read_adc();
        sprintf(message, "%d", front);
        
        lcd_clear();
        lcd_puts(message);
        
        __delay_ms(25000);
    }
}


void print_threshold(void) {
    
    lcd_clear();
    
    char message_fr[20] = {0};
    sprintf(message_fr, "Th fr: %d", threshold_front);
    lcd_puts(message_fr);
    
    lcd_goto(40);
    char message_bc[20] = {0};
    sprintf(message_bc, "Th br: %d", threshold_back);
    lcd_puts(message_bc);
    return;
    
}


void print_time_velocity (double time, double velocity) {
    
    lcd_clear();
    
    char message_fr[20] = {0};
    sprintf(message_fr, "T[ms]: %f", time);
    lcd_puts(message_fr);
    
    lcd_goto(40);
    char message_bc[20] = {0};
    sprintf(message_bc, "V[cm/s]: %f", velocity);
    lcd_puts(message_bc);
    return;
    
    
}


double calculate_time(void) {
    
    int result = 0;
    result = timer_overflows * (65536 / (TIMER_FREQ / 1000));
    result += (TMR0H * 256 + TMR0L) / (TIMER_FREQ / 1000);
    return result;
    
}


float calculate_velocity(double time) {
    
    return gate_distance / (time/1000);
}


void main(void) {
    OSCCON = (OSCCON & 0b10001111) | 0b01110000; // internal oscillator at full speed (16 MHz)
    
    TRISB = 0xFF; //buttons input
    ANSELB = 0x00; //enable digital input buffer
    
    ANSELA = 0xff;
    TRISA = 0xff;
    
    // leds output
    TRISD = 0x00;
    
    // selects AN0 pin and enables ADC
    ADCON0 = 0b00000001;
    // selects internal voltage reference
    ADCON1 = 0b00000000;
    // left justified result format, appropriate clock selected
    ADCON2 = 0b00101001;

    // GPIOC output
    TRISC = 0x00;
    
    TRISE = 0x00;
    PORTE = 0xff;
    
    setup_timer_int();
    setup_buttons_int();
    
    lcd_init();
    lcd_clear();
    lcd_goto(0);
    
    calibrate();
    __delay_ms(100000);
    __delay_ms(100000);
    
    change_position(FRONT);
    
    //debug_print();
    
    print_threshold();
    __delay_ms(400000);
    lcd_clear();
    lcd_goto(0);
    lcd_puts("Measuring");

    while(1) {
        
        int front = read_adc();
 
        if (front < threshold_front) {
                
            // starts timer
            TMR0ON = 1;
            
            PORTD = 0x0d;
            change_position(BACK);
                
            while(1) {
                     
                int back = read_adc();
                
                if (back < threshold_back) {
                    
                    // stops timer
                    TMR0ON = 0;
                    double time = calculate_time();
                    timer_overflows = 0;
                    double velocity = calculate_velocity(time);
                    
                    lcd_clear();
                    lcd_goto(0);
                    lcd_puts("Bullet detected");
                    
                    PORTD = 0xdd;
                    __delay_ms(100000);
                    print_time_velocity(time, velocity);
                    
                    INT0IE = 1;
                    while (button_pushed == 0) {}
                    button_pushed = 0;
                    
                    PORTD = 0x00;
                    lcd_clear();
                    lcd_goto(0);
                    lcd_puts("Measuring");
                    break;
                }
            }

            change_position(FRONT);
        }
        
    }
}
