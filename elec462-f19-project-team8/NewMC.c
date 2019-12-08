#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <curses.h>

#define PWD 2
#define LS 4

struct item{
	int check;
	char name[BUFSIZ - 10];
}item;

char pwd[BUFSIZ - 10];
char buf[BUFSIZ];
int count;
int cur_row;
struct item data[BUFSIZ];

char get_key();
void print_pwd();
void load_ls();
void print_ls();

int main(int argc, char* argv[])
{
	initscr();
	clear();

	FILE* fp;
	int temp;
	
	fp = popen("pwd", "r");
	fgets(pwd, BUFSIZ, fp);
	pclose(fp);

	print_pwd();
	load_ls();
	print_ls();

	while(1)
	{
		temp = get_key();
		if(temp == '\33')
		{
			get_key();
			temp = get_key();
			if(temp == 'A')		// Up Arrow
			{
				if(cur_row > 0)
					move(--cur_row + LS, 0);
				refresh();
			}
			else if(temp == 'B')	// Down Arrow
			{
				if(cur_row < count - 1)
					move(++cur_row + LS, 0);
				refresh();
			}
		}
	}	
	endwin();
}

char get_key()
{
	struct termios oldt, newt;
	char ch;
	tcgetattr(0, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON|ECHO);
	tcsetattr(0, TCSANOW, &newt);
	ch = getchar();
	tcsetattr(0, TCSANOW, &oldt);
	return ch;
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
	char temp[BUFSIZ];
	for(int i = 0; i < count; i++)
	{
		move(LS + i, 5);
		sprintf(temp, "%d %s", data[i].check, data[i].name);
		addstr(temp);
		bzero(temp, BUFSIZ);
	}
	move(LS, 0);
	refresh();
	cur_row = 0;
}
