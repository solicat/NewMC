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

#define MAX 100 // number of check
#define PWD 2
#define LS 4

#define SELECT 1

struct item{
	int check;
	char name[BUFSIZ];
	long size;
	char mod_time[26];
}item;

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


