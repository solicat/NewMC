#include <stdio.h>
#include <string.h>
#include <termios.h>
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

int main(int argc, char* argv[])
{
	initscr();
	clear();

	int c;
	FILE* fp;
	
	fp = popen("pwd", "r");
	fgets(pwd, BUFSIZ, fp);
	pclose(fp);

	print_pwd();
	load_ls();
	print_ls();	

	getch();
	
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
		data[count].check = 0;
		strcpy(data[count].name, buf);
		count++;
	}

	pclose(fp);
}

void print_ls()
{
	for(int i = 0; i < count; i++)
	{
		move(LS + i, 5);
		addstr(data[i].check);
		addstr(data[i].name);
	}
	move(LS, 0);
	refresh();
}
