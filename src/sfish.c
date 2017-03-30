#include "sfish.h"

int main(int argc, char** argv){
    //DO NOT MODIFY THIS. If you do you will get a ZERO.
    rl_catch_signals = 0;
    //This is disable readline's default signal handlers, since you are going
    //to install your own.

    char *cm;
    char *home = getenv("HOME");
    char * temp = getenv("USER");
    user = malloc(100 * sizeof(char));
    memcpy(user, temp, strlen(temp));
    char *prompt_p = prompt;
    char ** argv_array_p = argv_array;
    char *entire_string_p = entire_string;
    char *level_one_parsed_str = calloc(MAX_CAP, sizeof(char));
    char* program_path = malloc(200);
    char* temp_paths = malloc(400);
    gethostname(host, MIN_CAP);
    init_color_array();
    int num_args;
    int num_cmds, status;
    bool quit_flag = false;
    shell_term = STDIN_FILENO;
    
    ///pid_t current_job_pid;

    home_len = strlen(home);

    // This sets the absolute directory and old director to the current absolute
    // directory.
    UPDATE_PATH();
    UPDATE_PATH();
    UPDATE_PROMPT(home, user, prompt_p);

    signal (SIGINT, SIG_IGN);
    signal (SIGQUIT, SIG_IGN);
    signal (SIGTSTP, SIG_IGN);
    signal (SIGTTIN, SIG_IGN);
    signal (SIGTTOU, SIG_IGN);
    signal(SIGCHLD, zombie_killer);
    rl_bind_key(8 , print_help_menu);
    rl_bind_key(2, read_SPID);
    rl_bind_key(7, singal_SPID);
    rl_bind_key(16, print_all_info);

    // Give control of the terminal to your shell
    setpgid(getpid(), getpid());
    tcsetpgrp (shell_term, getpid());


    while(!quit_flag && (cm = readline(prompt_p)) != NULL) {
        memset(level_one_parsed_str, 0, MAX_CAP);
        bg_mode = level_one_parsing(cm, level_one_parsed_str);
        remove_lost_jobs();

      
        if(bg_mode == -1)
            continue;
        
        // Create the struct and the pid of the job will be equal to the 
        // pid of the first child as well as the group id, that same group id will
        // be passed on two the rest of the proceses
        forking = should_fork(level_one_parsed_str);
        num_cmds = args_sep(level_one_parsed_str, commands, "|");

        // This sets up your jobs ad procesess
        if(forking)
            setup_lists(cm, num_cmds);

        if(num_cmds > 1){
            current_pipe_status = -1;
            num_pipes = (num_cmds - 1) * 2;
            for (int i = 0; i < num_pipes - 1; i += 2)
                pipe(pipes + i);
    
        }
        else current_pipe_status = PIPES_NONE;

        for (int v = 0; v < num_cmds; ++v){
        // handle the more of the current pipe rederection
            if(current_pipe_status != PIPES_NONE){
                if(v == 0) current_pipe_status = PIPING_BEG;
                else if(v == num_cmds - 1) current_pipe_status = PIPING_LST;
                else current_pipe_status = PIPING_MID;
            }
        
        // You will parse the arguments with this function, this function
        // will return all the arguments
            memset(argv_array_p, 0, MAX_CAP);

            num_args = args_sep(commands[v], argv_array_p, " \t\n");

            if (argv_array[0] == NULL)
                continue;
            if (strcmp(*argv_array,"quit") ==0){
                quit_flag = true;
                break;
            }
            
            //printf("%s\n",cmd);
            
            if(strcmp(*argv_array, "help") == 0){
                int pid;

                if((pid = fork()) == 0){

                    int value = handle_file_descriptors(argv_array, num_args, v);
                        if(value == -1)
                            exit(1);

                    write(STDOUT_FILENO, usage_action, strlen(usage_action));
                    exit(0);
                }
                else
                    waitpid(pid, NULL, 0);

                return_value = 0;
            }
            else if(strcmp(*argv_array, "exit") == 0)
                exit(EXIT_SUCCESS);
            else if(strcmp(*argv_array, "cd") == 0){
                if(argv_array[1] == NULL){
                    chdir(home);
                    UPDATE_PATH();
                    UPDATE_PROMPT(home, user, prompt_p);
                    return_value = 0;
                }
                else if(strcmp(argv_array[1], ".") == 0){
                    // Does nothing
                    return_value = 0;
                }
                else if(strcmp(argv_array[1], "..") == 0){
                    chdir("..");
                    UPDATE_PATH();
                    UPDATE_PROMPT(home, user, prompt_p);
                    return_value = 0;
                }
                else if(strcmp(argv_array[1], "-") == 0){
                    chdir(old_path);
                    UPDATE_PATH();
                    UPDATE_PROMPT(home, user, prompt_p);
                    return_value = 0;
                }
                else{
                    int return_n;
                    char new_path[MAX_CAP];
                    memcpy(new_path, abs_path, MAX_CAP);
                    strcat((new_path + strlen(new_path)), "/");
                    strcat((new_path + strlen(new_path)), *(argv_array + 1));
                    return_n = chdir(new_path);
                    if(return_n == -1){
                        concat_all(
                            &entire_string_p, 3, "sfish: cd: ", 
                            *(argv_array + 1),
                            ": No such file or directory\n"
                        );
                        safe_print(STDERR_FILENO,entire_string);
                        return_value = 1;
                    }
                    else{
                        UPDATE_PATH();
                        UPDATE_PROMPT(home, user, prompt_p);
                        return_value = 0;
                    }
                }
            }
            else if(strcmp(*argv_array, "chpmt") == 0){
                if(argv_array[1] != NULL && argv_array[2] != NULL){
                    if(strcmp(argv_array[1], "user") != 0 && strcmp(argv_array[1], "machine") != 0){
                        concat_all(&entire_string_p, 2,argv_array[1], ": not a valid argument\n");
                        safe_print(STDERR_FILENO,entire_string);
                        return_value = 1;
                    }
                    else if(*argv_array[2] - 48 > 1){
                        safe_print(STDERR_FILENO,"Enter either 0 or 1\n");
                        return_value = 1;
                    }

                    else{
                        bool option = *argv_array[2] - 48;
                        if(strcmp(argv_array[1], "user") == 0)
                            user_op = option;
                        else
                            host_op = option;
                        UPDATE_PROMPT_TOOGLE(home, user, prompt_p);
                        return_value = 0;
                    }
                    

                }else
                    return_value = 1;
            }
            else if(strcmp(*argv_array, "chclr") == 0){
                if(argv_array[1] != NULL && argv_array[2] != NULL && argv_array[3] != NULL){
                    int index;
                    if(strcmp(argv_array[1], "user") != 0 && strcmp(argv_array[1], "machine") != 0){
                        concat_all(&entire_string_p, 2,argv_array[1], ": not a valid argument\n");
                        safe_print(STDERR_FILENO,entire_string);
                        return_value = 1;
                    }
                    else if((index = contains(argv_array[2])) == -1){
                        concat_all(&entire_string_p, 2,argv_array[2], ": not a valid color\n");
                        safe_print(STDERR_FILENO,entire_string);
                        return_value = 1;
                    }
                    else if(*argv_array[3] - 48 > 1){
                        safe_print(STDERR_FILENO,"Enter either 0 or 1\n");
                        return_value = 1;
                    }
                    else{
            // Here out arguments should be valid and it's ready to perform
            // the color change to the given toogle
                        bool bold = *argv_array[3] - 48;
                        #ifdef DEBUG
                        if(bold)
                            fprintf(stderr, "bold\n");
                        else
                            fprintf(stderr, "not bold\n");
                        #endif
                        if(strcmp(argv_array[1], "user") == 0){
                            char* user_p = user;
                            if(bold)
                                concat_all(&user_p, 3, colors_b[index], getenv("USER"), END);
                            else
                                concat_all(&user_p, 3, colors[index], getenv("USER"), END);
                        }
                        else{
                            char* host_p = host;
                            char temp[MIN_CAP];
                            gethostname(temp, MIN_CAP);
                            if(bold)
                                concat_all(&host_p, 3, colors_b[index], temp, END);
                            else
                                concat_all(&host_p, 3, colors[index], temp, END);
                        }

                        // Update the prompt
                        UPDATE_PROMPT_TOOGLE(home, user, prompt_p);
                        return_value = 0;
                    }
                    
                }else
                    return_value = 1;
            }else if(strcmp(*argv_array, "prt") == 0){
                if(return_value == NO_RETURN)
                    safe_print(STDERR_FILENO, "No Command have been called\n");
                else{
                    if(return_value)
                        safe_print(STDOUT_FILENO, "1\n");
                    else
                        safe_print(STDOUT_FILENO, "0\n");
                }
            }
            else if(strcmp(*argv_array, "bg") == 0){
                if(argv_array[0] == NULL || argv_array[1] == NULL){
                    return_value = 1;
                    continue;
                }
                else{
                    int cur_pid;

                    if(*argv_array[1] == '%')
                        cur_pid = return_pid(atoi(argv_array[1] + 1));
                    else
                        cur_pid = atoi(argv_array[1]);

                    if(cur_pid == 0){
                        return_value = 1;
                        continue;
                    }

                    #ifdef DEBUG
                    fprintf(stderr, "balls\n");
                    #endif
                    killpg(cur_pid, 18);
                    return_value = 0;
                }
                
            }
            else if(strcmp(*argv_array, "fg") == 0){
                if(argv_array[0] == NULL || argv_array[1] == NULL){
                    return_value = 1;
                    continue;
                }
                else{
                    bool found = false;
                    job_node* current_job = head_list_job;
                    int cur_pid;

                    if(*argv_array[1] == '%')
                        cur_pid = return_pid(atoi(argv_array[1] + 1));
                    else
                        cur_pid = atoi(argv_array[1]);

                    #ifdef DEBUG
                    fprintf(stderr, "%d\n", cur_pid);
                    #endif
                    if(cur_pid == 0){
                        return_value = 1;
                        continue;
                    }


                    if(head_list_job == NULL)
                        continue;

                    do{
                    if(current_job->current_job.pid == cur_pid){
                        found = true;
                        break;
                    }

                    }while((current_job = current_job->next) != NULL);

                    if(!found){ 
                        continue;
                        return_value = 1;
                    }

                    process* process_p = current_job->current_job.processes;
                    killpg(cur_pid, 18);
                    tcsetpgrp (shell_term, current_job->current_job.pid);

                    do{
                        waitpid (WAIT_ANY, &status, WUNTRACED);
                        catch_signals(status, current_job);
                        return_value = status;

                    }while((process_p = process_p->next) != NULL);
                    
                    
            

                    tcsetpgrp (shell_term, getpgid(getpid()));
                    return_value = 0;
                }
            }
            else if(strcmp(*argv_array, "pwd") == 0){

                int pid;

                if((pid = fork()) == 0){

                    int value = handle_file_descriptors(argv_array, num_args, v);
                        if(value == -1)
                            exit(1);

                    int index = strlen(abs_path);
                    abs_path[index] = '\n';
                    write(STDOUT_FILENO, abs_path, strlen(abs_path));
                    abs_path[index] = '\0'; // put the null terminator back
                    exit(0);
                }
                else
                    waitpid(pid, NULL, 0);

                return_value = 0;
            }
            else if(strcmp(*argv_array, "kill") == 0){
                 kill_all(argv_array);
                 return_value = 0;
            }
            else if(strcmp(*argv_array, "jobs") == 0){
                print_jobs();
                return_value = 0;
            }
            else {
                struct stat* current_stat = (struct stat*)malloc(sizeof(struct stat));
                char* temp_exe = strstr(*argv_array, "/");

                if(temp_exe != NULL && !stat(*argv_array, current_stat))
                    Execv(*argv_array, argv_array, num_args, v);
                else{
                    char* paths = getenv("PATH");
                    memcpy(temp_paths, paths, strlen(paths));

                    if(!path_exist(program_path, temp_paths, *argv_array)){
                        memcpy(argv_array[0], program_path, strlen(program_path) + 1);
                        *(argv_array[0] + strlen(program_path)) = '\0';
                        Execv(program_path, argv_array, num_args, v);
                    }
                    else{
                        concat_all(&entire_string_p, 2, *argv_array, ": command not found\n");
                        safe_print(STDERR_FILENO, entire_string);
                    }
                    
                }
                free(current_stat);
            }
            // #ifdef DEBUG
            // fprintf(stderr, "balls\n");
            // #endif
            DYNAMIC_FREE(argv_array, num_args);
        }
        CLOSE_FILE_DESCRIPTORS(num_pipes);


        if(current_pipe_status != PIPES_NONE && !bg_mode){
            tcsetpgrp (shell_term, abs_pid);

            for (int i = 0; i < num_cmds; ++i){
                waitpid (WAIT_ANY, &status, WUNTRACED);
                catch_signals(status, current_job_node);
                return_value = status;
            }

            tcsetpgrp (shell_term, getpgid(getpid()));
        }
        abs_pid = -1;
        DYNAMIC_FREE(commands, num_cmds);

        // If you're currently in the child exit or else you will have another
        // shell opened.

    }

    //Don't forget to free allocated memory, and close file descriptors.
    free(cm);
    free(user);
    free(level_one_parsed_str);
    free(head_list_job);
    free(program_path);
    free(temp_paths);
    //WE WILL CHECK VALGRIND!

    return EXIT_SUCCESS;
}

