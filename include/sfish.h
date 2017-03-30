#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/prctl.h>



#define END   "\001\e[0m\002"
#define BBLA   "\001\e[0;30m\002"
#define BRED   "\001\e[0;31m\002"
#define BGRE   "\001\e[0;32m\002"
#define BYEL   "\001\e[0;33m\002"
#define BBLU   "\001\e[0;34m\002"
#define BMAG   "\001\e[0;35m\002"
#define BCYN   "\001\e[0;36m\002"
#define BWHT   "\001\e[0;37m\002"
#define RED  "\001\e[1;31m\002"
#define GRE  "\001\e[1;32m\002"
#define YEL  "\001\e[1;33m\002"
#define BLU  "\001\e[1;34m\002"
#define MAG  "\001\e[1;35m\002"
#define CYN  "\001\e[1;36m\002"
#define WHT  "\001\e[1;37m\002"
#define BLA  "\001\e[1;30m\002"
#define RED_KEY   "red"
#define GRE_KEY   "green"
#define YEL_KEY   "yellow"
#define BLU_KEY   "blue"
#define MAG_KEY   "magenta"
#define CYN_KEY   "cyan"
#define WHT_KEY   "white"
#define BLA_KEY   "black"

#define RUNNING 0
#define STOPPED	1
#define TERMINA	2
#define PIPES_NONE 0
#define PIPING_MID 1
#define PIPING_BEG 2
#define PIPING_LST 3
#define NUM_COLORS 8
#define BACKGROUND 4
#define FOREGROUND 5
#define NO_RETURN -123456
#define MAX_CAP 1000
#define MID_CAP 400
#define MIN_CAP 100
#define UPDATE_PROMPT(home, user, prompt){\
	if (strcmp(abs_path, (home)) == 0){	\
		relative_path[0] = '~';	\
		relative_path[1] = '\0';	\
	}	 \
	UPDATE_PROMPT_TOOGLE((home), (user), (prompt));	\
}	\

#define UPDATE_PROMPT_TOOGLE(home, user, prompt){	\
	if(user_op && host_op) concat_all(&(prompt), 7, "sfish-", (user), "@", (host), ":[", relative_path,"]> "); \
	else if(user_op && !host_op) concat_all(&(prompt), 5, "sfish-", (user), ":[", relative_path,"]> ");	\
	else if(!user_op && host_op) concat_all(&(prompt), 5, "sfish-", (host), ":[", relative_path,"]> ");	\
	else concat_all(&(prompt), 3, "sfish:[", relative_path,"]> ");	\
}

#define UPDATE_PATH() \
	memset(old_path, 0, strlen(old_path));	\
	memcpy(old_path, abs_path, strlen(abs_path));	\
	memset(abs_path, 0, strlen(abs_path));	\
	getcwd(abs_path, MAX_CAP); 	\
	memcpy(relative_path, abs_path + home_len, strlen(abs_path));	\

#define DYNAMIC_FREE(arr, len){	\
	for (int i = 0; i < len; ++i)	\
			free(arr[i]);	\
}

#define CLOSE_FILE_DESCRIPTORS(len){	\
	for (int i = 0; i < len; ++i)	\
        close(pipes[i]);	\
}

const char* state[] = {"Running", "Stopped"};


/*
* We have to contruct the processes for each of the jobs.
*/
typedef struct process{
	pid_t process_pid;
	pid_t process_gpid;
	int state;
	struct process* next;
	struct process* prev;
}process;

/*
* This struct if gonna keep an linked list of all the jobs
* currently being runned.
*/

typedef struct job{

	process* processes;
	char name[MID_CAP];
	char start_time[10];
	int status;
	int state;
	int job_id;
	pid_t group_pid;
	pid_t pid;

}job;

typedef struct job_node{
	job current_job;

	struct job_node* next;
	struct job_node* prev;
}job_node;

/*
* Head of the list.
*/
job_node* head_list_job;
process* current_main_process;
job_node* current_job_node;

/* This will be the usage statement*/
const char *usage_action = "\nType 'help' or press ctrl h to see this list:\n"
"Commands:\n"
"help\nexit\ncd\npwd\nprt\n"
"chpmt SETTING TOGGLE\n"
"chclr SETTING COLOR BOLD\njobs\n"
"fg PID|JID\n"
"bg PID|JID\n"
"kill [signal] PID|JID\n"
"disown [PID|JID]\n";

/*This pointer will have the correct pointer with the correct
* amount of spaces to void any edgecases.
*/
char* argv_array[MID_CAP];
char* commands[MIN_CAP];

/*
* This variable will keep the absolute path of the current directory.
*/
char abs_path[MAX_CAP];
char old_path[MAX_CAP];
char entire_string[MAX_CAP];
char relative_path[MAX_CAP];
char prompt[MAX_CAP];
char host[MIN_CAP];
char* user;

/*
* These will help with handling the colors in the terminal.
*/
char* color_key[NUM_COLORS];
char* colors[NUM_COLORS];
char* colors_b[NUM_COLORS];

/*
* This keeps the pid of the current children for build ins.
*/
int build_in_pid;
int abs_pid = -1;
int SPID = -1;

int bg_mode;
int shell_term;
bool forking;

/*
* Last return value
*/
int return_value = NO_RETURN;

/*
* This will contain the currnt status of the pipe.
*/
int current_pipe_status = -1;
int pipes[MIN_CAP];
int num_pipes;

