// --- GPIO pins (directly on the MCU) ---
#define SET_BTN 9
#define RESET_PIN_23017 5
#define SCREEN_CONTRAST_PIN A0

// --- I2C bus clock (Hz) ---
// Change this value to tune I2C speed globally.
#define I2C_CLOCK_HZ 100000UL

// --- MCP23017 pin numbers (accent on the I/O expander, address 0x20) ---
// LCD data lines (directly addressed via ioFrom23017)
#define LCD_RS 8
#define LCD_EN 9
#define LCD_D4 10
#define LCD_D5 11
#define LCD_D6 12
#define LCD_D7 13
#define LED_NTP 3
#define LED_DAB 4
#define LED_GPS 5

// Encoder + CFG button (MCP23017 port A, accent pins 1-3)
#define MCP_ENC_A 0    ///< Encoder channel A
#define MCP_ENC_B 1    ///< Encoder channel B
#define MCP_CFG_BTN 2  ///< CFG button