int args_sep(char* cor_arg, char** parsed_args, char* delim){
    
    char* tk;
    int count = 0;

    while((tk = strsep(&cor_arg, delim)) != NULL){
        if(strcmp(tk, "") != 0){
            char* temp = malloc(strlen(tk) * sizeof(char) + 1);
            memcpy(temp, tk, strlen(tk) * sizeof(char));
            *(temp + strlen(tk)) = '\0';
            parsed_args[count] = temp;
            count++;
            temp = NULL;
        }
    }

    parsed_args[count] = NULL;

    return count;
}

void safe_print(int fd, const char* string){

    if((build_in_pid = fork()) == 0){
        write(fd, string, strlen(string));
        exit(0);
    }
    else
        waitpid(build_in_pid, NULL, 0);
}

void concat_all(char** final_string_p ,int arg_num, ...){
    
    va_list list_action;
    va_start(list_action, arg_num);
    **final_string_p = '\0';

    for (int i = 0; i < arg_num; ++i){
        strcat(*final_string_p, va_arg(list_action, char*));
        *(*(final_string_p) + strlen(*final_string_p)) = '\0';
    }

}

void init_color_array(){

    color_key[0] = RED_KEY; color_key[1] = BLU_KEY; color_key[2] = GRE_KEY;
    color_key[3] = YEL_KEY; color_key[4] = CYN_KEY; color_key[5] = MAG_KEY;
    color_key[6] = BLA_KEY; color_key[7] = WHT_KEY;

    colors[0] = RED; colors[1] = BLU; colors[2] = GRE; colors[3] = YEL;
    colors[4] = CYN; colors[5] = MAG; colors[6] = BLA; colors[7] = WHT;

    colors_b[0] = BRED; colors_b[1] = BBLU; colors_b[2] = BGRE; colors_b[3] = BYEL; 
    colors_b[4] = BCYN; colors_b[5] = BMAG; colors_b[6] = BBLA; colors_b[7] = BWHT;
}


