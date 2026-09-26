/*
 * ENSE 352 Lab Project: Mastermind Game 
 * Author: Rida Hashmi
 * SID: 200504477
 *
 * This program implements a simple single-player Mastermind game using a
 * Nucleo F103 microcontroller.  The code generates a random 4-digit secret
 * code (from hexes 0–15/F) and prompts the user to guess it using four
 * DIP switches.  After each guess, the board provides feedback through four
 * LEDs indicating how many digits are correct (solid) and how many are 
 * correct but in the wrong position (flashing). The blue user button on PC13 
 * logs each guess digit, advances between guesses, and starts new rounds.
 * The user gets 10 tries.
 *
 * Hardware connections:
 *   • Four LEDs (D0–D3) are connected to PA0, PA1, PA4 and PB0.
 *   • Four DIP switches are connected to PB8, PB9, PB6 and PB4
 *     (LSB on PB8 and MSB on PB4).
 *   • The blue user push button is connected to PC13 (active low).
 *
 * Note: At the end of the lose state when the correct secret digits are flashed,
 * LSB is shown from the LED at PA0 and MSB is shown from the LED at PB0
 * On my wiring this is left to right (LSB -> MSB) to visually correspond to the 
 * LSB and MSB ordering on the dip switches (more intuitive to interpret).
 */

#include "main.h"
#include <stdint.h>

/* Forward declarations of helper functions: */
static void gpio_init(void);                 //Set up all GPIO pins (LEDs, switches, button)
static uint8_t read_dip(void);               //Read the 4 DIP switches and return a 4-bit value
static void set_leds(uint8_t pattern);       //Turn LEDs on/off according to a 4-bit pattern
static void delay_cycles(volatile uint32_t count); //Simple delay loop, reusable
static int button_pressed(void);             //Return 1 if the blue button is currently pressed
static void wait_button_press(void);         //Wait until the button is pressed (with debounce)
static void wait_button_release(void);       //Wait until the button is released (with debounce)
static void display_selection(uint8_t digit_index); //Show which digit the user is entering
static void show_feedback(int correct, int wrong);  //Show correct/wrong digits using LEDs
static void win_sequence(uint8_t guess_count);      //Play win animation and show guess count
static void lose_sequence(void);             //Play lose animation and then reveal the code
static void generate_code(uint8_t code[4]);  //Create a new 4-digit secret code
static uint8_t next_random_nibble(void);     //Generate a pseudorandom value from 0–15
static void display_secret_code(void);       //Display the secret code after a loss

/* Random number seed for code generation */
static uint32_t rng_seed = 0x12345678U;

/* This array stores a copy of the secret code which is updated in
 * generate_code() and read by display_secret_code().  Declared static so
 * that it retains its value between function calls within this file.*/
static uint8_t last_secret[4] = {0};

int main(void) {
    //Initialise GPIO ports and pins 
    gpio_init();

    //Main game loop – run forever 
    for (;;) {
        //Generate a new random 4 digit code (digits 0-15) 
        uint8_t secret[4];
        generate_code(secret);

        //Counter for number of guesses
        uint8_t guess_count = 0;

        //Loop until the user wins or exhausts attempts
        for (;;) {
            ++guess_count;

            /* Read a 4 digit guess.  For each digit the LEDs
             * indicate which digit is being entered (1000, 1100, 1110, 1111)
             * and the DIP switches provide the value.  The user presses
             * the button to confirm each digit. */
            uint8_t guess[4] = {0};
            for (uint8_t i = 0; i < 4; ++i) {
                display_selection(i);
                //Wait for the user to press the button to capture the digit */
                wait_button_press();
							//Read DIP switches (invert logic due to pull ups)--> if (!(GPIOB->IDR & (1U << pin)))*/
                guess[i] = read_dip();
                //Debounce – wait until button released before next digit */
                wait_button_release();
            }

            /* Compute feedback: count digits in correct position and
             * digits present but in wrong position.  Use flags to
             * avoid counting the same code digit more than once. */
            int correct = 0;
            int wrong = 0;
            uint8_t used_code[4] = {0};
            uint8_t used_guess[4] = {0};

            //First pass – exact matches 
            for (int i = 0; i < 4; ++i) {
                if (guess[i] == secret[i]) {
                    ++correct;
                    used_code[i] = 1;
                    used_guess[i] = 1;
                }
            }
            //Second pass – count correct digits in wrong positions
            for (int i = 0; i < 4; ++i) {
                if (used_guess[i]) {
                    continue;
                }
                for (int j = 0; j < 4; ++j) {
                    if (used_code[j]) {
                        continue;
                    }
                    if (guess[i] == secret[j]) {
                        ++wrong;
                        used_code[j] = 1;
                        break;
                    }
                }
            }

            //Check win condition 
            if (correct == 4) {
                win_sequence(guess_count);
                break; //start new round 
            }
            //Check lose condition – allow up to 8 guesses
            if (guess_count >= 10U) {
                lose_sequence();
                break; //start new round 
            }

            /* Provide feedback: show number of correct and wrong digits
             * by lighting LEDs.  LEDs remain in this pattern until the
             * user presses the button to begin the next guess. */
            show_feedback(correct, wrong);
            /* When show_feedback returns, the button has just been released,
             * so continue to next guess. */
        }
    }
}

