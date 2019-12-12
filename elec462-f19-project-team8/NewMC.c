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
char* ch_fname(char*);
int isadir(char*);
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
			case '\n': 
				if(isadir(ch_fname(data[cur_row].name))){
					//if directory. chdir.
					//chdir(ch_fname(data[cur_row].name));
					//print_pwd();
					//load_ls();
				}
				else
					data[cur_row].check *= -1;
				break;
			case KEY_F(5):_cp(); load_ls(); break;//file copy
			case KEY_F(6):_mv(); load_ls(); break;//file mv
			case KEY_F(7):_mkdir(); load_ls(); break;//mkdir
			case KEY_F(8):_rm(); load_ls(); break;//file delete
			case KEY_HOME: cur_row = 0; break;
			case KEY_END:  cur_row = count-1; break;
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
		if(isadir(ch_fname(data[i].name))) printw("    | ");
		else{
			if(data[i].check == 1) printw("( ) | ");
			else printw("(V) | ");
		}
		printw("%s", data[i].name);
	}
	move(LINES-1, COLS-1);//just in case...
	curs_set(0);
	refresh();
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

void child_waiter(int signum){
	while(waitpid(-1, NULL, WNOHANG)>0);
}

void _cp(){
	char *sourcefile, num[10], targetfile[30] = "target";//input...
	int pid;
	signal(SIGCHLD, child_waiter);
	for(int i=0; i<count; i++){
		if(data[i].check == -1){//cp source target
			data[i].check *=-1;
			sourcefile = ch_fname(data[i].name);
			sprintf(num, "%d", i);
			strcat(targetfile, num);

			if((pid=fork())==-1) return;
			if(pid ==0){
			execlp("cp", "cp", sourcefile, targetfile, NULL);
			exit(1);
			}
		}
	}
}

void _mv(){
/*	char *source, num[10], target[30] = "mv";//input...
	int pid;
	signal(SIGCHLD, child_waiter);
	for(int i=0; i<count; i++){
		if(data[i].check == -1){//mv file1 file2 || mv file1 dir1
			data[i].check *=-1;
			sourcefile = ch_fname(data[i].name);
			sprintf(num, "%d", i);
			strcat(targetfile, num);

			if((pid=fork())==-1) return;
			if(pid ==0){
				if(isadir())
			execlp("mv", "mv", sourcefile, targetfile, NULL);
			exit(1);
			}
		}
	}
*/
}

void _mkdir(){
	char *arglist[100], buf[BUFSIZ];
	int pid, len=1;
	signal(SIGCHLD, child_waiter);
	move(count+7, 3);
       	addstr("input dirname: ");
	curs_set(1); 
	echo(); 
	refresh();

	arglist[0] = "mkdir";
	scanw("%s", buf);
	arglist[len++] = buf;
	arglist[len] = 0;
	curs_set(0);
	noecho();

	if((pid=fork())==-1) return;
	if(pid ==0){
		execvp(arglist[0], arglist);
		exit(1);
	}
}

void _rm(){
	char *arglist[100];
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