int contains(char* color){
    for (int i = 0; i < NUM_COLORS; ++i)
        if(strcmp(color, color_key[i]) == 0)
            return i;

    // This will return if the color wasn't found
    return -1;
}

int path_exist(char* return_path, char* paths, char* executable_string){

    char* program_paths[100];
    int num_paths = args_sep(paths, program_paths, ":");
    struct stat* current_stat = (struct stat*)malloc(sizeof(struct stat));
    int val;

    for (int i = 0; i < num_paths; ++i){
        concat_all(&return_path,4,program_paths[i], "/", executable_string, "\0");
        val = stat(return_path, current_stat);
        if(!val)
            return 0;
    }

    DYNAMIC_FREE(program_paths, num_paths);
    free(current_stat);


    return -1;
}

void Execv(char* exe, char** argv_array, int len, int index){
    pid_t pid;
    char * entire_string_p = entire_string;
    int status;

    // Temporary block all singnals to avoid race conditions
    //signal (SIGCHLD, SIG_IGN);
    pid = fork();
    if(pid != 0 && forking){
        if(abs_pid == -1)
            abs_pid = pid;

        if(setpgid(pid, abs_pid) == -1){
                #ifdef DEBUG
                fprintf(stderr, "Error");
                #endif
        }

        if(index == 0){
    // This means that this is the first command therefore this process will be
    // the leader of the group;
            current_job_node->current_job.pid = pid;
            current_main_process->process_pid = pid;
            current_main_process->process_gpid = abs_pid;
            #ifdef DEBUG
            fprintf(stderr, "The leader: %d\n", pid);
            fprintf(stderr, "The leader (actual): %d\n", getpgid(pid));
            #endif
        }
        else{
            current_main_process->process_pid = pid;
            current_main_process->process_gpid = abs_pid;
            
            #ifdef DEBUG
            fprintf(stderr, "current: %d\n", pid);
            fprintf(stderr, "The leader (actual): %d\n", getpgid(pid));
            #endif
        }
    // switch to the next process once this one has been processed
        current_main_process = current_main_process->next;
    }

    if(pid == 0){

        setpgid(getpid(), abs_pid);
        if (!bg_mode)
            tcsetpgrp (shell_term, abs_pid);

        signal (SIGINT, SIG_DFL);
        signal (SIGQUIT, SIG_DFL);
        signal (SIGTSTP, SIG_DFL);
        signal (SIGTTIN, SIG_DFL);
        signal (SIGTTOU, SIG_DFL);
        signal (SIGCHLD, SIG_DFL);
    // This function will handle the changes in the file descriptors
        int value = handle_file_descriptors(argv_array, len, index);
        if(value == -1)
            exit(1);
        
    // At this point the argv array is modified to only have the executable and its
    // arguments

        execv(exe, argv_array);

        if(errno == ENOENT){
            concat_all(&entire_string_p, 2, *argv_array, ": command not found\n");
            safe_print(STDERR_FILENO,entire_string);
        }
        else if(errno == ENOEXEC){
            concat_all(&entire_string_p, 2, *argv_array, ": file not an executable\n");
            safe_print(STDERR_FILENO,entire_string);
        }

        exit(0);
    }else if(current_pipe_status == PIPES_NONE && !bg_mode){
        tcsetpgrp (shell_term, abs_pid);
        waitpid (WAIT_ANY, &status, WUNTRACED);
        if(forking)
            catch_signals(status, current_job_node);
        tcsetpgrp (shell_term, getpgid(getpid()));
        return_value = status;
    }


}

