# Reference Links
  ## https://github.com/siorpaes/DTMF-decoder
  
# STM32 FSK Modulation Examples

This repository contains example code for implementing **Frequency-Shift Keying (FSK)** modulation on an STM32 microcontroller. Two approaches are provided: one using the STM32's DAC and timers to generate FSK signals directly, and another using an external SX1276 LoRa module for FSK modulation over RF.

## Table of Contents
- [Overview](#overview)
- [Approach 1: FSK Using STM32 DAC and Timers](#approach-1-fsk-using-stm32-dac-and-timers)
- [Setup Instructions](#setup-instructions)
- [License](#license)

## Overview
FSK is a modulation scheme where binary data is represented by discrete frequency changes of a carrier signal. This repository provides two methods to implement FSK on STM32 microcontrollers:
 **DAC-Based FSK**: Generates sine waves of different frequencies (e.g., 1 kHz and 2 kHz) using the STM32's DAC, DMA, and timers to represent binary 0 and 1.


These examples are written for STM32CubeIDE with HAL libraries and assume familiarity with STM32CubeMX for peripheral configuration.

## Approach : FSK Using STM32 DAC and Timers
This approach generates FSK signals by switching between two sine waves of different frequencies using the STM32's DAC, DMA, and timers. It’s suitable for low-frequency FSK (e.g., audio range).

### Code Snippet
```c
#include "stm32f4xx_hal.h"
#include <math.h>

// Sine wave lookup tables for two frequencies (e.g., 1 kHz and 2 kHz)
#define SINE_SAMPLES 100
uint32_t sine_wave_f1[SINE_SAMPLES]; // For frequency 1 (binary 0)
uint32_t sine_wave_f2[SINE_SAMPLES]; // For frequency 2 (binary 1)

// DAC, Timer, and DMA handles
DAC_HandleTypeDef hdac;
TIM_HandleTypeDef htim2;
DMA_HandleTypeDef hdma_dac1;

// Initialize sine wave tables
void init_sine_waves(void) {
    for (int i = 0; i < SINE_SAMPLES; i++) {
        sine_wave_f1[i] = (uint32_t)((sin(2 * M_PI * i / SINE_SAMPLES) + 1) * 2047); // 12-bit DAC, 1 kHz
        sine_wave_f2[i] = (uint32_t)((sin(4 * M_PI * i / SINE_SAMPLES) + 1) * 2047); // 2 kHz
    }
}

// Transmit FSK data
void transmit_fsk(uint8_t *data, uint8_t length) {
    for (uint8_t i = 0; i < length; i++) {
        // Select sine wave based on bit (0 or 1)
        uint32_t *wave = (data[i] == 0) ? sine_wave_f1 : sine_wave_f2;
        
        // Start DMA to DAC with selected wave
        HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, wave, SINE_SAMPLES, DAC_ALIGN_12B_R);
        
        // Wait for one bit duration (e.g., 1 ms for 1 kbps)
        HAL_Delay(1);
        
        // Stop DAC DMA
        HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);
    }
}

// Main function
int main(void) {
    HAL_Init();
    
    // Initialize DAC, Timer, and DMA (configured in STM32CubeMX)
    // Assume DAC1, TIM2, and DMA1 are set up
    init_sine_waves();
    
    // Example data to transmit
    uint8_t data[] = {1, 0, 1, 1, 0}; // Binary data
    transmit_fsk(data, sizeof(data));
    
    while (1) {
        // Main loop
    }
}
```

### Configuration
- **Hardware**: STM32 with DAC (e.g., STM32F4).
- **STM32CubeMX Setup**:
  - Configure DAC1 Channel 1 with DMA.
  - Set TIM2 to trigger DAC at 100 kHz (for 100 samples at 1 kHz).
  - Enable DMA1 in circular mode for DAC data transfer.
- **Output**: Connect DAC output (e.g., PA4) to an external circuit.
- **Notes**: Adjust `SINE_SAMPLES` and `HAL_Delay` for desired bit rate and frequency deviation.


## Setup Instructions
1. **Hardware Setup**:
   - For Approach 1: Use an STM32 board with DAC (e.g., STM32F4 Discovery). Connect DAC output to an oscilloscope or external circuit.
  
2. **Software Setup**:
   - Install STM32CubeIDE and STM32CubeMX.
   - Generate peripheral initialization code using STM32CubeMX (see configuration notes above).
   - Copy the provided code snippets into your project’s `main.c`.
3. **Testing**:
   - For Approach 1: Verify sine wave output with an oscilloscope.
4. **Dependencies**:
   - STM32 HAL libraries (included with STM32CubeIDE). 

## License
This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.