#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>
#include <fcntl.h>
#include <signal.h>


char cd(char * dir) {
    char tmp;
    if ((tmp = chdir(dir)) != 0) {
        fprintf(stderr, "Error: no such directory %s\n", dir);
    }
    return tmp;
}

void ListPrint(char** list){
    int k = 0;
    while(list[k++] != NULL){
        printf("%s.\n", list[k-1]);
    }
}

char ** input() { // user input of commands and parcing

    /* input */
    int cell = 8;
    char * str = (char *)malloc(cell * sizeof(char));
    if(str == NULL){
        fprintf(stderr, "An error occured while allocating memory\n");
        return NULL;
    }

	int buf = cell;
	int cnt = 0;
	char c;
	while ((c = getchar()) != '\n'){
		if(c != '\n' && c != EOF) { 
            str[cnt] = c;
            cnt++;
		}
		else { break; }
		if (--buf == 0) {
			str = (char *)realloc(str, cnt + cell);
			if (str == NULL){
                fprintf(stderr, "An error occured while allocating memory\n");
                return NULL;
            }
			buf += cell;
		}
	}

	if(cnt == 0) { // if input was empty
	    free(str);
	    return NULL;
	}
	str[cnt] = '\0';

	/* spliting commands with spaces to analyse it later */

	char* splitted_str = (char *)malloc(2 * strlen(str)  * sizeof(char));
	if(splitted_str == NULL){
        fprintf(stderr, "An error occured while allocating memory\n");
        return NULL;
    }

	cnt = 0;
	for(int i = 0; i < strlen(str); i++) {
	    if ((strchr("|&>\0", str[i]) != NULL) && (str[i] == str[i+1])){
	        splitted_str[cnt] = ' ';
	        splitted_str[cnt + 1] = str[i++];
	        splitted_str[cnt + 2] = str[i];
	        splitted_str[cnt + 3] = ' ';
            cnt += 4;
	    }
	    else if (strchr("()|&;><\0", str[i]) != NULL) {
	        splitted_str[cnt] = ' ';
	        splitted_str[cnt + 1] = str[i];
	        splitted_str[cnt + 2] = ' ';
            cnt += 3;
        }
        else {
            splitted_str[cnt] = str[i];
            cnt++;
        }
	}
	splitted_str[cnt] = '\0';
	free(str);

	/* creating list of input */

	char ** list = (char **)malloc(cell * sizeof(char*));
	if (list == NULL) {
        fprintf(stderr, "An error occured while allocating memory\n");
        free(splitted_str);
        return NULL;
    }

	buf = cell;
	cnt = 0;
	char * piece = strtok(splitted_str, " \0");
	do {
	    list[cnt] = (char *)malloc(strlen(piece) * sizeof(char));
	    strcpy(list[cnt], piece);
        cnt++;
	    if (--buf == 0) {
			list = (char **)realloc(list, (cnt + cell) * sizeof(char*));
			buf += cell;
		}
	    piece = strtok(NULL, " \0");
	} while (piece != NULL);
	
	list[cnt] = piece;
	free(splitted_str);
	free(piece);

    //ListPrint(list);
	
	return list;
} 


