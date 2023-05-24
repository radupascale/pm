#ifndef ARDUINO_H
#define ARDUINO_H
#include <Arduino.h>
#endif

#ifndef MUX_H
#define MUX_H
#define RANKS 8
#define FILES 8

extern const int selectPins[3]; // S0~3, S1~4, S2~5
extern const int zInput;		// Connect common (Z) to A0 (analog input)
extern byte chessboard[64];		// Could be 8 bytes, but 64 bytes is easier to work with

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
#define BOARD_STATE 'B'
#define BOARD_MOVE 'M'
#define BOARD_TIME 'T'
#define BOARD_TURN 'U'
#define WHITE 1
#define BLACK 0

void send_chessboard(void);
int readline(int readch, char *buffer, int len);
#endif

#ifndef TFT_H
#define TFT_H
#include "Ucglib.h"
#include <Adafruit_GFX.h>
#include "Adafruit_ILI9341.h"
#include "ChessFont25.h"
#include <SPI.h>
#include <pieces.h>
typedef unsigned int lichess_time_t;
extern Ucglib_ILI9341_18x240x320_HWSPI ucg;

class TFT
{
  public:
	TFT();
	void setup_fonts(void);
	void setup_tft(void);
	void draw_time(lichess_time_t time_opponent, lichess_time_t time_player);
	void draw_last_move(const char *move);
	void draw_chessboard(void);
	void draw_piece(char piece, int rank, int file);
	void draw_pieces(void);
	void draw_game(void);
    void draw_bitmap(void);
	void update_pieces(const char *new_board);

  private:
	/* Chessboard box variables */
	int chessboard_height = 160;
	int chessboard_width = 160;
	int chessboard_x = 40;
	int chessboard_y = 80;
	char ranks[8] = {'1', '2', '3', '4', '5', '6', '7', '8'};
	char files[8] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};
	char display_chessboard[64];
	/* Time boxes for players */
	int time_height = 40;
	int time_width = 240;
	int time_black_x = 0;
	int time_black_y = 0;
	int time_white_x = 0;
	int time_white_y = 280;
    int time_ox_offset = 75;
    int time_oy_offset = 20;
	/* Last move box */
	int last_move_height = 20;
	int last_move_width = 240;
	int last_move_x = 0;
	int last_move_y = 260;
};
#endif

#ifndef TIMER_H
#define TIMER_H
extern volatile int read_board;

void setup_timers(void);
void update_time(const char *time_packet);
extern volatile bool turn;
extern lichess_time_t white_time;
extern lichess_time_t black_time;
extern volatile bool clock_started;
extern volatile bool second_passed;
#endif