int handle_input_carats(char** vector, int vec_len){
    struct stat* current_stat = (struct stat*) malloc(sizeof(struct stat));
    char* entire_string_p = entire_string;
    char* input_file = NULL;
    int in_fd = -2;

    for (int i = 0; i < vec_len; ++i)
        if(strcmp(vector[i], "<") == 0){
            if(i >= vec_len - 1){
                safe_print(STDERR_FILENO,"syntax error near unexpected token\n");
                return -1;
            }
            else{
                input_file  = vector[i + 1];
                if(stat(input_file, current_stat) != 0){
                    concat_all(&entire_string_p, 2, input_file, ": No such file or directory\n");
                    safe_print(STDERR_FILENO, entire_string);
                    return -1;
                }
            }
        }

    // At this point the filename exist and this should be the last one in the
    // cmd and ready to open.
    if(input_file != NULL)
        in_fd = open(input_file, O_RDONLY);
    free(current_stat);
    return in_fd;
}

int handle_output_carats(char** vector, int vec_len){
    struct stat* current_stat = (struct stat*) malloc(sizeof(struct stat));
    char* output_file;
    int out_fd = -2;

    for (int i = 0; i < vec_len; ++i){
        if(strcmp(vector[i], ">") == 0){
            if(i >= vec_len - 1){
                if(out_fd != -2)
                    close(out_fd);
                safe_print(STDERR_FILENO, "syntax error near unexpected token\n");
                return -1;
            }else{
                if(out_fd != -2)
                    close(out_fd);
                output_file = vector[i + 1];
                out_fd = open(output_file,  O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR, 0666);

            }
        }
    }


    // At this point we should have the very last file in the output and ready
    // to return. Now we need to chop the redirections from the vector since these
    // things won't be needed anymore.
    for (int i = 0; i < vec_len; ++i)
        if(strcmp(vector[i], ">") == 0 || strcmp(vector[i], "<") == 0){
            vector[i] = NULL;   // if it doesnt work use memset
            break;
        }
    
    
    free(current_stat);
    return out_fd;
}

