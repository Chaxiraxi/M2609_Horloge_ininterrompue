// --- GPIO pins (directly on the MCU) ---
#define SET_BTN 9
#define RESET_PIN_23017 5
#define SCREEN_CONTRAST_PIN A0

// --- MCP23017 pin numbers (accent on the I/O expander, address 0x20) ---
// LCD data lines (directly addressed via ioFrom23017)
#define LCD_RS 8
#define LCD_EN 9
#define LCD_D4 10
#define LCD_D5 11
#define LCD_D6 12
#define LCD_D7 13

// Encoder + CFG button (MCP23017 port A, accent pins 1-3)
#define MCP_ENC_A 1    ///< Encoder channel A
#define MCP_ENC_B 2    ///< Encoder channel B
#define MCP_CFG_BTN 3  ///< CFG button