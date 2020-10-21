#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

#define BUFSIZE 64     // Size token
#define DELIM " \t\r\n\a"  //Delimiters
#define HISTORY_LIMIT 100   //Limit element of history array
#define MAX_LINE 80 /* The maximum length of a command */

void Add_history(char* history[], char* commandline, int* count)
{
	int i;

	for (i = 0; i < HISTORY_LIMIT; i++)
	{
		if (*count >= HISTORY_LIMIT)
		{
			if (i < HISTORY_LIMIT - 1)
			{
				strcpy(history[i], history[i + 1]);
			}

			else
			{
				strcpy(history[i], commandline);
			}
		}
		else if (!strcmp(history[i], "\0"))
		{
			strcpy(history[i], commandline);
			break;
		}
	}

	*count += 1;
}

void IORedirect(char **args, int i, int ioMode)
{
	pid_t pid, wpid;
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	int fd, status = 0;

	pid = fork();
	if (pid == 0)
	{
		if (ioMode == 0)   
			fd = open(args[i + 1], O_RDONLY, mode);
		else              
			fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, mode);
		if (fd < 0)
			fprintf(stderr, "File error");
		else 
		{
			dup2(fd, ioMode);   
			close(fd);          
			args[i] = NULL;
			args[i + 1] = NULL;

			if (execvp(args[0], args) == -1)
			{
				perror("SHELL");
			}
			exit(EXIT_FAILURE);
		}
	}
	else if (pid < 0)
	{
		//Thong bao loi
		perror("SHELL");
	}
	else
	{
		//Doi ket thuc xu li lenh
		do
		{
			wpid = waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}
}

char** CopyArgs(int argc, char **args)
{
	char **copy_of_args = malloc(sizeof(char *)* (argc + 1));
	int i;
	for (i = 0; i < argc; i++)
	{
		copy_of_args[i] = strdup(args[i]);
	}
	copy_of_args[i] = 0;
	return copy_of_args;
}

void PipeRedirect(char **args, int i)
{
	int fpipe[2];
	char **copy_args;

	copy_args = CopyArgs(i, args);
	if (pipe(fpipe) == -1)
	{  
		perror("pipe redirection failed");
		return;
	}

	if (fork() == 0)         
	{
		dup2(fpipe[1], STDOUT_FILENO);        
		close(fpipe[0]);     
		close(fpipe[1]);      

		execvp(copy_args[0], copy_args);    
		perror("First program execution failed");
		exit(1);
	}

	if (fork() == 0)   // child 2
	{
		dup2(fpipe[0], STDIN_FILENO);     
		close(fpipe[1]);      
		close(fpipe[0]);       

		execvp(args[i + 1], args + i + 1);    
		perror("Second program execution failed");
		exit(1);
	}

	close(fpipe[0]);
	close(fpipe[1]);
	wait(0);   // child 1 to finish
	wait(0);   // Wait for child 2
}

//Xu li lenh dau vao
char **HandlerCMD(char *cmd)
{
	int bufsize = BUFSIZE, i = 0;
	char **tokens = malloc(bufsize * sizeof(char*));
	char *token;

	if (!tokens)
	{
		fprintf(stderr, "Memory allocation failure\n");
		exit(EXIT_FAILURE);
	}

	token = strtok(cmd,DELIM);  
	while (token != NULL)
	{
		tokens[i] = token;
		i++;

		if (i >= bufsize) 
		{   
			bufsize += BUFSIZE;
			tokens = realloc(tokens, bufsize * sizeof(char*));
			if (!tokens)
			{
				fprintf(stderr, "memory allocation failure\n");
				exit(EXIT_FAILURE);
			}
		}

		token = strtok(NULL,DELIM);
	}

	tokens[i] = NULL;  
	return tokens;
}

void Process(char **args, int i)
{
	pid_t pid, wpid; 
	int status;

	if (i > 0)
		args[i] = NULL; 
	pid = fork();

	if (pid == 0)        
	{
		execvp(args[0], args);
		perror("Program execution failed");
		if (i == 0)
			exit(1);
	}

	if (i == 0)
	{
		do
		{
			wpid = waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}
	else
	{
		// Dau nhay doi lenh
		fprintf(stderr, "Background process: ");
		wpid = waitpid(-1, &status, WNOHANG);
	}
}

int main()
{
	char *commandline = NULL, *pwd = NULL;
	char **args, *options[4] = { "<", ">", "|", "&" };
	int i = 0, j = 0, choose, found, history_count = 0;
	char* history[HISTORY_LIMIT];

	for (j = 0; j < HISTORY_LIMIT; j++)
	{
		history[j] = malloc(MAX_LINE);
		strcpy(history[j], "\0");
	}

	// Get pwd
	pwd = getenv("HOME");

	int should_run = 1; /* flag to determine when to exit program */

	while (should_run)
	{
		ssize_t bsize = 0;
		found = 0;

		printf("osh->");
		getline(&commandline, &bsize, stdin);

		//Xu li, chia lenh nguoi dung nhap
		args = HandlerCMD(commandline);             

		if (commandline == NULL)
		{
			free(commandline);
			free(args);
			continue;
		}

		//Nhap "exit"
		if (strcmp(commandline, "exit") == 0)
			break;

		//NHap cd
		else if (strcmp(commandline, "cd") == 0)
		{
			if (args[1] == NULL)
			{
				if (pwd[0] != 0)
				{
					chdir(pwd); 
				}
			}
			else
			{
				chdir(args[1]);
			}
		}
		
		//Nhap history
		else if (!strcmp(commandline, "history"))
		{
			for (j = HISTORY_LIMIT - 1; j >= 0; j--)
			{
				if (!strcmp(history[j], "\0"))
				{
					continue;
				}
				printf("%i %s\n", j + 1 + (history_count > HISTORY_LIMIT ? history_count - HISTORY_LIMIT : 0), history[j]);
			}
			continue;
		}

		//Nhap "!!"
		else if (!strcmp(commandline, "!!"))
		{
			if (history_count == 0)
			{
				printf("No commands in history !!\n");
				continue;
			}
			else
			{
				strcpy(commandline, history_count > HISTORY_LIMIT ? history[HISTORY_LIMIT - 1] : history[history_count - 1]);
				printf("%s\n", commandline);
				Process(args, 0);                      
			}
		}
		//Nhap <,>,|,&
		else
		{
			i = 1;
			while (args[i] != NULL)
			{
				for (choose = 0; choose < 4; choose++)
				{
					if (strcmp(args[i], options[choose]) == 0)
						break;
				}
				if (choose < 4)
				{
					found = 1;
					if (choose < 3 && args[i + 1] == NULL)
					{
						fprintf(stderr, "SHELL: parameter missing\n");
						break;
					}
					if (choose < 2)
						IORedirect(args, i, choose);
					else if (choose == 2)
						PipeRedirect(args, i);
					else if (choose == 3)
						Process(args, i);
					break;
				}

				i++;
			}

			if (found == 0)
				Process(args, 0);
		}

		Add_history(history, commandline, &history_count);

		free(commandline);
		free(args);
	}
	return 0;
}
