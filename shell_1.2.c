#include<stdio.h>
#include<stdarg.h>
#include<unistd.h> /* fork() */
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/wait.h> /* wait() */
#include<errno.h>
#include<dirent.h>
#include<readline/readline.h>
#include<readline/history.h>

#define BUFF 100
#define EXIT "exit"
#define RED "\x1b[91m"
#define GREEN "\x1b[92m"
#define BLUE "\x1b[94m"
#define WHITE "\x1b[97m"

void parse(char cmd[]);
int shell_execute(char *cmd_tokens[]);
int shell_external(char *cmd_tokens[]);
void pipe_cmd(int num,char *commands[][10]);
void bg_execute(char *cmd_tokens[]);

int main(int argc,char *argv[])
{
	char *username = getenv("USER");
	char cwd[1000];
	while(argc!=1){
		char cmd[1000];
		FILE *fp;
		fp = fopen(argv[argc-1],"r");
		if(fp!=NULL)
			while(fgets(cmd,BUFF,fp)){
				getcwd(cwd,sizeof(cwd));
				char prompt[1000] = GREEN;
				strcat(prompt,username);
				strcat(prompt,":~");
				strcat(prompt,BLUE);
				strcat(prompt,cwd);
				strcat(prompt,"$ ");			
				cmd[strlen(cmd)-1] = '\0';
				strcat(prompt,WHITE);				
				printf("%s"RED"%s"WHITE"\n",prompt,cmd);
				add_history(cmd);			
				parse(cmd);
			}
		else printf("Error opening File!");
		argc--;
	}
	char *cmd;
	while(1){
		/* Prompt Display*/
		getcwd(cwd,sizeof(cwd));
		char prompt[1000] = GREEN;
		strcat(prompt,username);
		strcat(prompt,":~");
		strcat(prompt,BLUE);
		strcat(prompt,cwd);
		strcat(prompt,"$ ");
		strcat(prompt,WHITE);
		/* input the command */
		cmd = readline(prompt);
		if(cmd[0]!=0) add_history(cmd);
		/* if user entered exit, then EXIT */
		if(strcmp(cmd, EXIT) == 0)	break;
		/* Parse the command and execute it */
		else  parse(cmd);
		free(cmd);
	}
	return 0;
}

/* Parses a Command */
void parse(char cmd[])
{	
	char* token_cnt;
	char* rest_cnt = cmd;
	int cnt_type = 1;
	for(int j=0;j<strlen(cmd);j++){
		if(cmd[j]=='&'){
			if(j+1<strlen(cmd)&&cmd[j+1]=='&')
				cnt_type = 2;
			else
				cnt_type = 3;
			break;
		}
		if(cmd[j]=='|'){
			if(j+1<strlen(cmd)&&cmd[j+1]=='|')
				cnt_type = 4;
			else
				cnt_type = 5;
			break;
		}
	}
	if(cnt_type==1){
		while(token_cnt = strtok_r(rest_cnt,";",&rest_cnt)){	
			char* commands[10];
			char* token;		
			int i;
 			for(i=0; (token = strtok_r(token_cnt, " ", &token_cnt)); i++) commands[i] = token;
			commands[i] = '\0';
			shell_execute(commands);
		}
	}
	else if(cnt_type==2){
		while(token_cnt = strtok_r(rest_cnt,"&&",&rest_cnt)){	
			char* commands[10];
			char* token;
			int i;
 			for(i=0; (token = strtok_r(token_cnt, " ", &token_cnt)); i++) commands[i] = token;
			commands[i] = '\0';
			int exit = shell_execute(commands);
			if(exit!=0) break;
		}
	}
	else if(cnt_type==3){
		if(token_cnt = strtok_r(rest_cnt,"&",&rest_cnt)){	
			char* commands[10];
			char* token;
			int i;
 			for(i=0; (token = strtok_r(token_cnt, " ", &token_cnt)); i++) commands[i] = token;
			commands[i] = '\0';
			bg_execute(commands);
		}
	}
	else if(cnt_type==4){
		while(token_cnt = strtok_r(rest_cnt,"||",&rest_cnt)){	
			char* commands[10];
			char* token;
			int i;	
 			for(i=0; (token = strtok_r(token_cnt, " ", &token_cnt)); i++) commands[i] = token;
			commands[i] = '\0';
			int exit = shell_execute(commands);
			if(exit==0) break;
		}
	}	
	else if(cnt_type==5){
		char *commands[10][10];
		int j=0;
		while(token_cnt = strtok_r(rest_cnt,"||",&rest_cnt)){	
			char* token;
			int i;	
 			for(i=0; (token = strtok_r(token_cnt, " ", &token_cnt)); i++) commands[j][i] = token;
			commands[j][i] = '\0';
			j+=1;
		}
		printf("%d\n",j);
		pipe_cmd(j,commands);		
	}
}

