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
	long size;
	char* mod_time;
}item;

char pwd[BUFSIZ - 10];
char buf[BUFSIZ];
int count;
int page;
int cur_row;
int cur_page;
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
void _search();

int main(int argc, char* argv[])
{
	signal(SIGINT, SIG_IGN); signal(SIGQUIT, SIG_IGN); signal(SIGTSTP, SIG_IGN);
	initscr();

	crmode();
	keypad(stdscr, TRUE);
	noecho();
	
	clear();

	start_color();
	
	init_color(COLOR_BLACK, 180, 200, 300);
	init_color(COLOR_YELLOW, 1000, 1000, 0);
	init_pair(SELECT, COLOR_YELLOW, COLOR_BLACK);

	int c; cur_row = 0; cur_page = 0;

	getcwd(pwd, BUFSIZ-10);
	load_ls();

	while(1)
	{	
		clear();
		print_pwd();
		print_ls();
		print_menu();

		switch(c = getch())
		{
			case KEY_DOWN: if(cur_page < page){
						if(cur_row+1<15) cur_row++; 
						else {cur_row=0; cur_page++;}
					}
					else if(cur_page == page && cur_page*15 + cur_row+1<count) cur_row++;
					break;

			case KEY_UP: if(cur_row>0)cur_row--; else if(cur_page > 0){cur_row=14; cur_page--;} break;

			case KEY_PPAGE: if(cur_page > 0){
						cur_page--;
					}
					else cur_row = 0;
					break;

			case KEY_NPAGE: if(cur_page < page){
						cur_page++;
						if(cur_row > count%15 - 1)
							cur_row = count%15 - 1;
					}
					else cur_row = count%15 - 1;
					break;

			case '\n': if(isadir(ch_fname(data[cur_page*15 + cur_row].name))){
					//if directory. chdir.
					chdir(ch_fname(data[cur_page*15 + cur_row].name));
					getcwd(pwd, BUFSIZ-10);
					load_ls();
					cur_row = 0; cur_page = 0;
					}
					break;

			case KEY_F(5):	_cp();
					load_ls();
					print_ls();
					break;		//file copy

			case KEY_F(6):	_mv();
					load_ls();
					print_ls();
					break;		//file mv

			case KEY_F(7):	_mkdir();
					load_ls();
					print_ls();
					break;		//mkdir

			case KEY_F(8):	_rm();
					load_ls();
					print_ls();
					break;		//file delete

			case KEY_F(9):	endwin();
					return 1;	//quit

			case '/' : _search();           //search
				   break;

			case KEY_HOME:	cur_row = 0; 
					break;		//cur move top

			case KEY_END:	if(cur_page == page) cur_row = count%15 - 1;
					else cur_row = 14;
					break;		//cur move bottom
			case KEY_IC:	if(cur_page*15 + cur_row != 0)
						data[cur_page*15 + cur_row].check *= -1;
					if((cur_page < page && cur_row+1<15) || (cur_page == page && cur_page*15 + cur_row +1<count))
						cur_row++;
					else if(cur_page < page && cur_row == 14)
					{
						cur_page++;
						cur_row = 0;
					}
					break;
			default: break;
		}
	}
	endwin();
}

void print_pwd()
{	
	clear();
	move(PWD-1, 5);
	printw("[[ Current Directory ]]");
	move(PWD, 5);
	standout();
	addstr(pwd);
	standend();
	move(LINES-1, 0);
	refresh();	
}

int static compare(const void* first, const void* second)
{
	return strcmp((char*)((struct item*)first)->name, (char*)((struct item*)second)->name);
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
	page = count/15;
	pclose(fp);
	/*DIR* dir_ptr;
	struct dirent* direntp;
	struct stat info;

	count = 0;

	if((dir_ptr = opendir(pwd)) == NULL)
		exit(1);
	else
	{
		while((direntp = readdir(dir_ptr)) != NULL)
		{
			data[count].check = 1;
			strcpy(data[count].name, direntp->d_name);
			stat(direntp->d_name, &info);
			data[count].size = (long)((struct stat)info).st_size;
//			strcpy(data[count].mod_time, 4+ctime((struct stat)info).st_mtime);
			count++;
		}
		page = count/15;
	}
	closedir(dir_ptr);

	qsort(data, count, sizeof(item), compare);

	for(int i = 0; i < count - 1; i++)
		data[i] = data[i + 1];
	count -= 1;*/
}