/*------------------------------------------------------------------*/
/* GPIO initialisation                                              */
/*------------------------------------------------------------------*/
static void gpio_init(void) {
    /* Enable clocks for GPIOA, GPIOB and GPIOC in RCC->APB2ENR.
     * Bits: 2 = GPIOA, 3 = GPIOB, 4 = GPIOC.
     */
    RCC->APB2ENR |= (1U << 2) | (1U << 3) | (1U << 4);

    /* --- Configure LED pins as outputs (10 MHz push pull) ---
     * D0 on PA0, D1 on PA1, D2 on PA4, D3 on PB0.*/
    //Clear configuration bits then set MODE=01 (10 MHz) and CNF=00 (push pull)
    //PA0 (bits 3:0), PA1 (bits 7:4), PA4 (bits 19:16)
    GPIOA->CRL &= ~((0xFU << (0 * 4)) | (0xFU << (1 * 4)) | (0xFU << (4 * 4)));
    GPIOA->CRL |=  ((0x1U << (0 * 4)) | (0x1U << (1 * 4)) | (0x1U << (4 * 4)));
    //PB0 (bits 3:0)
    GPIOB->CRL &= ~(0xFU << (0 * 4));
    GPIOB->CRL |=  (0x1U << (0 * 4));

    //Initialise LEDs to off
    GPIOA->BRR = (1U << 0) | (1U << 1) | (1U << 4);
    GPIOB->BRR = (1U << 0);

    /* --- Configure DIP switch pins as inputs with pull up ---
     * DIP0 (LSB) on PB8
     * DIP1 on PB9
     * DIP2 on PB6
     * DIP3 (MSB) on PB4
     * For input with pull up: MODE=00, CNF=10 (0b1000).*/
    //PB4 (bits 19:16) and PB6 (bits 27:24) in CRL 
    GPIOB->CRL &= ~((0xFU << (4 * 4)) | (0xFU << (4 * 6)));
    GPIOB->CRL |=  ((0x8U << (4 * 4)) | (0x8U << (4 * 6)));
    //PB9 and PB10 in CRH (pins 8–15) 
    //Bits positions: (pin - 8) * 4 
    GPIOB->CRH &= ~((0xFU << ((9 - 8) * 4)) | (0xFU << ((10 - 8) * 4)));
    GPIOB->CRH |=  ((0x8U << ((9 - 8) * 4)) | (0x8U << ((10 - 8) * 4)));
    //Enable internal pull ups by writing 1 to the ODR bits */
    GPIOB->ODR |= (1U << 4) | (1U << 6) | (1U << 9) | (1U << 10);

    /* --- Configure PC13 (blue user button) as input with pull up ---
     * PC13 sits in CRH at pin index 13.  MODE=00, CNF=10.*/
    GPIOC->CRH &= ~(0xFU << ((13U - 8U) * 4U));
    GPIOC->CRH |=  (0x8U << ((13U - 8U) * 4U));
    //Activate pull up (set ODR bit) */
    GPIOC->ODR |= (1U << 13);
}

/*------------------------------------------------------------------*/
/* Read DIP switches and return a 4 bit value (0–15).  Each switch  */
/* pulls the pin low when ON, so the bits are inverted.             */
/*------------------------------------------------------------------*/
//(invert logic due to pull ups)--> if (!(GPIOB->IDR & (1U << pin)))
static uint8_t read_dip(void) { 
    uint8_t value = 0;
    //LSB – PB8 
    if (!(GPIOB->IDR & (1U << 8))) {
        value |= 0x1U;
    }
    //Bit 1 – PB9 
    if (!(GPIOB->IDR & (1U << 9))) {
        value |= 0x2U;
    }
    //Bit 2 – PB6 
    if (!(GPIOB->IDR & (1U << 6))) {
        value |= 0x4U;
    }
    //MSB – PB4 
    if (!(GPIOB->IDR & (1U << 4))) {
        value |= 0x8U;
    }
    return value;
}
/*------------------------------------------------------------------*/
/* Set the four LEDs based on a 4‑bit pattern.  bit3 → D3 (PB0),    */
/* bit2 → D2 (PA4), bit1 → D1 (PA1), bit0 → D0 (PA0).               */
/*------------------------------------------------------------------*/
static void set_leds(uint8_t pattern) {
    //Turn off all LEDs first 
    GPIOA->BRR = (1U << 0) | (1U << 1) | (1U << 4);
    GPIOB->BRR = (1U << 0);

    //Turn on LEDs according to pattern bits 
    if (pattern & 0x1U) {
        GPIOA->BSRR = (1U << 0);
    }
    if (pattern & 0x2U) {
        GPIOA->BSRR = (1U << 1);
    }
    if (pattern & 0x4U) {
        GPIOA->BSRR = (1U << 4);
    }
    if (pattern & 0x8U) {
        GPIOB->BSRR = (1U << 0);
    }
}

/*------------------------------------------------------------------*/
/* This crude delay loops until 'count' reaches zero.               */
/* Each iteration executes a NOP (no-operation) instruction,        */
/* wasting CPU cycles to create an approximate delay.               */
/* 'count' is adjustable for different situations                   */
/*------------------------------------------------------------------*/
static void delay_cycles(volatile uint32_t count) {
    while (count-- > 0U) {
        __asm__ volatile ("nop");
    }
}

/*------------------------------------------------------------------*/
/* Check if the user button is currently pressed.  PC13 is active   */
/* low (logic 0 when pressed).                                      */
/*------------------------------------------------------------------*/
static int button_pressed(void) {
    return ((GPIOC->IDR & (1U << 13)) == 0U);
}

/*------------------------------------------------------------------*/
/* Wait for the user to press the button (with basic debounce).      */
/*------------------------------------------------------------------*/
static void wait_button_press(void) {
    //Wait for a transition from released to pressed 
    while (!button_pressed()) {
        //spin until pressed 
    }
    //Debounce – wait a short period while still pressed 
    delay_cycles(50000U);
}

/*------------------------------------------------------------------*/
/* Wait for the user to release the button after a press.           */
/*------------------------------------------------------------------*/
static void wait_button_release(void) {
    while (button_pressed()) {
        //spin until released 
    }
    //Debounce 
    delay_cycles(50000U);
}

/*------------------------------------------------------------------*/
/* Display which digit is currently being selected.  The pattern    */
/* progresses as 1000, 1100, 1110 and 1111 for digit indices 0–3.   */
/*------------------------------------------------------------------*/
static void display_selection(uint8_t digit_index) {
    uint8_t pattern = 0U;
    for (uint8_t i = 0; i <= digit_index && i < 4; ++i) {
        pattern |= (0x8U >> i);
    }
    set_leds(pattern);
}

/*------------------------------------------------------------------*/
/* Provide feedback on the number of digits correct and incorrect.   */
/* The first 'correct' LEDs are solidly lit.  The next 'wrong' LEDs */
/* blink on and off.  The LEDs remain in this state until the       */
/* button is pressed, upon which this function returns.             */
/*------------------------------------------------------------------*/
static void show_feedback(int correct, int wrong) {
    int blink_state = 0;
    for (;;) {
        //If button pressed, exit after debounce 
        if (button_pressed()) {
            //small debounce delay 
            delay_cycles(50000U);
            //wait for release 
            wait_button_release();
            //Turn off LEDs 
            set_leds(0U);
            return;
        }

        //Build pattern for this cycle 
        uint8_t pattern = 0U;
        for (int i = 0; i < 4; ++i) {
            //Index from 0..3 correspond to LED bits 3..0 
            if (i < correct) {
                //solid LEDs 
                pattern |= (0x8U >> i);
            } else if (i < correct + wrong) {
                //blinking LEDs 
                if (blink_state) {
                    pattern |= (0x8U >> i);
                }
            }
        }
        set_leds(pattern);
        //Delay for blink timing 
        delay_cycles(200000U);
        //Toggle blink state 
        blink_state = !blink_state;
    }
}

