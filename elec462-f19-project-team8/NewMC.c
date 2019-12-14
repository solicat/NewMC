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
#include <time.h>

#define MAX 100 //number of check
#define PWD 2
#define LS 4

#define SELECT 1

struct item{
	int check;
	char name[BUFSIZ];
	long size;
	char mod_time[26];
}item;

char pwd[BUFSIZ - 10];
char buf[BUFSIZ];
int fcount, dcount, count;
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
void help_mess(WINDOW* win, int* , int, char* m);
void _help();
void _cat();
void cat_title(int*, char*);

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
						if(cur_page == page)
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

			case KEY_F(1) : _help();
					break;

			case KEY_F(2) : _cat();
					load_ls();
					break;

			case KEY_F(3) : _search();           
				   	break;		//search

			case KEY_F(4) : if(cur_page*15 + cur_row == 0) continue;
					if(data[cur_page*15 + cur_row].check == 1)
					{
						for(int i = 1; i < count ; i++)
							data[i].check = -1;
					}
					else
					{
						for(int i = 1; i < count ; i++)
							data[i].check = 1;
					}
				   	break;		//(dis)select all files

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
					cur_row = 0; cur_page = 0;
					print_ls();
					break;		//file delete

			case KEY_F(9):	endwin();
					return 1;	//quit

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
	/*
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
	pclose(fp);*/
	DIR* dir_ptr;
	struct dirent* direntp;
	struct stat info;
	
	fcount = dcount = count = 0;

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
			strcpy(data[count].mod_time, ctime(&(info.st_mtime)));
			if(S_ISDIR(info.st_mode)) dcount++;
			else fcount++;
			count++;
		}
	}
	closedir(dir_ptr);

	qsort(data, count, sizeof(item), compare);

	for(int i = 0; i < count - 1; i++)
		data[i] = data[i + 1];
	count -= 1;
	page = count/15;
}

void print_ls()
{
	move(LINES-3, 0);
	attron(A_UNDERLINE);
	if(count-1 == 0)
		printw("No Directories and Files in this directory                      ");
	else if(dcount - 2 == 0)
		printw("%2d Files in this directory                          Page (%d / %d)", fcount, cur_page+1, page+1);
	else if(fcount == 0)
		printw("%2d Directories in this directory                    Page (%d / %d)", dcount - 2, cur_page+1, page+1);
	else
		printw("%2d Directories and %2d Files in this directory       Page (%d / %d)", dcount - 2, fcount, cur_page+1, page+1);
	attroff(A_UNDERLINE);
	move(LS-1, 5);
	attron(COLOR_PAIR(SELECT));

	printw(".n%8s%34s%16s", "Name", "Size", "Modify time");
	attroff(COLOR_PAIR(SELECT));
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
			//printw("%s", data[i].name);
			if(isadir(data[i].name)) printw("/");
			else printw(" ");		
			printw("%-35s%8ld%30s", data[i].name, data[i].size, data[i].mod_time);
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
			//printw("%s", data[i].name);
			if(isadir(data[i].name)) printw("/");
			else printw(" ");
			printw("%-35s%8ld%30s", data[i].name, data[i].size, data[i].mod_time);
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
	char* menu[9] = {"Help", "CAT", "SEARCH", "SelA", "Copy", "Ren/Mov", "Mkdir", "Delete", "Exit"};
	move(LINES-2, 0);
	for(int i = 0; i < 9; i++)
	{
		printw("F%-2d %-10s", i + 1, menu[i]);
		if(i==4) move(LINES-1, 0);
	}
	move(LINES-1, COLS-1);
}

char* ch_fname(char* f)
{
	char fname[30], *filename;
	//filename extraction
	strcpy(fname, "./");
	strcpy(fname+2, f);

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
	int i;
	signal(SIGCHLD, child_waiter);

	//input targetfile names
	for(int i =0; i<count; i++){
		if(data[i].check == -1){
			strcpy(sourcefile[num], ch_fname(data[i].name));
			input_set(1);
			if(isadir(ch_fname(data[i].name)))
				printw("input dirname > cp -r %s : ", sourcefile[num]+2);	
			else
				printw("input filename > cp %s : ", sourcefile[num]+2);
			scanw("%s", buf);
			if(strcmp(buf, "quit")==0) return;
			strcpy(targetfile[num++], buf);
		}
	}
	
	//exec cp source target (num is checking num)
	for(i=0; i<num; i++)
	{
		if((pid=fork())==-1) return;
		if(pid ==0)
		{
			if(isadir(ch_fname(sourcefile[i]))){
				execlp("cp", "cp", "-r", sourcefile[i], targetfile[i], NULL);
				exit(1);
			}
			else{
				execlp("cp", "cp", sourcefile[i], targetfile[i], NULL);
				exit(1);
			}
		}
	}

	input_set(0);
	while(waitpid(-1, NULL, WNOHANG) == 0);
}