void print_ls()
{
	move(LINES-3, 0);
	attron(A_UNDERLINE);
	printw("%d Files in this directory               Page (%d / %d)", count - 1, cur_page+1, page+1);
	attroff(A_UNDERLINE);
	move(LS-1, 5);
	//attron(COLOR_PAIR(SELECT));
	printw("[[ File List ]]");
	//printw(".n%29s%31s", "Name", "Size");
	//attroff(COLOR_PAIR(SELECT));
	if(cur_page < page)
	{
		for(int i = cur_page*15; i < (cur_page+1)*15; i++)
		{
			move(LS + i%15, 5);
			if(i == (cur_row + cur_page*15))
			{
				move(LS + i%15, 2);
				printw(">> ");
			}
			if(data[i].check == -1)
				attron(COLOR_PAIR(SELECT));
			printw("%s", data[i].name);			
			//printw("%-55s%6ld", data[i].name, data[i].size);
			if(data[i].check == -1)
				attroff(COLOR_PAIR(SELECT));
		}
	}
	else
	{
		for(int i = page*15; i < count; i++)
		{
			move(LS + i%15, 5);
			if(i == (cur_row + page*15))
			{
				move(LS + i%15, 2);
				printw(">> ");
			}
			if(data[i].check == -1)
				attron(COLOR_PAIR(SELECT));
			printw("%s", data[i].name);
			//printw("%-55s%6ld", data[i].name, data[i].size);
			if(data[i].check == -1)
				attroff(COLOR_PAIR(SELECT));
		}
	}
	move(LINES-1, COLS-1);//just in case
	curs_set(0);
	refresh();
}

void print_menu()
{
	char* menu[10] = {"Help", "Menu", "View", "Edit", "Copy", "Ren/Mov", "Mkdir", "Delete", "Quit"};
	move(LINES-2, 0);
	for(int i = 0; i < 9; i++)
	{
		printw("F%-2d %-8s", i + 1, menu[i]);
		if(i==4) move(LINES-1, 0);
	}
	move(LINES-1, COLS-1);
}

