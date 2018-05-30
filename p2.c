/* Brenda Tran
 * Professor Carroll
 * CS570
 * Program 4
 * Due: April 25th
 * Program 4 simulates a command line interpreter with the following functions
 * Move '-f' '-n' ''
 * cd arg1 arg2, arg1 dir1
 * multiple piping
 * forking
 * comments
 * error checking
 * I/O redirection
 * a parse helper function that accounts for specific metacharacters
 * each function is specified below
*/
#include "p2.h"
#include "getword.h"
#include "CHK.h"
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <zconf.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

char *INFILE, *OUTFILE;
int pipe_flag, in_flag, out_flag, comment_flag, background_flag, and_pipe_flag, eof_flag;
int char_count, word_count;
char *newargv[MAXITEM];
char word_array[1000];
int pipe_index[10];
int in_redirect, out_redirect;
pid_t parent_pid, child_pid;
int dev_null;
void pipeFunction(int);
void pipeAnd(int);

//PARSE - receives input by calling getword function. Getword parses user input by spaces and metacharacters loading
// them into a char array and returns the size. Parse increments appropriate metacharacters and puts regular words/commands into newargv.

int parse() {
    char *word_ptr;
    int position = 0;
    int pointer = 0;
    INFILE = OUTFILE = 0;
    char_count = word_count = 0;
    pipe_flag = in_flag = out_flag = comment_flag = background_flag = and_pipe_flag = eof_flag = 0;


    while (1) {
        word_ptr = (word_array + pointer);
        char_count = getword(word_ptr);
    //EOF
        if (char_count == 0) {
            eof_flag = 1;
            break;
    //NEWLINE
        } else if (char_count == -10) {
            break;
        }
    //BACKGROUND FLAG
        if (*word_ptr == '&') {
            background_flag++;
            break;
        }
    //BACKGROUND/AND
        if (char_count == -2) {
            and_pipe_flag++;
        }
    // COMMENT
        if (*word_ptr == '#' && word_count == 0) {
            comment_flag++;
        }
        if (*word_ptr == '#' && char_count == -1){
            if (word_count == 1){
                comment_flag++;
            } else {
                char_count = 1;
            }
        }
        // PIPE
        if (*word_ptr == '|') {
            if (pipe_flag > 10) {
                fprintf(stderr, "SYNTAX ERROR: Too many pipes\n");
                break;
            }
            newargv[position++] = NULL;
            pipe_index[pipe_flag++] = position;

        //INPUT/OUTPUT REDIRECTION
        } else if (*word_ptr == '<' || in_flag == 1) {
            in_flag++;
            INFILE = word_ptr;
            if (in_flag > 2) {
                fprintf(stderr, "Too many in flags\n");
                break;
            }

        } else if (*word_ptr == '>' || out_flag == 1) {
            out_flag++;
            OUTFILE = word_ptr;
            if (out_flag > 2) {
                fprintf(stderr, "Too many out flags\n");
                break;
            }
        } else {
            newargv[position++] = word_ptr;
        }
        word_array[pointer + char_count] = '\0';
        pointer += char_count + 1;
        word_count++;
    }
    newargv[position] = NULL;
    return word_count;
}