int handle_file_descriptors(char** argv_array, int len, int index){
    int fd_in, fd_out, current_index;

    if(current_pipe_status == PIPING_BEG){
        if(check_token(argv_array, len, ">"))
            return -1;

        dup2(pipes[1], 1);
        CLOSE_FILE_DESCRIPTORS(num_pipes);
        fd_in = handle_input_carats(argv_array, len);
        if(fd_in != -2 && fd_in != -1){ 
            dup2(fd_in, 0); close(fd_in); 
        }
        else if(fd_in == -1) return -1;

    }
    else if(current_pipe_status == PIPING_MID){
        if(check_token(argv_array, len, ">") || check_token(argv_array, len, "<"))
            return -1;

        current_index = (index * 2) -1;
        dup2(pipes[current_index + 2], 1);
        dup2(pipes[current_index - 1], 0);
        CLOSE_FILE_DESCRIPTORS(num_pipes);

    }
    else if(current_pipe_status == PIPING_LST){
        if(check_token(argv_array, len, "<"))
            return -1;


        dup2(pipes[num_pipes - 2], 0);
        CLOSE_FILE_DESCRIPTORS(num_pipes);
        fd_out = handle_output_carats(argv_array, len);
        if(fd_out != -2 && fd_out != -1){ 
            dup2(fd_out, 1); close(fd_out); 
        }
        else if(fd_out == -1) return -1;
    }else{

        // Here the carrot functions will take care of any redirecting changes
        // It starts with the input first.
        fd_in = handle_input_carats(argv_array, len);
        if(fd_in != -2 && fd_in != -1){ dup2(fd_in, 0); close(fd_in); }
        else if(fd_in == -1) return -1;


        // Now the output
        fd_out = handle_output_carats(argv_array, len);
        if(fd_out != -2 && fd_out != -1){ dup2(fd_out, 1); close(fd_out); }
        else if(fd_out == -1) return -1;
    }

    return 0;
}