char* ch_fname(char* f)
{
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

int isadir(char* f)
{
	struct stat info;
	return (stat(f,&info)!=-1 && S_ISDIR(info.st_mode));
}

void input_set(int b)//1 is set, 0 is unset
{
	move(LINES-7, 0);
	clrtobot();//wipe
	curs_set(b); 
	if(b)
	{
		echo(); 
		refresh();
	}
	else
	{
		noecho();
	}
}

void child_waiter(int signum)
{
	while(waitpid(-1, NULL, WNOHANG)>0);
}

void _cp()
{
	char sourcefile[MAX][BUFSIZ], targetfile[MAX][BUFSIZ], buf[BUFSIZ];
	int pid, num=0;
	signal(SIGCHLD, child_waiter);

	//input targetfile names
	for(int i =0; i<count; i++){
		if(data[i].check == -1){
			strcpy(sourcefile[num], ch_fname(data[i].name));
			input_set(1);
			printw("('quit' to return)   input filename : cp %s : ", sourcefile[num]);
			scanw("%s", buf);
			if(strcmp(buf, "quit")==0) return;
			strcpy(targetfile[num++], buf);
		}
	}
	input_set(0);

	//exec cp source target (num is checking num)
	for(int i=0; i<num; i++)
	{
		if((pid=fork())==-1) return;
		if(pid ==0)
		{
			execlp("cp", "cp", sourcefile[i], targetfile[i], NULL);
			exit(1);
		}
	}

	while(waitpid(-1, NULL, WNOHANG) == 0);
}

void _mv()
{
	char buf[BUFSIZ];
	int pid, num=0;
	int mv_stat = 0;//0 : mv file1 file2     1: mv file1 dir1
	signal(SIGCHLD, child_waiter);

	input_set(1);
	printw("selecet mode : 1. rename  2. move directory  3. return >> ");
	do
	{
		scanw("%d ", &mv_stat);
		if(mv_stat == 3) return;
		if(mv_stat !=1 && mv_stat != 2)
			printw("please input 1 or 2 or 3 >> ");
		else break;
	}
	while(1);

	if(mv_stat == 1)	//rename files selected
	{
		char sourcefile[MAX][BUFSIZ], targetfile[MAX][BUFSIZ];

		//input targetfile names
		for(int i =0; i<count; i++)
		{
			if(data[i].check == -1)
			{
				strcpy(sourcefile[num], ch_fname(data[i].name));
				input_set(1);
				printw("('quit' to return)   input filename : mv %s : ", sourcefile[num]);
				scanw("%s", buf);
				if(strcmp(buf, "quit")==0) return;
				strcpy(targetfile[num++], buf);
			}
		}

		input_set(0);	

		//exec cp source target (num is checking num)
		for(int i=0; i<num; i++)
		{
			if((pid=fork())==-1) return;
			if(pid ==0)
			{
				execlp("mv", "mv", sourcefile[i], targetfile[i], NULL);
				exit(1);
			}
		}
	}
	else	//move on another directory
	{
		char *arglist[MAX];

		input_set(1);
		addstr("('quit' to return)   input dirname: ");
		scanw("%s", buf);
		if(strcmp(buf, "quit")==0) return;

		arglist[num++] = "mv";

		//mv file1 file2 file3... dir1
		for(int i=0; i<count; i++)
		{
			if(data[i].check == -1)
				arglist[num++] = ch_fname(data[i].name);
		}
	
		arglist[num++] = buf;
		arglist[num] = 0;
		input_set(0);

		if((pid=fork())==-1) return;
		if(pid ==0)
		{
			execvp(arglist[0], arglist);
			exit(1);
		}
	}

	while(waitpid(-1, NULL, WNOHANG) == 0);
}

void _mkdir(){
	char *arglist[MAX], buf[BUFSIZ];
	int pid, len=1;
	signal(SIGCHLD, child_waiter);

	input_set(1);
	addstr("('quit' to return)   input dirname : ");

	arglist[0] = "mkdir";
	scanw("%s", buf);
	if(strcmp(buf, "quit") == 0) return;
	arglist[len++] = buf;
	arglist[len] = 0;
	input_set(0);

	if((pid=fork())==-1) return;
	if(pid ==0)
	{
		execvp(arglist[0], arglist);
		exit(1);
	}
}

void _rm(){
	char *arglist[MAX];
	arglist[0] = "rm";
	int pid, len=1;
	signal(SIGCHLD, child_waiter);

	//rm file1 file2 file3...
	for(int i=0; i<count; i++)
	{
		if(data[i].check == -1)
			arglist[len++] = ch_fname(data[i].name);
	}

	arglist[len] = 0;

	if((pid=fork())==-1) return;
	if(pid ==0)
	{
		execvp(arglist[0], arglist);
		exit(1);
	}
}

void _search(){
	char c;
	char buf[BUFSIZ];
	int i;
	input_set(1);
	addch('/');
	strcpy(buf, "./");
	scanw("%s", buf+2);
	if(strcmp(buf, "./..") == 0) return;
	for(i = 0; i<count; i++){
		if(!strcmp(ch_fname(data[i].name), buf)) {
			data[i].check *=-1;
			cur_row = i%15;
			break;
		}
	}
	input_set(0);
	if(i == count){
	 	printw("cannot found");
		while((c=getch())!='\n');
	}
}
