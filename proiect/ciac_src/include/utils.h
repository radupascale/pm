#ifndef ARDUINO_H
    #define ARDUINO_H
    #include <Arduino.h>
#endif

#ifndef MUX_H
    #define MUX_H
    #define RANKS 8
    #define FILES 8

    extern const int selectPins[3];  // S0~3, S1~4, S2~5
    extern const int zInput;  // Connect common (Z) to A0 (analog input)
    extern byte chessboard[64]; // Could be 8 bytes, but 64 bytes is easier to work with

    void selectMuxPin(byte pin);
    void read_mux(int rank);
    void setup_mux(void);
#endif

#ifndef SHIFT_REG_H
    #define SHIFT_REG_H
    #define BUFF_SIZE 100
    extern char buff[BUFF_SIZE];
    // Pin connected to ST_CP of 74HC595
    extern int latchPin;
    // Pin connected to SH_CP of 74HC595
    extern int clockPin;
    ////Pin connected to DS of 74HC595
    extern int dataPin;
    // Pin connected to MR of 74HC595
    extern int resetPin;
    // holders for information you're going to pass to shifting function
    extern byte data;
    extern byte dataArray[10];

    void shiftOut(int myDataPin, int myClockPin, byte myDataOut);
    void clear_registers(void);
    void light_rank(int rank);
    void setup_shift_register(void);
#endif

#ifndef SERIAL_H
    #define SERIAL_H
    #define BOARD_STATE_BYTE 'B'
    #define BOARD_STATE_INFO 'I'

    void send_chessboard(void);
    int readline(int readch, char *buffer, int len);
#endif

#ifndef TFT_H
    #define TFT_H
    #include <Adafruit_GFX.h>
    #include <SPI.h>
    #include "Ucglib.h"
    extern Ucglib_ILI9341_18x240x320_HWSPI ucg;

    void setup_tft(void);
    void hello_world(void);
#endif

#ifndef TIMER_H
    #define TIMER_H
    extern volatile int read_board;

    void setup_timers(void);
#endif