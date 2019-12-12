#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <curses.h>

#define PWD 2
#define LS 4

struct item{
	int check;
	char name[BUFSIZ];
}item;

char pwd[BUFSIZ - 10];
char buf[BUFSIZ];
int count;
int cur_row;
struct item data[BUFSIZ];

void print_pwd();
void load_ls();
void print_ls();
void cd();

int main(int argc, char* argv[])
{
	initscr();

    	crmode();
	keypad(stdscr, TRUE);
   	noecho();

	clear();

	int c; cur_row = 0;
	FILE* fp; struct stat sbuf;
	
	fp = popen("pwd", "r");
	fgets(pwd, BUFSIZ, fp);


	load_ls();
	
	while(1)
	{	
		clear();
		print_pwd();
		print_ls();

		switch(c = getch())
		{
			case KEY_DOWN: if(cur_row+1<count)cur_row++; break;
			case KEY_UP: if(cur_row>0)cur_row--; break;
			case '\n': data[cur_row].check *= -1;
			default: break;
		}
	}

	pclose(fp);
	endwin();
}

void print_pwd()
{	
	clear();
	move(PWD, 5);
	standout();
	addstr(pwd);
	standend();
	move(LINES-1, 0);
	refresh();	
}

void load_ls()
{
	FILE* fp;
	char command[BUFSIZ] = "ls -al ";
	strcat(command, pwd);

	count = 0;

	fp = popen(command, "r");
	fgets(buf, BUFSIZ, fp);	// info
	fgets(buf, BUFSIZ, fp);	// ./
	while(fgets(buf, BUFSIZ, fp))
	{
		data[count].check = 1;
		strcpy(data[count].name, buf);
		count++;
	}

	pclose(fp);
}

void print_ls()
{
	printw("%d Files in this directory", count);
	for(int i = 0; i < count; i++)
	{
		move(LS + i, 5);
		if(i == cur_row)
		{
			move(LS + i, 2);
			printw(">> ");
		}
		if(data[i].check == 1)	printw("( ) | ");
		else printw("(V) | ");
		printw("%s", data[i].name);
	}
	move(LS, 0);
	refresh();
}