bool check_token(char** args, int len, char* token){
    for (int i = 0; i < len; ++i)
        if(strcmp(argv_array[i], token) == 0)
            return true;

    // If it wasn't found then this returns false
    return false;
}

void zombie_killer(){
    if (head_list_job == NULL || head_list_job->current_job.processes == NULL)
        return;
 
    job_node* current_job_node = head_list_job;
    process* current_process = head_list_job->current_job.processes;

    sigset_t mask_all, pre_all;
    int olderrno = errno, status, r_value; 
    pid_t pid;
    sigfillset(&mask_all);

    sigprocmask(SIG_BLOCK, &mask_all, &pre_all);
    do{
        pid = current_process->process_pid;
        if ((r_value = waitpid(pid, &status, WNOHANG)) > 0){

            #ifdef DEBUG
            fprintf(stderr, "%d got killed\n", pid);
            fprintf(stderr, "And the one in the struct was: %d\n", current_process->process_gpid);

            #endif

            // Mark the process as terminated for now
            catch_signals(status, current_job_node);
            return_value = status;
        }

        //if(errno != ECHILD) safe_print(STDERR_FILENO,"waitpid error");

    }while(next_process(&current_job_node ,&current_process) != 0);
    
    sigprocmask(SIG_SETMASK, &mask_all, NULL);
    errno = olderrno;
  
}

int level_one_parsing(char* old, char* _new){
    int index_new = 0, len = strlen(old), j = 0;
    while(old[j] == ' ' || old[j] == '\t' || old[j] == '\n') j++;

    // This will be changed later depending on what out shell supposed to do.
    if(old[0] == '<' || old[0] == '<' || old[0] == '&' || old[0] == '\n')
        return -1;

    _new[index_new++] = ' ';
    for (int i = j; i < len; ++i){
        if(old[i] == '&'){
                memset(old + i, 0, len - i);
                return 1;
        }
        else if(old[i] == '<' || old[i] == '>'){

            _new[index_new++] = ' ';
            _new[index_new++] = old[i];
            _new[index_new++] = ' ';
        }
        else
            _new[index_new++] = old[i];
    }
    _new[index_new++] = ' ';

    return 0;
}

