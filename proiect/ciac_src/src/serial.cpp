#include "utils.h"

void send_chessboard() 
{
    /* Send the chessboard to the serial port from the eight rank to the first rank
     * and from the first file to the eighth file.
    */
    Serial.write(BOARD_STATE);  // B for board
    Serial.write(chessboard, RANKS * FILES);
    Serial.write('\n');
}

int readline(int readch, char *buffer, int len)
{
	static int pos = 0;
	int rpos;

	if (readch > 0) {
		switch (readch) {
			case '\r': // Ignore CR
				break;
			case '\n': // Return on new-line
				rpos = pos;
				pos = 0;  // Reset position index ready for next time
				return rpos;
			default:
				if (pos < len-1) {
					buffer[pos++] = readch;
				}
		}
	}
	return 0;   
}