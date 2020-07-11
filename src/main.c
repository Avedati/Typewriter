// https://www.viget.com/articles/game-programming-in-c-with-the-ncurses-library/
#include <ctype.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define DELAY 30000

// https://stackoverflow.com/questions/3437404/min-and-max-in-c
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) < (Y)) ? (Y) : (X))

struct cursorPos { int x, y; };

char* buffer;
struct cursorPos* cursor;

int cursorIndex(void) {
	int indexX = 0;
	int indexY = 0;
	for(int i=0;i<strlen(buffer);i++) {
		if(buffer[i] == '\n') {
			indexX = 0;
			indexY++;
		}
		else if(indexY == cursor->y) {
			indexX++;
		}
		if(indexX == cursor->x && indexY == cursor->y) { return i; }
	}
	return -1;
}

struct cursorPos cursorPosition(int cursorIndex) {
	struct cursorPos result;
	result.x = 0;
	result.y = 0;
	for(int i=0;i<strlen(buffer);i++) {
		if(i == cursorIndex) {
			return result;
		}
		else if(buffer[i] == '\n') {
			result.x = 0;
			result.y++;
		}
		else {
			result.x++;
		}
	}
	result.x = -1;
	result.y = -1;
	return result;
}

void insertChar(int index, char c) {
	index++;
	int buff_len = strlen(buffer);
	buffer = realloc(buffer, (buff_len + 2) * sizeof(char));
	for(int i=buff_len-1;i>=index;i--) {
		buffer[i+1] = buffer[i];
	}
	buffer[index] = c;
	buffer[buff_len+1] = 0;
}

int lineLen(void) {
	int x = 0, y = 0;
	for(int i=0;i<strlen(buffer);i++) {
		if(buffer[i] == '\n') {
			if(y == cursor->y) { return x; }
			y++;
		}
		else if(y == cursor->y) {
			x++;
		}
	}
	return x;
}

int numLines(void) {
	int lines = 1;
	for(int i=0;i<strlen(buffer);i++) {
		if(buffer[i] == '\n') { lines++; }
	}
	return lines;
}

