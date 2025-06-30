/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body for STM32F103 RTC Alarm Device
  * @description    : Uses internal RTC to wake from Standby mode at user-defined alarm time,
  *                  displays time on SSD1306 OLED, and rings buzzer for 10 seconds.
  *                  Returns to Standby mode to save power.
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "rtc.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SSD1306_I2C_ADDR (0x3C << 1) // SSD1306 I2C address (0x3C, shifted for HAL)
#define BUZZER_Pin GPIO_PIN_0
#define BUZZER_GPIO_Port GPIOA
#define BUTTON_HOUR_Pin GPIO_PIN_1
#define BUTTON_MINUTE_Pin GPIO_PIN_2
#define BUTTON_GPIO_Port GPIOA
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
RTC_TimeTypeDef sTime = {0};
RTC_DateTypeDef sDate = {0};
RTC_AlarmTypeDef sAlarm = {0};
uint8_t alarm_hour = 8; // Default alarm: 08:00
uint8_t alarm_minute = 0;
volatile uint8_t display_active = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void SSD1306_Init(void);
void SSD1306_Clear(void);
void SSD1306_DisplayTime(uint8_t hour, uint8_t minute, uint8_t second);
void Set_Alarm(uint8_t hour, uint8_t minute);
void Ring_Buzzer(uint32_t duration_ms);
void Enter_Standby_Mode(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* Minimal SSD1306 OLED Driver */
void SSD1306_WriteCommand(uint8_t cmd) {
    uint8_t data[2] = {0x00, cmd}; // Control byte: 0x00 for command
    HAL_I2C_Master_Transmit(&hi2c1, SSD1306_I2C_ADDR, data, 2, HAL_MAX_DELAY);
}

void SSD1306_WriteData(uint8_t *data, uint16_t size) {
    uint8_t buffer[33];
    buffer[0] = 0x40; // Control byte: 0x40 for data
    for (uint16_t i = 0; i < size; i++) {
        buffer[i + 1] = data[i];
        HAL_I2C_Master_Transmit(&hi2c1, SSD1306_I2C_ADDR, buffer, 2, HAL_MAX_DELAY);
    }
}

void SSD1306_Init(void) {
    HAL_Delay(100); // Wait for OLED to power up
    SSD1306_WriteCommand(0xAE); // Display OFF
    SSD1306_WriteCommand(0xD5); // Set display clock
    SSD1306_WriteCommand(0x80); // Default ratio
    SSD1306_WriteCommand(0xA8); // Set multiplex
    SSD1306_WriteCommand(0x3F); // 64 pixels
    SSD1306_WriteCommand(0xD3); // Set display offset
    SSD1306_WriteCommand(0x00); // No offset
    SSD1306_WriteCommand(0x40); // Set start line
    SSD1306_WriteCommand(0x8D); // Charge pump
    SSD1306_WriteCommand(0x14); // Enable charge pump
    SSD1306_WriteCommand(0x20); // Memory mode
    SSD1306_WriteCommand(0x00); // Horizontal addressing
    SSD1306_WriteCommand(0xA1); // Segment remap
    SSD1306_WriteCommand(0xC8); // COM scan direction
    SSD1306_WriteCommand(0xDA); // COM pins
    SSD1306_WriteCommand(0x12); // Default
    SSD1306_WriteCommand(0x81); // Contrast
    SSD1306_WriteCommand(0xCF); // Default
    SSD1306_WriteCommand(0xD9); // Pre-charge
    SSD1306_WriteCommand(0xF1); // Default
    SSD1306_WriteCommand(0xDB); // VCOMH
    SSD1306_WriteCommand(0x40); // Default
    SSD1306_WriteCommand(0xA4); // Entire display ON
    SSD1306_WriteCommand(0xA6); // Normal display
    SSD1306_WriteCommand(0xAF); // Display ON
}

void SSD1306_Clear(void) {
    uint8_t data[128] = {0};
    for (uint8_t page = 0; page < 8; page++) {
        SSD1306_WriteCommand(0xB0 + page); // Set page
        SSD1306_WriteCommand(0x00); // Low column
        SSD1306_WriteCommand(0x10); // High column
        SSD1306_WriteData(data, 128); // Write zeros
    }
}

void SSD1306_DisplayTime(uint8_t hour, uint8_t minute, uint8_t second) {
    char time_str[16];
    snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", hour, minute, second);
    SSD1306_Clear();
    // Simplified: Write time as raw pixels (replace with u8g2 for proper fonts)
    SSD1306_WriteCommand(0xB0); // Page 0
    SSD1306_WriteCommand(0x00); // Low column
    SSD1306_WriteCommand(0x10); // High column
    uint8_t mock_font[8] = {0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00}; // Dummy pattern
    for (uint8_t i = 0; i < strlen(time_str); i++) {
        SSD1306_WriteData(mock_font, 8); // Simulate character
    }
}

/* Alarm and Buzzer Functions */
void Set_Alarm(uint8_t hour, uint8_t minute) {
    sAlarm.AlarmTime.Hours = hour;
    sAlarm.AlarmTime.Minutes = minute;
    sAlarm.AlarmTime.Seconds = 0;
    sAlarm.Alarm = RTC_ALARM_A;
    sAlarm.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY; // Ignore date
    HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN);
    // Store in backup registers
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, hour);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR2, minute);
}

