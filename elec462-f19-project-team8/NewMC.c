#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <curses.h>

#define MAX 100 //number of check
#define PWD 2
#define LS 4

#define SELECT 1

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
void print_menu();
void cd();
char* ch_fname(char*);
int isadir(char*);
void input_set();
void child_waiter(int);
void _cp();
void _mv();
void _mkdir();
void _rm();

int main(int argc, char* argv[])
{
	initscr();

	crmode();
	keypad(stdscr, TRUE);
	noecho();
	
	clear();

	start_color();
	
	init_color(COLOR_BLACK, 200, 200, 500);
	init_color(COLOR_YELLOW, 1000, 1000, 0);
	init_pair(SELECT, COLOR_YELLOW, COLOR_BLACK);

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
		print_menu();

		switch(c = getch())
		{
			case KEY_DOWN: if(cur_row+1<count)cur_row++; break;
			case KEY_UP: if(cur_row>0)cur_row--; break;
			case '\n': 
					     if(isadir(ch_fname(data[cur_row].name))){
						     //if directory. chdir.
						     chdir(ch_fname(data[cur_row].name));
						     getcwd(pwd, BUFSIZ-10);
						     load_ls();
						     cur_row = 0;
					     }
					     break;
			case KEY_F(5):_cp(); load_ls(); print_ls(); break;//file copy
			case KEY_F(6):_mv(); load_ls(); print_ls(); break;//file mv
			case KEY_F(7):_mkdir(); load_ls(); print_ls(); break;//mkdir
			case KEY_F(8):_rm(); load_ls(); print_ls(); break;//file delete
			case KEY_HOME: cur_row = 0; break;
			case KEY_END:  cur_row = count - 1; break;
			case KEY_IC: if(cur_row != 0)data[cur_row].check *= -1; if(cur_row+1<count)cur_row++; break;
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
	move(LINES-2, 0);
	printw("%d Files in this directory", count - 1);
	for(int i = 0; i < count; i++)
	{
		move(LS + i, 5);
		if(i == cur_row)
		{
			move(LS + i, 2);
			printw(">> ");
		}
		if(data[i].check == -1)
			attron(COLOR_PAIR(SELECT));
		printw("%s", data[i].name);
		if(data[i].check == -1)
			attroff(COLOR_PAIR(SELECT));
	}
	move(LINES-1, COLS-1);//just in case
	curs_set(0);
	refresh();
}

void print_menu()
{
	char* menu[10] = {"Help", "Menu", "View", "Edit", "Copy", "RenMov", "Mkdir", "Delete", "PullDn", "Quit"};
	move(LINES-1, 0);
	for(int i = 0; i < 10; i++)
		printw("%-2d%-8s", i + 1, menu[i]);
	move(LINES-1, COLS-1);
}

char* ch_fname(char* f){
	char fname[30], *name, *filename;
	//filename extraction
	strcpy(fname, "./");
	name = strrchr(f, ' ')+1;
	strcat(fname, name);
	fname[strlen(fname)-1]='\0';//remove \r\n

	filename = (char*)malloc(sizeof(fname));
	strcpy(filename, fname);

	return filename;
}

int isadir(char* f){
	struct stat info;
	return (stat(f,&info)!=-1 && S_ISDIR(info.st_mode));
}

void input_set(int b){//1 is set, 0 is unset
	if(b){
		move(count+7, 3);
		clrtobot();//wipe
		curs_set(b); 
		echo(); 
		refresh();
	}
	else{
		curs_set(b);
		noecho();
	}
}

void child_waiter(int signum){
	while(waitpid(-1, NULL, WNOHANG)>0);
}

void _cp(){
	char sourcefile[MAX][BUFSIZ], targetfile[MAX][BUFSIZ], buf[BUFSIZ];
	int pid, num=0;
	signal(SIGCHLD, child_waiter);

	//input targetfile names
	for(int i =0; i<count; i++){
		if(data[i].check == -1){
			strcpy(sourcefile[num], ch_fname(data[i].name));
			input_set(1);
			printw("input filename : cp %s ", sourcefile[num]);
			scanw("%s", buf);
			strcpy(targetfile[num++], buf);
		}
	}
	input_set(0);

	//exec cp source target (num is checking num)
	for(int i=0; i<num; i++){
		if((pid=fork())==-1) return;
		if(pid ==0){
			execlp("cp", "cp", sourcefile[i], targetfile[i], NULL);
			exit(1);
		}
	}
}

void _mv(){
	char buf[BUFSIZ];
	int pid, num=0;
	int mv_stat = 0;//0 : mv file1 file2     1: mv file1 dir1
	signal(SIGCHLD, child_waiter);

	input_set(1);
	printw("selecet mode : 1. rename  2. move directory >> ");
	do{
		scanw("%d ", &mv_stat);
		if(mv_stat !=1 && mv_stat != 2)
			printw("please input 1 or 2");
		else break;
	}while(1);

	if(mv_stat == 1){//rename files selected
		char sourcefile[MAX][BUFSIZ], targetfile[MAX][BUFSIZ];
		//input targetfile names
		for(int i =0; i<count; i++){
			if(data[i].check == -1){
				strcpy(sourcefile[num], ch_fname(data[i].name));
				input_set(1);
				printw("input filename : mv %s ", sourcefile[num]);
				scanw("%s", buf);
				strcpy(targetfile[num++], buf);
			}
		}
		input_set(0);	

		//exec cp source target (num is checking num)
		for(int i=0; i<num; i++){
			if((pid=fork())==-1) return;
			if(pid ==0){
				execlp("mv", "mv", sourcefile[i], targetfile[i], NULL);
				exit(1);
			}
		}
	}
	else{//move on another directory
		char *arglist[MAX];

		input_set(1);
		addstr("input dirname: ");
		scanw("%s", buf);

		arglist[num++] = "mv";
		for(int i=0; i<count; i++){//mv file1 file2 file3... dir1
			if(data[i].check == -1)
				arglist[num++] = ch_fname(data[i].name);
		}
		arglist[num++] = buf;
		arglist[num] = 0;
		input_set(0);

		if((pid=fork())==-1) return;
		if(pid ==0){
			execvp(arglist[0], arglist);
			exit(1);
		}
	}
}

void _mkdir(){
	char *arglist[MAX], buf[BUFSIZ];
	int pid, len=1;
	signal(SIGCHLD, child_waiter);

	input_set(1);
	addstr("input dirname: ");

	arglist[0] = "mkdir";
	scanw("%s", buf);
	arglist[len++] = buf;
	arglist[len] = 0;
	input_set(0);

	if((pid=fork())==-1) return;
	if(pid ==0){
		execvp(arglist[0], arglist);
		exit(1);
	}
}

void _rm(){
	char *arglist[MAX];
	arglist[0] = "rm";
	int pid, len=1;
	signal(SIGCHLD, child_waiter);
	for(int i=0; i<count; i++){//rm file1 file2 file3...
		if(data[i].check == -1)
			arglist[len++] = ch_fname(data[i].name);
	}
	arglist[len] = 0;

	if((pid=fork())==-1) return;
	if(pid ==0){
		execvp(arglist[0], arglist);
		exit(1);
	}
}
