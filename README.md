# Nucleo-F103-MasterMind-Game

This repository contains a simple, single‑player Mastermind style game implementation for a STM32 Nucleo‑F103 microcontroller. The goal of the game is to guess a secret sequence of four hexadecimal digits (0–F) using a set of four DIP switches and to interpret feedback presented via LEDs and a push button. A linear feedback shift register (PRNG) generates a new secret code each round. The project was built for the ENSE 352 (Computer Systems Architecture) lab. It illustrates how to configure GPIO peripherals on an STM32, implement debouncing for inputs, and provide user feedback using simple LED patterns. No external libraries (other than the CMSIS headers) are required, and the main.c and main.h files included are sufficient for running the project.

## Author
This game was implemented by Rida Hashmi for ENSE 352 (Computer Systems Architecture) at the University of Regina. The code in this repository is provided for educational use.

## Hardware Requirements
To run this game you will need the following:
* **Microcontroller** – An STM32 Nucleo‑F103 board (or any STM32F103 with similar GPIO availability).
* **DIP switches** – Four SPST DIP switches wired to the microcontroller’s GPIOB pins.
* **LEDs** – Four LEDs connected to GPIOA and GPIOB pins via suitable resistors.
* **Wiring Equipment** – wires to connect to the Nucleo-F103 board and the breadboard and a 10k pull-up resistor

## Software Requirements
The code was uploaded to the board using Keil µVision 5.0.

### Pin Mapping
| **Component**    | **STM32 Pin** | **Purpose / Bit Position** |
|------------------|---------------|-----------------------------|
| **D0 LED**       | PA0           | Bit 0 (LSB)                 |
| **D1 LED**       | PA1           | Bit 1                       |
| **D2 LED**       | PA4           | Bit 2                       |
| **D3 LED**       | PB0           | Bit 3 (MSB)                 |
| **DIP Switch 0** | PB8           | Input Bit 0 (LSB)           |
| **DIP Switch 1** | PB9           | Input Bit 1                 |

## Pinout Reference Sheet:
<p align="center"> <img width="505" height="453" alt="image" src="https://github.com/user-attachments/assets/f667dffe-31be-47fb-b821-ae15ffff7445" /> </p>

## Gameplay Overview
When the board starts it generates a new four‑digit secret code. Each digit is a nibble between 0x0 and
0xF (0–15). **The player has up to 10 guesses to determine the secret**.

## Entering a guess
1. **Prepare a digit** – Position the four DIP switches to represent your guess nibble. The binary value
of the switches is read with PB8 as the least‑significant bit and PB4 as the most‑significant bit.
2. **Confirm the digit** – Press the blue user button (PC13). The current LED pattern shows which
digit index you are entering:
   * For the first digit (index 0) LED D3 is lit (1000), then the pattern builds up to 1111 for the
fourth digit. This acts as a cursor showing which digit you are currently entering.
3. **Repeat** – Enter all four digits one after the other. After each press, wait for the button to be
released (debouncing occurs automatically).
   * For the first digit (index 0) LED D3 is lit (1000), then the pattern builds up to 1111 for the
fourth digit. This acts as a cursor showing which digit you are currently entering.

## Feedback Interpretation
After you have confirmed all four digits, the program compares your guess with the secret code and
provides feedback through the LEDs:
* **Solid LEDs (lit)** – Indicate the number of digits that are correct and in the correct position. For
example, two solid lights mean two of your four nibbles match exactly.
* **Blinking LEDs** – Indicate the number of digits that are correct but in the wrong position. These
LEDs blink on and off at a fixed rate. For example, if two LEDs are solid and one blinks, two
digits are in the right place and one digit is present in the secret but in a different position.
The game waits in this feedback state until you press the user button. Once pressed and released, the
LEDs turn off and you may begin entering your next guess.

## Winning and losing
* **Win** – If all four digits are correct in the correct positions, all LEDs blink together four times.
Then the LEDs display your guess count in binary. Press the button once more to start a new
round.
* **Lose** – If you have not guessed the code within 10 attempts, a sweeping animation moves a single
lit LED from D3 to D0 four times. Afterwards the program flashes each digit of the secret code in
sequence for approximately two seconds per digit, with the least significant bit on LED D0
(which should be connected to PA0) and the most significant bit on LED D3 (which should be
connected to PB0). Once the code has been displayed, press the button to begin a new round.

Customisation and notes
* **Debugging** – A hard‑coded secret can be uncommented in generate_code() in main.c to aid
debugging and test the feedback. By default, the game uses a simple 32‑bit linear congruential
generator to produce pseudorandom nibbles for the secret code.
* **Timing** – The blink and delay loops use busy‑waits calibrated for a 72 MHz clock. If your board
runs at a different speed you may need to adjust the delay_cycles() counts.
* **Safety** – Always ensure appropriate current‑limiting resistors are used with LEDs and that the
DIP switches do not short the pins directly to ground or VDD.

<p align="center"> <img width="688" height="448" alt="image" src="https://github.com/user-attachments/assets/274f3709-3e61-4e5e-85be-2075e5c9244c" /> </p>

***Note:*** 
The black 10k bussed resistor is used as the pull-up (the logic is inverted in the code due to this), and the yellow resistor is an isolated 270 ohm resistor. If you don’t have this, you can use 4 individual 270 ohm resistors in line with the LEDs.


<p align="center"> <img width="718" height="594" alt="image" src="https://github.com/user-attachments/assets/7496446c-fad9-4aea-b754-8b79d10461ea" /> </p>