void Ring_Buzzer(uint32_t duration_ms) {
    HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
    HAL_Delay(duration_ms);
    HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
}

void Enter_Standby_Mode(void) {
    HAL_PWR_EnableBkUpAccess();
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, 0xA5A5); // Marker for reset check
    HAL_PWR_EnterSTANDBYMode();
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_RTC_Init();

  /* USER CODE BEGIN 2 */
  // Check if woken from Standby
  if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) == 0xA5A5) {
      HAL_PWR_EnableBkUpAccess();
      HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, 0x0000); // Clear marker
      // Read alarm time
      alarm_hour = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1);
      alarm_minute = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR2);
      // Get current time
      HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
      HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN); // Required for time to update
      // Initialize OLED
      SSD1306_Init();
      // Display time
      SSD1306_DisplayTime(sTime.Hours, sTime.Minutes, sTime.Seconds);
      // Ring buzzer for 10 seconds
      Ring_Buzzer(10000);
      // Re-set alarm for next day
      Set_Alarm(alarm_hour, alarm_minute);
      // Return to Standby
      Enter_Standby_Mode();
  }

  // Normal startup: Initialize time and alarm
  sTime.Hours = 0;
  sTime.Minutes = 0;
  sTime.Seconds = 0;
  HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 1;
  sDate.Year = 25;
  HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

  // Restore alarm from backup registers (if set previously)
  alarm_hour = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1);
  alarm_minute = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR2);
  if (alarm_hour > 23 || alarm_minute > 59) {
      alarm_hour = 8; // Default
      alarm_minute = 0;
  }
  Set_Alarm(alarm_hour, alarm_minute);

  // Initialize OLED
  SSD1306_Init();
  display_active = 1;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      if (display_active) {
          HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
          HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN); // Required
          SSD1306_DisplayTime(sTime.Hours, sTime.Minutes, sTime.Seconds);
      }
      HAL_Delay(1000); // Update display every second
      // Enter Standby after 30 seconds of inactivity (adjust as needed)
      static uint32_t idle_counter = 0;
      if (idle_counter++ > 30) {
          display_active = 0;
          SSD1306_WriteCommand(0xAE); // OLED OFF
          Enter_Standby_Mode();
      }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == BUTTON_HOUR_Pin) {
        alarm_hour = (alarm_hour + 1) % 24;
        Set_Alarm(alarm_hour, alarm_minute);
        display_active = 1;
        SSD1306_Init(); // Re-init OLED
    }
    else if (GPIO_Pin == BUTTON_MINUTE_Pin) {
        alarm_minute = (alarm_minute + 1) % 60;
        Set_Alarm(alarm_hour, alarm_minute);
        display_active = 1;
        SSD1306_Init(); // Re-init OLED
    }
}

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
    // Handled in main() via Standby wake-up
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */