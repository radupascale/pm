#include "utils.h"
void setup_tft() {
	ucg.begin(UCG_FONT_MODE_TRANSPARENT);
	ucg.clearScreen();
}

void hello_world() {
	ucg.setRotate90();
	ucg.setFont(ucg_font_ncenR14_tr);
	ucg.setPrintPos(0,25);
	ucg.setColor(255, 255, 255);
	ucg.print("Hello World!");
}