//PIPE FUNCTION - deals with all of the piping and is called from main. This is VERTICAL piping meaning each grandchild has its own parent
// No two children will have the same parent. 10 pipes are allowed and was checked in parse. Forking denotes the duplication of
//a process parent/child child/grandchild
void pipeFunction(int pipe_flag) {
    int pipeDes[pipe_flag * 2];
    int pipe_ctr = 0;
    int pipeLoc;
    pid_t pid, child, middlePID;
    fflush(stdout);
    fflush(stderr);
//DEV/NULL - is opened and prepared for redirection/piping. It will hold anything we want it to thus all of the piping
    if (background_flag != 0 && in_flag == 0) {
        if ((dev_null = open("/dev/null", O_RDONLY)) < 0) {
            perror("dev/null error");
            exit(9);
        }
        CHK(dup2(dev_null, STDIN_FILENO));
        CHK(close(dev_null));
    }
    if ((child = fork()) < 0) {
        perror("Child Fork Failed ");
    }
    if (child == 0) {
        while (pipe_ctr < pipe_flag) {
            //this will keep track of all of the process when creating more grandchilds down the line
            //assigning each grandchild forked from the previous child will have its own process
            pipe(pipeDes + 2 * pipe_ctr);
            fflush(stdout);
            fflush(stderr);
            //beginning to fork all of the children after the first fork
            //same error checking, making sure 0 is returned to denote a successfull fork
            if ((middlePID = fork()) < 0) {
                perror("Middle fork error");
                exit(9);
            } else if (middlePID == 0) {
                pipe_ctr++;
                pipeLoc = 2 * pipe_ctr - 1;
                //assign the process for the middle children
                if (pipe_ctr <= pipe_flag) {
                    CHK(dup2(pipeDes[pipeLoc], STDOUT_FILENO));
                    CHK(close(pipeDes[pipeLoc]));
                }
                //this is the last pipe and it checks if it needs to i/o redirect
                if (pipe_ctr == pipe_flag) {
                    if (in_flag != 0) {
                        CHK(dup2(in_redirect, STDIN_FILENO));
                        CHK(close(in_redirect));
                    }
                }
                continue;
            }
            //this is the first pipe and it checks if it needs to i/o redirect
            if (pipe_ctr == 0) {
                if (out_flag != 0) {
                    CHK(dup2(out_redirect, STDOUT_FILENO));
                    CHK(close(out_redirect));
                }
            }
            //middle children redirect of processes
            if (pipe_ctr != pipe_flag) {
                pipeLoc = 2*pipe_ctr;
                CHK(dup2(pipeDes[pipeLoc], STDIN_FILENO));
                CHK(close(pipeDes[(pipeLoc) + 1]));
            }
            break;
        }
        //these next two calls executes the process after the assignments have finished.
        //it checks to make sure the execs successfully execute, throws an error if not 0
        if (pipe_ctr != pipe_flag) {
            pipeLoc = (pipe_flag - 1) - pipe_ctr;
            if (execvp(newargv[pipe_index[pipeLoc]], &newargv[pipe_index[pipeLoc]]) < 0) {
                perror("execvp error");
                exit(9);
            }
        } else {
            if ((execvp(newargv[0], newargv)) < 0) {
                perror("execvp error");
                exit(9);
            }
        }
        //checking to see if the parent PID is equel to the middle
        //making sure that all of the children finish before breaking out
        while (1) {
            pid = wait(NULL);
            if (pid == middlePID) break;
        }
    }
    //if the background is flagged then the child process must print its PID
    if (background_flag == 1) {
        printf("%s [%d]\n", *newargv, child);
        background_flag = 0;
        //checks to see if all of the children have finished before the parent breaks out of the function
    } else {
        while (1) {
            CHK(pid = wait(NULL));
            if (pid == child) break;
        }
    }
}

//handler for signal
void myhandler(int signum) {}