int remove_job(pid_t pid){
    bool found = false;
    job_node* current_job = head_list_job;


    if(head_list_job == NULL){
        SPID = -1;
        return 0;
    }

    do{
        if(current_job->current_job.pid == pid){
            found = true;
            break;
        }

    }while((current_job = current_job->next) != NULL);

    // *********** remove this at the end **************//
    if(!found) return 0;

    //free(current_job);
    // This means there's only the head in the list
    if(current_job->prev == NULL && current_job->next == NULL){
        head_list_job = NULL;
        SPID = -1;
        return 1;
    }

    // This means that we're looking at the head of the list
    if(current_job->prev == NULL && current_job->next != NULL){
        current_job = current_job->next;
        current_job->prev = NULL;
        head_list_job = current_job;
        return 1;
    }
    // This will mean that we're in the tail of the list
    else if(current_job->prev != NULL && current_job->next == NULL){
        current_job = current_job->prev;
        current_job->next = NULL;
        return 1;
    }
    // This means we're in the middle of our list
    else if(current_job->prev != NULL && current_job->next != NULL){
        current_job = current_job->prev;
        current_job->next = current_job->next->next;
        current_job = current_job->next;
        current_job->prev = current_job->prev->prev;
        return 1;
    }

    return 1;
}

void insert_job(job_node* new_job){
    int next_num = 1;
    job_node* previous;

    if(head_list_job == NULL){
        head_list_job = new_job;
        head_list_job->next = NULL;
        head_list_job->prev = NULL;
        new_job->current_job.job_id = next_num;
        return;
    }

    job_node* current_job_node = head_list_job;

    do{
        if(next_num != current_job_node->current_job.job_id){
            if(current_job_node->prev != NULL){
                new_job->next = current_job_node;
                current_job_node->prev->next = new_job;
                new_job->prev = current_job_node->prev;
                current_job_node->prev = new_job;
                new_job->current_job.job_id = next_num;
                return;

            }
            else{
                current_job_node->prev = new_job;
                new_job->next = current_job_node;
                new_job->prev = NULL;
                head_list_job = new_job;
                new_job->current_job.job_id = next_num;
                return;
            }
        }
        next_num++;
        previous = current_job_node;
    } while((current_job_node = current_job_node->next) != NULL);

    // If it's here then that means that it made to the end of the list
    new_job->prev = previous;
    previous->next = new_job;
    new_job->current_job.job_id = next_num;
}


void append_process(job_node* current_job_node, pid_t process_pid){

    
    process* new_process = (process*) malloc(sizeof(process));
    new_process->state = RUNNING;

    if(current_job_node->current_job.processes == NULL){
        current_job_node->current_job.processes = new_process;
        current_job_node->current_job.processes->next = NULL;
        current_job_node->current_job.processes->prev = NULL;
        return;
    }

    current_job_node->current_job.processes->prev = new_process;
    new_process->next = current_job_node->current_job.processes;
    new_process->prev = NULL;
    current_job_node->current_job.processes = new_process;
}

int next_process(job_node** current_job_node_p, process** process_p){


    if((*process_p)->next){
        *process_p = (*process_p)->next;
        return 1;
    }
    else{
        if((*current_job_node_p)->next){
            (*current_job_node_p) = (*current_job_node_p)->next;
            (*process_p) = (*current_job_node_p)->current_job.processes;
            return 1;
        }
        else return 0;
    }
}

bool should_fork(char* cmd){

    if(strstr(cmd, " quit ") != NULL) return false;
    else if(strstr(cmd, " exit ") != NULL) return false;
    else if(strstr(cmd, " chclr ") != NULL) return false;
    else if(strstr(cmd, " chpmt ") != NULL) return false;
    else if(strstr(cmd, " kill ") != NULL) return false;
    else if(strstr(cmd, " cd ") != NULL) return false;
    else if(strstr(cmd, " jobs ") != NULL) return false;
    else if(strstr(cmd, " prt ") != NULL) return false;
    
    return true;
}