void _mv()
{
	char buf[BUFSIZ];
	int pid, num=0;
	int mv_c=0;
	int mv_stat = 0; //0 : mv file1 file2     1: mv file1 dir1
	char *menu[3] = {"1. Rename", "2. Move Directory", "3. Quit"};
	signal(SIGCHLD, child_waiter);
	while(1)
	{
		input_set(1);
		printw("[selecet mode]");
		for(int i = 0; i < 3 ; i++)
		{
			move(LINES-6+i, 0);
			if(i == mv_c) printw(">> ");
			move(LINES-6+i, 5);
			printw("%s", menu[i]);
		}
		move(LINES-1, COLS-1);
		switch(mv_stat = getch())
		{
			case KEY_DOWN: if(mv_c < 2)mv_c++; break;
			case KEY_UP: if(mv_c > 0) mv_c--; break;
			case '\n':
					if(mv_c == 2) return; // quit
					else if(mv_c == 0)	//rename files selected
					{
						char sourcefile[MAX][BUFSIZ], targetfile[MAX][BUFSIZ];

						//input targetfile names
						for(int i =0; i<count; i++)
						{
							if(data[i].check == -1)
							{
								strcpy(sourcefile[num], ch_fname(data[i].name));
								input_set(1);
								printw("input filename > mv %s : ", sourcefile[num]+2);
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
						addstr("input dirname > ");
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
					return;
		}
	}
}
void _mkdir(){
	char *arglist[MAX], buf[BUFSIZ];
	int pid, len=1;

	input_set(1);
	addstr("input dirname > ");

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
	wait(NULL);
}

void _rm(){
	char *arglist_file[MAX], *arglist_dir[MAX];
	arglist_file[0] = "rm";
	arglist_dir[0] = "rm";
	arglist_dir[1] = "-r";
	int pid,pid2, flen=1, dlen = 2;
	signal(SIGCHLD, child_waiter);

	for(int i=0; i<count; i++)
	{
		if(data[i].check == -1){
			if(isadir(ch_fname(data[i].name)))
				arglist_dir[dlen++] = ch_fname(data[i].name);
			else
				arglist_file[flen++] = ch_fname(data[i].name);
		}	
	}
	//rm file1 file2 file3... or rm -r dir1 dir2 dir3...
	arglist_file[flen] = 0;
	arglist_dir[dlen] = 0;

	if((pid=fork())==-1) return;
	if(pid == 0)
	{
		if((pid2=fork())==-1) return;
		if(pid2 == 0){
			execvp(arglist_dir[0], arglist_dir);
			exit(1);
		}
		execvp(arglist_file[0], arglist_file);
		exit(1);
	}

	while(waitpid(-1, NULL, WNOHANG) == 0);
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

void help_mess(WINDOW* win, int* y, int x, char* m){
	wmove(win, *y, x); 
	*y+=2;
	waddstr(win, m);
}

void _help(){
	int width=50, height=20, px=12, py=4, _y = 3, _x = 1;
	WINDOW* win = newwin(height, width, py, px);
	box(win, 0, 0);
	//help title
	wmove(win, 0, width/2-4);
	waddstr(win, "< HELP >");
	//virticle line
	wmove(win, 1, width/2);
	wvline(win, 0, height-3);

	help_mess(win, &_y, _x, "F1: Provide help.");
	help_mess(win, &_y, _x, "F2: Show the contents");
	help_mess(win, &_y, _x, "F3: Find and Select");
	help_mess(win, &_y, _x, "F4: Select All/");
	wmove(win, _y-1, _x); waddstr(win, "    Deselect All"); _y++;
	help_mess(win, &_y, _x, "F5: Copy file.");
	_y = 3; _x = width/2+1;
	help_mess(win, &_y, _x, "F6: Rename or change"); 
	wmove(win, _y-1, _x); waddstr(win, "    directory"); _y++;
	help_mess(win, &_y, _x, "F7: Make new directory.");
	help_mess(win, &_y, _x, "F8: Delete file.");
	help_mess(win, &_y, _x, "F9: Exit  mc"); _y+=2; wmove(win, _y, _x);
	waddstr(win, "When 'quit' is enterd,"); _y++; wmove(win, _y, _x);
	waddstr(win, "return to inital scre"); _y++; wmove(win, _y, _x);
	waddstr(win, "en at any time."); _y=18; wmove(win, _y, _x-8);
	
	waddstr(win, "<press any key>");

	init_pair(3, COLOR_BLACK, COLOR_WHITE);
	wbkgd(win, COLOR_PAIR(3));

	wrefresh(win);

	getch();//wait

}

void _cat(){
	FILE *fp;
	char *filelist[MAX], cmd[MAX], buf[BUFSIZ], c;
	int len = 0, pos;
	for(int i=0; i<count; i++)
	{
		if(data[i].check == -1)
			filelist[len++] = ch_fname(data[i].name);
	}

	for(int i=0; i<len; i++)
	{
		sprintf(cmd, "%s %s", "cat", filelist[i]);//cmd = cat file
		fp = popen(cmd, "r");

		cat_title(&pos, filelist[i]);	

		//print cat
		while(fgets(buf, BUFSIZ, fp))
		{
			printw("%s", buf);
			if(++pos==LINES-5){//next page
				while((c=getch())!='\n');
				cat_title(&pos, filelist[i]);
			}
		}
		while((c=getch())!='\n');//wait
		pclose(fp);
	}

}

void cat_title(int* pos, char* f){
	*pos = PWD -1;
	clear();
	move(*pos, 5);
	printw("[[ %s ]]", f);
	move(*pos+2, 0);
}