/* if command internal execute, else send to shell_external */
int shell_execute(char *cmd_tokens[])
{ 	 
	int status;
	if(strcmp(cmd_tokens[0],"help") == 0)
	{
		printf("LIST OF SUPPORTED COMMANDS: \n");
		printf("Internal Commands-> cd\tpwd\tclear\treset\n");
		printf("External Commands-> /bin/ls\t/bin/date\t...\n");
		status = 1;	
	}	
	else  if(strcmp(cmd_tokens[0],"cd") == 0)
	{
		status = chdir(cmd_tokens[1]);
		if(status==-1){
			perror("cd command failed");
			status = -1;
		}	
	}
	else if(strcmp(cmd_tokens[0],"pwd") == 0)
	{
		char pwd[1024];
		getcwd(pwd,sizeof(pwd));
		printf("%s\n",pwd);
		if(status==-1){
			perror("pwd command failed");
			status = -1;
		}
	}
	else if(strcmp(cmd_tokens[0],"reset") == 0)
	{
		system("reset");
		if(status==-1){
			perror("reset command failed");
			status = -1;
		}
	}
	else if(strcmp(cmd_tokens[0],"clear") == 0)
	{
		system("clear");
		if(status==-1){
			perror("clear command failed");
			status = -1;
		}
	}
	else status = shell_external(cmd_tokens);
	return status;		
}

int shell_external(char *cmd_tokens[]){
	int status;	
	pid_t p;
	p = fork();
	if(p==0)	
		if(execvp(cmd_tokens[0],cmd_tokens)==-1){
			perror("Exec Failed.");
			exit(1);
		}
	wait(&status);
	return status;
}


void pipe_cmd(int num, char *commands[][10]){
	int new_fds[2];
	int old_fds[2];	
	int cmd_num = 0,j = 0,i=0;
    while(cmd_num<num){
		pipe(new_fds);
		if(fork()==0) {
			 //if there is a previous command
			if(cmd_num!=0){
				dup2(old_fds[0],0);
				close(old_fds[0]);
				close(old_fds[1]);
			}
			//if there is a next command
			if(cmd_num!=num-1){
				dup2(new_fds[1],1);
				close(new_fds[0]);
				close(new_fds[1]);
			}
			if(execvp(commands[cmd_num][0],commands[cmd_num])==-1){
				perror("Exec Failed.");
				exit(1);
			}
		} else {
			//if there is a previous command
			if(cmd_num!=0){
				close(old_fds[0]);
				close(old_fds[1]);
			}
			//if there is a next command
			if(cmd_num!=num-1){
				old_fds[0] = new_fds[0];
				old_fds[1] = new_fds[1];
			}
		}
		cmd_num+=1;
	}
	if(num>1){
		close(old_fds[0]);
		close(old_fds[1]);
	}
	for(i=0;i<num;i++) wait(NULL);
}


void bg_execute(char *cmd_tokens[]){
	int status;	
	pid_t p;
	p = fork();
	if(p==0){
		setpgid(p,1); 
		if(execvp(cmd_tokens[0],cmd_tokens)==-1){
			perror("Exec Failed.");
			exit(1);
		}
	}
	waitpid(-1,&status, WNOHANG);
}