/*
* Length of the home directory.
*/
int home_len;

/*
* These variables will indicate whether the user and host are disables.
*/
bool host_op = 1;
bool user_op = 1;


/*
* This function takes an arg and parse it such that it removes
* any undesireable white space and keep the args in the format of:
* cmp arg.
* @param cor_arg the argument passed through the shell
* @param par_arg an char pointer that will contain the correct
* 		 arguement with the appropiat spaces.
*/
int args_sep(char*, char**, char*);

/*
* This function will print the paramenter using forking and write to avoid
* any race conditions in other thread.
* @param char* This is the string that will be printer to stdout
*/

void safe_print(int, const char*);

/*
* This function is gonna take a sequence of arguments and concat them all.
*/

void concat_all(char**, int, ...);

/*
* This function initialize the color arrays needed.
*/

void init_color_array();

/*
* This function will return true if the color imputted by the user is
* contained in the list.
* @param char* The string color passed to the shell.
* @return returns the key index, else returns -1 if it doesn't exist.
*/
int contains(char*);

/*
* This function takes a string with paths gathered by getenv and tries to find
* the path for the program to be runned.
* @param char* first pointer will contain the path for the program if it exist
* @param char* second pointer is for the lists of paths
* @param char* third pointer will contain the name of the execcutable
*/
int path_exist(char*, char*, char*);

/*
* This is a wrapper function for execv in order to take care of any errors that
* it might encounter and making this thread safe.
* @param char* first pointer is the excutable
* @param char** The argv for that executable.
*/
void Execv(char*, char**, int, int);

/*
* This will interate throught he string and return very last string after a ">" but
* along the way it it finds any ">" it will create any file from any string that comes
* after a ">." Then it will proceed to chop that string by interting NULL at the first instance
* of ">" to only keep the esxacutable and the aguments.
* @param The argv to be handled.
* @return the file descriptor of the very last file that has stdout redirected to. It will
* return -1 if there's an error. It will reuturn -2 if there was no output_carots.
*/
int handle_output_carats(char**, int);

/*
* This will do the same thing as output_caroot but instead of creating file the program
* will check if the files exist, if any file along the string before a "<" doesn't exist then
* it should return error. If it does exist it  will return the fd of the very last file, if
* the file doesn't exist then it will create one ans return the fd.
* @param The argv to be handled.
* @return the fd of the opened file, -1 if there was an error, messages will be handled in
* this function. It will reuturn -2 if there was no input_carots.
*/
int handle_input_carats(char**, int);

/*
* This fuction just handles all the descriptor changes that will be made. I will call
* hanndle output and input carats and also will decide how to manage the fds in case
* the program is going on ongoing pipes.
* @param char** The argv for that executable.
* @param int the index of the current argv.
* @return 0 if the program should continue, -1 if not
*/
int handle_file_descriptors(char**, int, int);

/*
* This dynamically frees an char** in order to be used later on.
* @param char** the pointer to the array of arrays.
* @param int number of strings.
*/
void dynamic_free(char**, int);

/*
* This fuction will return true if the char* it's inside of char** and
* return false otherwise.
*/
bool check_token(char**, int, char*);

/*
* This function will close all file descriptors execept the ones in
* the peramaters. If the peramater is -1 then just ignore that one.
* @param int int the file descriptors to be left alone
* @param the length of the pipe array
*/
void close_fds(int, int, int);

/*
* This handler will get activates everytime theres a sigchild singal and it
* it interate the for loop and call wait pid to reap the children and once it
* know which children has died it will remove it from the list.
*/
void zombie_killer();

/*
* This function will interate through the array and create a space between the
* < and > and it will also keep in the lookout for any & and if found it set the
* rest of the string to 0 since that won't be exceutted in the command.
* @param char** the string to be parsed
* @param char** the resulting string
* @return 1 if it will be runned in the background, 0 otherwise, -1 if error
*/
int level_one_parsing(char*, char*);

/*
* This function removes a function from the linked list.
* @param job_node* the job_node to be removed from the list.
*/
int remove_job(pid_t pid);

/*
* Pushes a job pushes to the top of the linked list.
* @param job_node* the job_node to be added to the list.
*/
void insert_job(job_node*);

/*
* This function interates through the processes of the given job and
* removes it from the list.
* @param job_node* curent job
* @pid the pid of the process that has to be reaped.
* return 0 on success and -1 on failure. 
*/
void remove_process(job_node*, process*);

/*
* This will append a process to the list of processes in the job struct.
* @param job_node* curent job
* @pid the pid of the process that has to be added.
*/
void append_process(job_node*, pid_t);

/*
* This function will look at the first argument amd determine if it
* should be forked or not
*/
bool should_fork(char* cmd);

/*
* This job will return the next job process thats avaliable in the 2D
* array of jobs and processes, if it returns NULL that means that every
* process has been interated.
* @param job_node* current job
* @param process* the next process
*/
int next_process(job_node**, process**);

/*
* This function creates a job list if needed for the this command as well as
* construct all the processes needed for it.
*/
void setup_lists(char*, int);

void kill_all(char**);

void int_to_str(char*, unsigned int);

void print_jobs();

void remove_lost_jobs();

void catch_signals(int, job_node*);

int read_SPID(int, int);

int singal_SPID(int, int);

int print_help_menu(int, int);

int print_all_info(int, int);
void kill_fg(int, int);

pid_t return_pid(int);