int main(int argc, char** argv) {
	cursor = malloc(sizeof(struct cursorPos*));
	cursor->x = 3;
	cursor->y = 0;

	initscr();
	keypad(stdscr, TRUE); // https://stackoverflow.com/questions/43923546/proper-way-of-catching-controlkey-in-ncurses
	noecho();
	move(cursor->y, cursor->x);

	int screenW = 0, screenH = 0;
	getmaxyx(stdscr, screenH, screenW);

	char statusSeparator[screenW+1];
	for(int i=0;i<screenW;i++) { statusSeparator[i] = '='; }
	statusSeparator[screenW] = 0;

	char* statusLine = malloc(sizeof(char) * 23);
	strcpy(statusLine, "Untitled, mode: insert");
	statusLine[22] = 0;

	char mode = 'i';
	char* filename = malloc(sizeof(char) * 9);
	strcpy(filename, "Untitled");
	filename[8] = 0;
	int fileOpen = 1;

	FILE* fp = fopen(filename, "r+");
	if(fp) {
		fileOpen = 1;
		// https://stackoverflow.com/questions/14002954/c-programming-how-to-read-the-whole-file-contents-into-a-buffer
		fseek(fp, 0, SEEK_END);
		long fsize = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		buffer = malloc((fsize + 1) * sizeof(char));
		fread(buffer, 1, fsize, fp);
		buffer[fsize] = 0;
		fclose(fp);
	}
	else {
		buffer = malloc(4 * sizeof(char));
		strcpy(buffer, "...");
		buffer[3] = 0;
	}

	int justSaved = 0;

	char* backupBuffer = NULL;
	struct cursorPos* backupCursor = (struct cursorPos*)malloc(sizeof(struct cursorPos*));

	int running = 1;
	while(running) {
		clear();
		mvprintw(0, 0, buffer);
		mvprintw(screenH - 2, 0, statusSeparator);
		mvprintw(screenH - 1, 1, statusLine);
		move(cursor->y, cursor->x);
		int ch = getch();
		refresh();
		switch(ch) {
			case 127: // delete
				if(mode == 'i' || mode == 'o') {
					if(strlen(buffer) > 0) {
						if(--(cursor->x) < 0) {
							cursor->x = 0;
							if(cursor->y > 0) {
								(cursor->y)--;
								cursor->x = lineLen();
							}
						}
						move(cursor->y, cursor->x);
						int buff_length = strlen(buffer);
						int index = cursorIndex();
						for(int i=index+1;i<buff_length;i++) {
							buffer[i] = buffer[i+1];
						}
						buffer = realloc(buffer, buff_length * sizeof(char));
						buffer[buff_length - 1] = 0;
					}
					if(mode == 'i' && justSaved) {
						memset(statusLine, 0, strlen(statusLine));
						statusLine = realloc(statusLine, (strlen(filename) + 15) * sizeof(char));
						strcat(statusLine, filename);
						strcat(statusLine, ", mode: insert");
						statusLine[14 + strlen(filename)] = 0;
					}
				}
				break;
			case '\t':
				if(mode == 'i' || mode == 'o') {
					insertChar(cursorIndex(), ' ');
					move(cursor->y, ++(cursor->x));
					insertChar(cursorIndex(), ' ');
					move(cursor->y, ++(cursor->x));
					if(mode == 'i' && justSaved) {
						memset(statusLine, 0, strlen(statusLine));
						statusLine = realloc(statusLine, (strlen(filename) + 15) * sizeof(char));
						strcat(statusLine, filename);
						strcat(statusLine, ", mode: insert");
						statusLine[14 + strlen(filename)] = 0;
					}
				}
				break;
			case 27: // escape
				if(mode == 'i') {
					mode = 'c';
					// https://stackoverflow.com/questions/8107826/proper-way-to-empty-a-c-string/8107851#:~:text=buffer%5B0%5D%20%3D%20'%5C,to%20the%20first%20NULL%20character.
					memset(statusLine, 0, strlen(statusLine));
					statusLine = realloc(statusLine, (strlen(filename) + 16) * sizeof(char));
					strcat(statusLine, filename);
					strcat(statusLine, ", mode: command");
					statusLine[15 + strlen(filename)] = 0;
				}
				break;
			case 260: // left arrow
				if(mode == 'i' || mode == 'o') {
					cursor->x = MAX(cursor->x - 1, 0);
				}
				break;
			case 261: // right arrow
				if(mode == 'i' || mode == 'o') {
					cursor->x = MIN(cursor->x + 1, lineLen());
				}
				break;
			case 259: // up arrow
				if(mode == 'i') {
					cursor->y = MAX(cursor->y - 1, 0);
					cursor->x = MIN(cursor->x, lineLen());
				}
				break;
			case 258: // down arrow
				if(mode == 'i') {
					cursor->y = MIN(cursor->y + 1, numLines() - 1);
					cursor->x = MIN(cursor->x, lineLen());
				}
				break;
			case 10: // return
				if(mode == 'i') {
					insertChar(cursorIndex(), '\n');
					cursor->x = 0;
					(cursor->y)++;
					if(justSaved) {
						memset(statusLine, 0, strlen(statusLine));
						statusLine = realloc(statusLine, (strlen(filename) + 15) * sizeof(char));
						strcat(statusLine, filename);
						strcat(statusLine, ", mode: insert");
						statusLine[14 + strlen(filename)] = 0;
					}
				}
				else if(mode == 'o') {
					// https://www.programiz.com/c-programming/c-file-input-output
					FILE* fp = fopen(buffer, "r+");
					if(fp) {
						filename = (char*)malloc((strlen(buffer) + 1) * sizeof(char));
						strcpy(filename, buffer);
						filename[strlen(buffer)] = 0;
						fileOpen = 1;
						// https://stackoverflow.com/questions/14002954/c-programming-how-to-read-the-whole-file-contents-into-a-buffer
						fseek(fp, 0, SEEK_END);
						long fsize = ftell(fp);
						fseek(fp, 0, SEEK_SET);
						buffer = realloc(buffer, (fsize + 1) * sizeof(char));
						fread(buffer, 1, fsize, fp);
						buffer[fsize] = 0;
						fclose(fp);
						mode = 'i';
						memset(statusLine, 0, strlen(statusLine));
						statusLine = realloc(statusLine, (strlen(filename) + 15) * sizeof(char));
						strcat(statusLine, filename);
						strcat(statusLine, ", mode: insert");
						statusLine[14 + strlen(filename)] = 0;
					}
				}
				break;
			default:
				// TODO: make sure that only certain characters can be added.
				if(mode == 'c') {
					if(ch == 'i') {
						mode = 'i';
						memset(statusLine, 0, strlen(statusLine));
						statusLine = realloc(statusLine, (strlen(filename) + 15) * sizeof(char));
						strcat(statusLine, filename);
						strcat(statusLine, ", mode: insert");
						statusLine[14 + strlen(filename)] = 0;
					}
					else if(ch == 'o') {
						if(fileOpen) {
							// https://stackoverflow.com/questions/50451446/how-can-i-delete-all-the-text-from-text-file-in-c
							FILE* fp = fopen(filename, "w");
							if(fp) {
								fprintf(fp, "%s", buffer);
								fclose(fp);
							}
						}
						mode = 'o';
						memset(statusLine, 0, strlen(statusLine));
						statusLine = realloc(statusLine, (strlen(filename) + 13) * sizeof(char));
						strcat(statusLine, filename);
						strcpy(statusLine, ", mode: open");
						statusLine[12 + strlen(filename)] = 0;
						backupBuffer = realloc(backupBuffer, (strlen(buffer) + 1) * sizeof(char));
						strcpy(backupBuffer, buffer);
						backupBuffer[strlen(backupBuffer)] = 0;
						memset(buffer, 0, strlen(buffer));
						backupCursor->x = cursor->x;
						backupCursor->y = cursor->y;
						cursor->x = 0;
						cursor->y = 0;
					}
					else if(ch == 'w') {
						if(fileOpen) {
							FILE* fp = fopen(filename, "w");
							if(fp) {
								fprintf(fp, "%s", buffer);
								fclose(fp);
							}
						}
						mode = 'i';
						memset(statusLine, 0, strlen(statusLine));
						statusLine = realloc(statusLine, (strlen(filename) + 22) * sizeof(char));
						strcat(statusLine, filename);
						strcat(statusLine, ", mode: insert, saved");
						statusLine[21 + strlen(filename)] = 0;
						justSaved = 1;
					}
					else if(ch == 'q') {
						if(fileOpen) {
							FILE* fp = fopen(filename, "w");
							if(fp) {
								fprintf(fp, "%s", buffer);
								fclose(fp);
							}
						}
						running = 0;
					}
				}
				else if(mode == 'i' || mode == 'o') {
					insertChar(cursorIndex(), ch);
					move(cursor->y, ++(cursor->x));
					if(mode == 'i' && justSaved) {
						memset(statusLine, 0, strlen(statusLine));
						statusLine = realloc(statusLine, (strlen(filename) + 15) * sizeof(char));
						strcat(statusLine, filename);
						strcat(statusLine, ", mode: insert");
						statusLine[14 + strlen(filename)] = 0;
					}
				}
				break;
		}
	}
	endwin();
	free(buffer);
	free(cursor);
	return 0;
}