/*------------------------------------------------------------------*/
/* Display win animation: blink all LEDs together four times then    */
/* show the number of guesses as a 4 bit binary on the LEDs.         */
/* Wait for the button press before starting a new game.             */
/*------------------------------------------------------------------*/
static void win_sequence(uint8_t guess_count) {
    //Blink all LEDs on/off 4x
    for (int i = 0; i < 4; ++i) {
        set_leds(0xF);
        delay_cycles(300000U);
        set_leds(0x0);
        delay_cycles(300000U);
    }
    //Display number of guesses (1–8) on LEDs as binary
    set_leds(guess_count & 0x0FU);
    //Wait for user to press and release the button 
    wait_button_press();
    wait_button_release();
    //Turn off LEDs 
    set_leds(0U);
}

/*------------------------------------------------------------------*/
/* Display lose animation: cycle a moving light pattern four times  */
/* across the LEDs then wait for button press to start a new game.  */
/*------------------------------------------------------------------*/
static void lose_sequence(void) {
    //4 cycles
    for (int r = 0; r < 4; ++r) {
        //Move a single LED from D3 to D0 
        for (int step = 0; step < 4; ++step) {
            //Pattern: 1000, 0100, 0010, 0001 
            uint8_t pattern = 0x8U >> step;
            set_leds(pattern);
            delay_cycles(200000U);
        }
    }
    //Clear LEDs
    set_leds(0U);
    /* After the lose animation, flash the correct secret code on the LEDs.
     * This gives the player feedback on what the actual code was. */
    display_secret_code();
    //Now wait for user button to start a new game
    wait_button_press();
    wait_button_release();
}

/*--------------------------------------------------------------------*/
/* Display the secret code to the user after a loss.  The correct code*/
/* is flashed digit by digit on the four LEDs.  The LSB of each nibble*/
/* corresponds to LED D0 (PA0) and the MSB corresponds to LED D3(PB0) */
/* Each digit is displayed for a short interval with all LEDs turned  */
/* off in between.                                                    */
/*--------------------------------------------------------------------*/
static void display_secret_code(void) {
    //Loop through the 4 digits stored in last_secret and display each
    for (uint8_t i = 0; i < 4; ++i) {
        //Show the digit on the LEDs 
        set_leds(last_secret[i] & 0x0FU);
        /* Hold the display for a longer period so each digit remains visible
             * for roughly two seconds.  Adjust the count empirically on hardware
             * to achieve a two second delay. */
            delay_cycles(8000000U);
        //Turn off LEDs to create a visible separation between digits
        set_leds(0U);
        //Brief pause before the next digit
        delay_cycles(300000U);
    }
}

/*------------------------------------------------------------------*/
/* Generate a random 4 digit code.  Each digit is a nibble (0–15).  */
/*------------------------------------------------------------------*/
static void generate_code(uint8_t code[4]) {
    //Hard-coded secret value UNCOMMENT FOR DEBUGGING/TESTING: 0x12AF (1, 2, A, F)
    //code[0] = 0x1;  // digit 1
    //code[1] = 0x2;  // digit 2
    //code[2] = 0xA;  // digit 3
    //code[3] = 0xF;  // digit 4

    /* Copy the secret into last_secret for later display.  This allows the
     * lose_sequence() function to reveal the correct code to the player
     * after the loss indication completes.  Only the lower 4 bits of each
     * nibble are stored to ensure no higher bits accidentally light LEDs. */
    for (int i = 0; i < 4; ++i) {
        last_secret[i] = code[i] & 0x0FU;
    }

    /*random version (COMMENT WHEN DEBUGGING/TESTING with hard-coded value):*/
		
     for (int i = 0; i < 4; ++i) {
         code[i] = next_random_nibble();
         last_secret[i] = code[i] & 0x0FU; // also store for display
     }
		
    
}
/*------------------------------------------------------------------*/
/* Simple 32 bit LCG to generate pseudorandom 4 bit values.         */
/*------------------------------------------------------------------*/
static uint8_t next_random_nibble(void) {
    //Constants from Numerical Recipes 
    rng_seed = rng_seed * 1664525U + 1013904223U;
    return (uint8_t)((rng_seed >> 28U) & 0x0FU);
}