void kill_all(char** arg_v){
    if(arg_v[0] == NULL || arg_v[1] == NULL || arg_v[2] == NULL)
        return;

    int cur_pid;

    if(*argv_array[2] == '%')
        cur_pid = return_pid(atoi(argv_array[2] + 1));
    else
        cur_pid = atoi(argv_array[2]);

        if(cur_pid == 0){
            return_value = 1;
            return;
    }

    int signal = atoi(arg_v[1]);;

    killpg(cur_pid, signal);
}

 
void setup_lists(char* cm, int len){
    current_job_node = (job_node*)malloc(sizeof(job_node));
    current_job_node->current_job.state = RUNNING;
    memset(current_job_node->current_job.name, 0, MIN_CAP);
    memcpy(current_job_node->current_job.name, cm, strlen(cm) * sizeof(char));
    
    insert_job(current_job_node);

    for (int i = 0; i < len; ++i)
        append_process(current_job_node, 0);

    current_main_process = current_job_node->current_job.processes;
    
}

void int_to_str(char* buff, unsigned int num){
    char temp_buf[1000] = {0};
    int count = 0, rev = -1;

    do
        temp_buf[count++] = (num % 10) + 48;
    while ((num = num/10) != 0);

    for (int i = count; i >= 0; --i)
        buff[rev++] = temp_buf[i];

    buff[rev++] = '\0';
}

void print_jobs(){


    job_node* current_job_node = head_list_job;
    if(current_job_node == NULL)
        return;

    char* entire_string_p = entire_string;
    char job_id[100];
    char pid[1000];

    do{
        int_to_str(job_id, current_job_node->current_job.job_id);
        int_to_str(pid, current_job_node->current_job.pid);
        concat_all(&entire_string_p,9, 
            "[",job_id, "]      ", state[current_job_node->current_job.state],
            "      ", pid,"      ", current_job_node->current_job.name, "\n\0\0"
        );

        safe_print(STDOUT_FILENO, entire_string);
    }while((current_job_node = current_job_node->next) != NULL);


}

void remove_lost_jobs(){
    int rm;
    if(head_list_job == NULL)
        return;

    do{
        rm = remove_job(0);
    }while(rm);
}

void catch_signals(int status, job_node* current_job_node){
    
    if(WIFSIGNALED(status) || WIFEXITED(status)){
  
        current_job_node->current_job.pid = 0;
    }
    else if(WIFSTOPPED(status))
        current_job_node->current_job.status = STOPPED;
    else if(WIFCONTINUED(status))
        current_job_node->current_job.status = RUNNING;
}

int read_SPID(int num1, int num2){
    #ifdef DEBUG
    fprintf(stderr, "balls\n");
    #endif
    remove_lost_jobs();
    if(head_list_job == NULL)
        SPID = -1;
    else
        SPID = head_list_job->current_job.pid;

    return 0;
}

int singal_SPID(int num1, int num2){
    #ifdef DEBUG
    fprintf(stderr, "balls\n");
    #endif
    if(SPID == -1){
        safe_print(STDERR_FILENO,"\nSPID does not exist and has been set to -1\n");
        return 0;
    }

    job_node* current_job_node = head_list_job;

    do{
        #ifdef DEBUG
        fprintf(stderr, "%d\n", current_job_node->current_job.pid);
        fprintf(stderr, "%d\n", SPID);
        #endif
        if(current_job_node->current_job.pid == SPID){
            
            killpg(current_job_node->current_job.pid, 15);

             #ifdef DEBUG
            fprintf(stderr, "dead\n");
            #endif
            break;
        }
    }while((current_job_node = current_job_node->next) != NULL);

    return 0;
}


int print_help_menu(int num1, int num2){

    safe_print(STDOUT_FILENO, usage_action);
    return 0;
}


int print_all_info(int num1, int num2){

    safe_print(STDERR_FILENO,"\n----Info----\n"
        "help\nprt\n ----CTRL----\n" 
        "cd\nchclr\nchpmt\npwd\nexit\n----Job Control---- \n"
        "bg\nfg\ndisown\njobs\n---Number of Commands Run---\n");


    return 0;
}

pid_t return_pid(int job_id){
    bool found = false;
    job_node* current_job = head_list_job;

    if(head_list_job == NULL)
        return 0;

    do{
        if(current_job->current_job.job_id == job_id){
            found = true;
            break;
        }

    }while((current_job = current_job->next) != NULL);

    if(!found)
        return 0;
    else
        return current_job->current_job.pid;
        
}