int main(int argc, char **argv) {
    int tempflag = 0;
    (void) setpgid(0, 0);
    (void) signal(SIGTERM, myhandler);
    if(argc == 2) {
            tempflag++;
            in_redirect = open(argv[1], O_RDONLY);
            dup2(in_redirect, STDIN_FILENO);
            close(in_redirect);

    }
    while (1) {
        if(tempflag == 0) {
            (void) printf("p2: ");
        }
        parse();
        //EOF - must break and p2 must terminate per the instructions
        if (eof_flag != 0) break;
        //NEWLINE - when this is found the program must reprompt
        //avoiding termination
        if (word_count == 0) continue;
        //COMMENT - if there was a comment flag triggered then these steps are executed
        if(comment_flag > 0){
            int pCtr = 0;
            int pTemp = 0;
            while(pCtr < word_count) {
                if(strcmp(newargv[pCtr], "#") == 0) {
                    pTemp = pCtr;
                    break;
                }
                pCtr++;
            }
            if((strcmp(newargv[pCtr], "#") == 0) && strcmp(newargv[0], "echo") != 0 ) {
                while(pTemp < word_count){
                    newargv[pCtr]= '\0';
                    pTemp++;
                }
            }
        }
//CD COMMAND - this is changes the directory but a few errors are checked before
        if ((strcmp(newargv[0], "cd")) == 0) { //change directory
            if (newargv[2] != NULL) {
                fprintf(stderr, "Too many arguments for cd function\n");
                continue;
            } else if (newargv[1] == NULL) {
                CHK(chdir(getenv("HOME")));
            } else if (chdir(newargv[1]) != 0) {
                perror("cd error: ");
                continue;
            }
        }
// MV COMMAND - this moves files to other files and moves files to directories
//  it also deals with the -f -n flag
        if ((strcmp(newargv[0], "MV")) == 0) {
            char *mv_flags = NULL;
            int mv_ctr = 1;
            int file1 = '\0';
            int file2 = '\0';
            if (word_count < 3) {
                fprintf(stderr, "Not enough args for MV.\n");
                continue;
            }
            if (word_count == 3) {
                if(access(newargv[2], F_OK) == 0) {
                    fprintf(stderr, "Trying to overwrite.\n");
                    continue;
                }
                if(link(newargv[1], newargv[2]) == 0) {
                    unlink(newargv[1]);
                    continue;
                }
            }
            if (word_count > 3) {
                while (mv_ctr < word_count) {
                    if ((strcmp(newargv[mv_ctr], ("-f")) == 0) || ((strcmp(newargv[mv_ctr], "-n")) == 0)) {
                        mv_flags = newargv[mv_ctr];
                    } else {
                        if (file1 == '\0') {
                            file1 = mv_ctr;
                        } else if (file2 == '\0') {
                            file2 = mv_ctr;
                        } else {
                            fprintf(stderr, "Too many files.\n");
                            break;
                        }
                    }
                    mv_ctr++;
                }
            }
            if(file1 == '\0' || file2 == '\0') {
                fprintf(stderr, "Files missing for move.\n");
                continue;
            }
            if (strcmp(mv_flags, "-f") == 0) {
                link(newargv[file1], newargv[file2]);
                unlink(newargv[file1]);
                continue;
            }

            if (access(newargv[file2], F_OK) == 0) {
                fprintf(stderr, "Trying to overwrite.\n");
                continue;
            }
            if (link(newargv[file1], newargv[file2]) == 0) {
                unlink(newargv[file1]);
                continue;
            }
        }
        if (and_pipe_flag != 0) {
            pipeAnd(pipe_flag);
            continue;

        }
        if (in_flag != 0) {
            if (INFILE == NULL) {
                fprintf(stderr, "In file is missing\n");
                continue;
            }
            if ((in_redirect = open(INFILE, O_RDONLY)) < 0) {
                perror("INFILE I/O error: ");
                continue;
            }
        }
        if (out_flag != 0) {
            if (OUTFILE == NULL) {
                fprintf(stderr, "Out file is missing\n");
                continue;
            }
            if ((out_redirect = open(OUTFILE, O_EXCL | O_WRONLY| O_CREAT, S_IRWXU)) < 0) {
                perror("OUTFILE I/O error: ");
                continue;
            }

        }
// PIPE FLAG  - this is called down here before we must have the INPUT/OUTPUT redirection opened before going into pipe
//  each child will get a process and the file will duplicate the appropriate infile/outfile for the process if it
//   has I/O redirection
        if (pipe_flag != 0) {
            pipeFunction(pipe_flag);
            continue;
        }
//  FORK - this is the last thing found after all of the checks/processses and function calls.
        fflush(stdout);
        fflush(stderr);
        CHK(child_pid = fork());
        if (child_pid == 0) {
            if (background_flag != 0 && in_flag == 0) {
                if ((dev_null = open("/dev/null", O_RDONLY)) < 0) {
                    perror("/dev/null error: ");
                    exit(9);
                }
                dup2(dev_null, STDIN_FILENO);
                close(dev_null);
            }
            if (in_flag != 0) {
                dup2(in_redirect, STDIN_FILENO);
                close(in_redirect);
            }
            if (out_flag != 0) {
                dup2(out_redirect, STDOUT_FILENO);
                close(out_redirect);
            }
            if ((execvp(newargv[0], newargv)) < 0) {
                perror("execvp error");
                exit(9);
            }
        }
        if (background_flag != 0) {
            printf("%s [%d]\n", newargv[0], child_pid);
            background_flag = 0;
            continue;
        } else {
            while (1) {
                parent_pid = wait(NULL);
                if (parent_pid == child_pid) break;
            }
        }


    }
    killpg(getpgrp(), SIGTERM);
    printf("p2 terminated.\n");
    exit(0);

}
