#include <windows.h>
#include <cstdlib>
#include <cstdio>

void SetColor(unsigned int uFore,unsigned int uBack) { //更改之后输出的字体颜色
    HANDLE handle=GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(handle,uFore+uBack*0x10);
}
void SetSize(unsigned int uCol,unsigned int uLine) { //更改控制台大小
    char cmd[64];
    sprintf(cmd,"mode con cols=%d lines=%d",uCol,uLine);
    system(cmd);
}
void SetTitle(LPCSTR lpTitle) {
	SetConsoleTitle(lpTitle);
}
void SetPosC(COORD a) { // set cursor
	HANDLE out=GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleCursorPosition(out, a);
}
void SetPos(int x, int y) { // set cursor2
	COORD pos= {x, y};
	SetPosC(pos);
}
void HideConsoleCursor() {
	CONSOLE_CURSOR_INFO cursor_info = {1, 0};
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursor_info);
}
