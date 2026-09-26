/*
ENSE 352 Mastermind Lab Project Header File
Student: Rida Hashmi
SID: 200504477
*/

#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>

typedef struct {
    volatile uint32_t CR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t APB2RSTR;
    volatile uint32_t APB1RSTR;
    volatile uint32_t AHBENR;
    volatile uint32_t APB2ENR;
    volatile uint32_t APB1ENR;
    volatile uint32_t BDCR;
    volatile uint32_t CSR;
} RCC_TypeDef;

#define RCC ((RCC_TypeDef *) 0x40021000)
// Basically the power room address when we write RCC->, we're referring to this

typedef struct {
    volatile uint32_t CRL;   // Controls pins 0-7
    volatile uint32_t CRH;   // Controls pins 8-15
    volatile uint32_t IDR;   // Input data
    volatile uint32_t ODR;   // Output data
    volatile uint32_t BSRR;  // Bit set/reset
    volatile uint32_t BRR;   // Bit reset
} GPIO_TypeDef;

#define GPIOA ((GPIO_TypeDef *) 0x40010800) // Lights for D0, D1, D2
#define GPIOB ((GPIO_TypeDef *) 0x40010C00) // Light D3 and all buttons
#define GPIOC ((GPIO_TypeDef *)0x40011000U) // Address for PC13

#endif