int redirection(char ** redirect) {
    int d;
    int i = 0;
    while (redirect[i] != NULL) {

        if (strcmp(redirect[i], ">") == 0) {
            d = open(redirect[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (d == -1) {
                fprintf(stderr, "Error: cannot open %s\n", redirect[i + 1]);
                return 1;
            }
            dup2(d, 1);
            close(d);
        }

        else if(strcmp(redirect[i], ">>") == 0) {
            d = open(redirect[i + 1], O_WRONLY | O_APPEND);
            if(d == -1) { d = creat(redirect[i + 1], 0666); }
            if(d == -1) {
                fprintf(stderr, "Error: cannot create %s\n", redirect[i + 1]);
                return 1;
            }
            dup2(d, 1);
            close(d);
        }

        else if(strcmp(redirect[i], "<") == 0) {
            d = open(redirect[i+1], O_RDONLY);
            if(d == -1){
                fprintf(stderr, "Error: cannot open %s\n", redirect[i + 1]);
                return 1;
            }
            dup2(d, 0);
            close(d);
        }
        i += 2;
    }
    return 0;
}
char cmd_parce(char ** list);

char run(char ** list, char background_flag) {
    
    if (strcmp(list[0], "cd") == 0) {
        if ((list[1] == NULL) || (list[2] != NULL)) {
            fprintf(stderr, "Error: wrong amount of arguments in cd\n");
            return 1;
        }
        else { return cd(list[1]); }
    }

    char ** redirect = (char **)malloc(sizeof(char*) * 5);
    if (redirect == NULL){
        fprintf(stderr, "An error occured while allocating memory\n");
        return 1;
    }

    /* conveyor */

    int i = 0;
    int j = 0;
    int list_size = 0;
    
    while (list[list_size++] != NULL);

    char** conv = (char**)malloc(sizeof(char*) * list_size);

    if (conv == NULL){
        fprintf(stderr, "An error occured while allocating memory\n");
        free(redirect);
        return 1;
    }
    if ((strcmp("<", list[0]) == 0) || (strcmp(">", list[0]) == 0) || (strcmp(">>", list[0]) == 0)){
        redirect[j] = list[0];
        redirect[j + 1] = list[1];
        j += 2;
        i += 2;
        if((strcmp("<", list[2]) == 0) || (strcmp(">", list[2]) == 0) || (strcmp(">>", list[2]) == 0)){
            redirect[j] = list[2];
            redirect[j + 1] = list[3];
            j += 2;
            i+= 2;
        }
    }
    else if ((list_size > 2) && ((strcmp("<", list[list_size - 3]) == 0) || (strcmp(">", list[list_size - 3]) == 0) || (strcmp(">>", list[list_size - 3]) == 0))){
        redirect[j] = list[list_size - 3];
        redirect[j + 1] = list[list_size - 2];
        j += 2;
        list_size -= 2;
        if((list_size > 2) && ((strcmp("<", list[list_size-3]) == 0) || (strcmp(">", list[list_size - 3]) == 0) || (strcmp(">>", list[list_size - 3]) == 0))){
            redirect[j] = list[list_size - 3];
            redirect[j + 1] = list[list_size - 2];
            j += 2;
            list_size -= 2;
        }
    }
    redirect[j] = NULL;

    j = 0;
    int conv_size = 1;
    for(i = i; i < list_size - 1; i++){
        if(strcmp(list[i], "|") == 0)
            conv_size++;
        conv[j] = list[i];
        j++;
    }
    conv[j] = NULL;

    /**/
    char ** tmp = conv;
    pid_t pid_arr[conv_size];
    int fd[2];
    
    for(i = 0; i < conv_size; i++) {
        pipe(fd);
        int prevfd;
        char ** cur_cmd = (char **)malloc(sizeof(char*) * list_size);

        if(cur_cmd == NULL) {
            fprintf(stderr, "An error occured while allocating memory\n");
            free(conv);
            return 1;
        }
        
        int k = 0;
        while (tmp[k] != NULL && (strcmp(tmp[k], "|") != 0)){
            cur_cmd[k] = tmp[k];
            k++;
        }
        cur_cmd[k] = NULL;
        tmp += k + 1;
        
        if ((pid_arr[i] = fork()) == 0){
            if (!background_flag) { signal(SIGINT, SIG_DFL); }

            if (redirection(redirect) == 1) {
                fprintf(stderr, "Redirection error\n");
                exit(1);
            }

            if (i != 0) { dup2(prevfd, 0); }
            if (i != conv_size - 1) { dup2(fd[1], 1); }

            close(fd[0]);
            close(fd[1]);
            
            execvp(cur_cmd[0], cur_cmd);
            fprintf(stderr, "Error: cannot execute %s\n", cur_cmd[0]);
            exit(1);
        }

        free(cur_cmd);
        prevfd = fd[0];
        close(fd[1]);
    }

    free(conv);
    free(redirect);
    for (i = 0; i < conv_size - 1; i++) { waitpid(pid_arr[i], NULL, 0); }
    int conv_st;
    waitpid(pid_arr[conv_size - 1], &conv_st, 0);
    if (WIFEXITED(conv_st) == 0) {
        //if (WSTOPSIG(conv_st) == SIGINT);
            return SIGINT;
        //fprintf(stderr, "Error: conveyor executing error\n");
        //return 1;
    }
    else { return WEXITSTATUS(conv_st); }
}


char conditional_cmd(char** list, char background_flag) {
    
    char exit_st = 0;
    char condition = 0;
    char ** cur_cmd;
    int cnt = 0;
    int i;
    int cmd_len = 256;

    while(1) {

        cur_cmd = (char**)malloc(cmd_len * sizeof(char*));
        if(cur_cmd == NULL){
            fprintf(stderr, "An error occured while allocating memory\n");
            return 1;
        }

        i = 0;
        while ((list[cnt] != NULL) && (strcmp(list[cnt], "&&") != 0) && (strcmp(list[cnt], "||") != 0)) {
            cur_cmd[i] = list[cnt];
            i++;
            cnt++;
        }

        cur_cmd[i] = NULL;
        
        if(exit_st == condition) { exit_st = run(cur_cmd, background_flag); }

        if(list[cnt] == NULL) { break; }

        if (strcmp("&&", list[cnt]) == 0) { condition = 0; }
        else { condition = 1; }

        cnt++;
        free(cur_cmd);
    }
    free(cur_cmd);

    return exit_st;
}


char background_mode(char** list) {
    pid_t pid;
    if ((pid = fork()) == 0) {
        if (fork() == 0) {
            int dev_null = open("/dev/null", O_RDWR);
            dup2(dev_null, 0);
            dup2(dev_null, 1);
            close(dev_null);
            conditional_cmd(list, 1);
            exit(0);
        }
        exit(0);
    }
    int st;
    waitpid(pid, &st, 0);
    if(WIFEXITED(st) == 0) { return 1; }
    else { return WEXITSTATUS(st); }
}

char cmd_parce(char ** list) { // parce commands, distribute them between background mode commands and commands with conditional execution

    int cmd_len = 256;
    pid_t pid;

    if ((pid = fork()) == 0) {
        int counter = 0;
        int i, j;
        char exit_st = 0;
        
        while(1) {
            char ** cur_cmd = (char**)malloc(cmd_len * sizeof(char*));
            i = counter;
            j = 0;
            
            while (list[i] != NULL && (strcmp(list[i], "&") != 0) && (strcmp(list[i], ";") != 0)) {
                cur_cmd[j] = list[i];
                i++;
                j++;
            }
            cur_cmd[j] = NULL;
            
            if ((list[i] != NULL) && (strcmp(list[i], "&") == 0)) { exit_st = exit_st | background_mode(cur_cmd); }
            else { exit_st = exit_st | conditional_cmd(cur_cmd, 0); }
            
            i++;
            if (list[i - 1] != NULL) {
                if (list[i] == NULL) {
                    free(cur_cmd);
                    break;
                }
                else { counter = i;}
            }
            else {
                free(cur_cmd);
                break;
            }
            free(cur_cmd);
        }
        exit(exit_st);
    }

    int st;
    waitpid(pid, &st, 0);
    if(WIFEXITED(st) != 0) { return WEXITSTATUS(st); }
    else { return -1; }
}

void deletelst(char ** list){
    int i = 0;
    while (list[i++] != NULL){
        free(list[i - 1]);
    }
    free(list);
}


int main() {

    signal(SIGINT, SIG_IGN);
    char exit_st;
    char ** list;

    while(1) {
        printf("> ");
        list = input();
        if (list != NULL){
            if (strcmp(list[0], "exit") == 0) {
                deletelst(list);
                break;
            }
            
            exit_st = cmd_parce(list);
            if (exit_st && exit_st != SIGINT) {
                if (exit_st == -1) { fprintf(stderr, "Error: invalid command construction\n"); }
                else { fprintf(stderr, "Error: command exit status is %d\n", exit_st); }
            }
            deletelst(list);
        }
    }
}