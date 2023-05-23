
#include "utils.h"
void TFT::setup_tft(void)
{
	ucg.begin(UCG_FONT_MODE_TRANSPARENT);
	setup_fonts();
	ucg.clearScreen();

	for (int i = 0; i < RANKS * FILES; i++)
		display_chessboard[i] = EMPTY_SQUARE;
}

TFT::TFT()
{
}

/**
 * @brief Setup the rectangles on which the game will be displayed.
 * TODO: Depending on the type of the game, we will display the last move or not.
 *
 */
void TFT::setup_fonts(void)
{
	ucg.setFont(ucg_font_fub17_hr);
}

void TFT::draw_game(void)
{
	/* TODO: SEPARATE FUNCTION */
	/* Draw background rectangle*/
	ucg.setColor(153, 255, 204);
	ucg.drawBox(0, 0, 240, 320);

	/* TO DO: SEPARATE FUNCTION */
	/* Draw rectangles above and below the chessboard where the time will be displayed */
	ucg.setColor(128, 128, 128);
	ucg.drawBox(time_opponent_x, time_opponent_y, time_width, time_height);
	ucg.drawBox(time_player_x, time_player_y, time_width, time_height);

	draw_chessboard();
	draw_pieces();
}

/**
 * @brief Time is stored as an integer representing the number of seconds left.
 * So a value of 600 means 10 minutes left.
 * @param time_opponent
 * @param time_player
 */
void TFT::draw_time(lichess_time_t time_opponent, lichess_time_t time_player)
{
	char text[100];
	/* TODO: MACROS */
	int hours_opponent = time_opponent / 3600;
	int minutes_opponent = (time_opponent % 3600) / 60;
	int seconds_opponent = (time_opponent % 3600) % 60;
	int hours_player = time_player / 3600;
	int minutes_player = (time_player % 3600) / 60;
	int seconds_player = (time_player % 3600) % 60;

	/* TODO: SEPARATE FUNCTIONS, MAYBE REDRAW ONLY A BOX IN WHICH THE TIME FITS */
	ucg.setColor(128, 128, 128);
	ucg.drawBox(time_opponent_x, time_opponent_y, time_width, time_height);
	ucg.drawBox(time_player_x, time_player_y, time_width, time_height);

	/* TODO: ONLY UPDATE THE TIME IF IT CHANGED. SELECT A BIGGER FONT. CENTER THE TIME. */
	sprintf(text, "%02d:%02d:%02d", hours_opponent, minutes_opponent, seconds_opponent);
	ucg.setColor(0, 0, 0); // Black color for the text
	ucg.setPrintPos(time_opponent_x + 30, time_opponent_y + time_height / 2);
	ucg.print(text);

	sprintf(text, "%02d:%02d:%02d", hours_player, minutes_player, seconds_player);
	ucg.setColor(0, 0, 0);
	ucg.setPrintPos(time_player_x + 30, time_player_y + time_height / 2);
	ucg.print(text);
}

void TFT::draw_piece(char piece, int rank, int file)
{
	rank = RANKS - rank - 1;

	int x = chessboard_x + file * chessboard_width / FILES;
	int y = chessboard_y + rank * (chessboard_height / RANKS);
	int color;

	/* Reset the current board square color */
	if ((rank + file) % 2 == 0) {
		ucg.setColor(255, 255, 255);
		color = WHITE;
	} else {
		ucg.setColor(153, 76, 0);
		color = BLACK;
	}

	ucg.drawBox(x, y, chessboard_width / FILES, chessboard_height / RANKS);

	/* TODO: ADD SWITCH CASE TO PRINT BMP DEPENDING ON PIECE VALUE */

	/* Set the font color depending on the color of the square */
	if (color == WHITE)
		ucg.setColor(0, 0, 0);
	else
		ucg.setColor(255, 255, 255);

	/* Lowercase pieces need a little lift. The white queen has the biggest ascii value.*/
	if (piece > WHITE_ROOK)
		y -= 5;

	ucg.setPrintPos(x, y + chessboard_height / RANKS);
	ucg.print(piece);
}

/**
 * @brief Draw squares of alternating colors
 *
 */
void TFT::draw_chessboard(void)
{
	/* Print the name of the FILES above the first line */
	ucg.setColor(0, 0, 0);
	for (int i = 0; i < RANKS; i++) {
		ucg.setPrintPos(chessboard_x + i * chessboard_width / RANKS, chessboard_y);
		ucg.print(files[i]);
	}
	/* Draw squares of alternating COLORS */
	for (int i = 0; i < RANKS; i++) {
		ucg.setColor(0, 0, 0);
		ucg.setPrintPos(chessboard_x - 15, chessboard_y + i * chessboard_height / RANKS + 20);
		ucg.print(ranks[RANKS - i - 1]);
		for (int j = 0; j < FILES; j++) {
			if ((i + j) % 2 == 0)
				ucg.setColor(255, 255, 255);
			else
				ucg.setColor(153, 76, 0);
			ucg.drawBox(chessboard_x + j * chessboard_width / FILES, chessboard_y + i * chessboard_height / RANKS,
						chessboard_width / FILES, chessboard_height / RANKS);
		}
		ucg.setColor(0, 0, 0);
		ucg.setPrintPos(chessboard_x + chessboard_width, chessboard_y + i * chessboard_height / RANKS + 20);
		ucg.print(ranks[RANKS - i - 1]);
	}
	/* Print the name of the FILES below the last line */
	ucg.setColor(0, 0, 0);
	for (int i = 0; i < RANKS; i++) {
		ucg.setPrintPos(chessboard_x + i * chessboard_width / RANKS, chessboard_y + chessboard_height + 20);
		ucg.print(files[i]);
	}
}

/**
 * @brief Draw the pieces as stored in display_chessboard
 *
 */
void TFT::draw_pieces(void)
{
	for (int i = 0; i < RANKS * FILES; i++) {
		if (display_chessboard[i] != EMPTY_SQUARE) {
			draw_piece(display_chessboard[i], i / RANKS, i % FILES);
		}
	}
}

/**
 * @brief Update the internal board with the new position of the pieces.
 * Clean the previous squares and draw the piece on the new square.
 *
 * @param new_board
 */
void TFT::update_pieces(const char *new_board)
{
	for (int i = 0; i < RANKS * FILES; i++) {
		if (new_board[i] != display_chessboard[i]) {
			draw_piece(EMPTY_SQUARE, i / RANKS, i % FILES);
			display_chessboard[i] = new_board[i];
			draw_piece(display_chessboard[i], i / RANKS, i % FILES);
		}
	}
}

void TFT::draw_last_move(const char *move)
{
	char text[100];

	ucg.setColor(255, 0, 0);
	ucg.drawBox(last_move_x, last_move_y, last_move_width, last_move_height);

	strcpy(text, "Last move: ");
	strcat(text, move);
	ucg.setColor(0, 0, 0); // Black color for the text
	ucg.setPrintPos(last_move_x + 30, last_move_y + last_move_height);
	ucg.print(text);
}