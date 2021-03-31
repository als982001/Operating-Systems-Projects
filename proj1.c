#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_LINE 80
#define MAX_INPUT_NUM 10

int times = 0;

int main(int argc, char* args[MAX_LINE])
{
	int should_run = 1;

	pid_t pid;

	while (should_run)
	{
		++times;			// A variable for running program 'times' times.
		int len;			// A variable for representing length of inputted sentence.
		int concurrent = 0; // A variable for running cuncurrently.
		int fd[2];			// A variable for pipe
		int fdr;			// A variable for function 'dup2'
		int status;

		char input_sentence[MAX_LINE];	// A array for inputted sentence.
		char* temp[2];
		char* front[MAX_LINE];			// This is for saving first command when sentence has '>', '<' or '|'
		char* back[MAX_LINE];			// This is for saving back command too.
		char special_character = 'N';	// A variable for check special character(>, <, |)
										// 'N' means that there is no special character at sentence.

		printf("osh>");
		fflush(stdout);

		fgets(input_sentence, MAX_LINE, stdin);
		len = strlen(input_sentence);
		input_sentence[len - 1] = '\0';

		argc = 0;
		int spc_num = 0;

		// Remove all spacing and paste the sentences.
		for (int k = 0; k < len - 1; ++k)
		{
			if (input_sentence[k] == ' ' && input_sentence[k + 1] != '-')
			{
				for (int m = k; m < len - 1; ++m)
				{
					input_sentence[m] = input_sentence[m + 1];
				}
				--len;
			}
		}

		// When the end of sentence is '&', this means that running concurrently at background.
		if (input_sentence[len - 2] == '&')
			concurrent = 1;

		// If there is a special character '>'
		if (strchr(input_sentence, '>') != NULL)
		{
			special_character = '>';
			temp[0] = strtok(input_sentence, ">");
			temp[1] = strtok(NULL, ">");

			front[0] = strtok(temp[0], " ");
			front[1] = strtok(NULL, " ");
			front[2] = (char*)0;

			/*
			back[0] = temp[1];
			back[1] = (char*)0;
			*/

			back[0] = strtok(temp[1], "&");
			back[1] = (char*)0;
		}
		// Else if there is a special character '<'
		else if (strchr(input_sentence, '<') != NULL)
		{
			special_character = '<';
			temp[0] = strtok(input_sentence, "<");
			temp[1] = strtok(NULL, "<");

			front[0] = strtok(temp[0], " ");
			front[1] = strtok(NULL, " ");
			front[2] = (char*)0;

			back[0] = strtok(temp[1], "&");
			back[1] = (char*)0;
		}
		// Else if there is a special character '|'
		else if (strchr(input_sentence, '|') != NULL)
		{
			special_character = '|';
			temp[0] = strtok(input_sentence, "|");
			temp[1] = strtok(NULL, "|");

			front[0] = strtok(temp[0], " ");
			front[1] = strtok(NULL, " ");
			front[2] = (char*)0;

			back[0] = strtok(temp[1], " ");
			back[1] = strtok(NULL, " ");
			back[2] = (char*)0;
		}
		// Else if there is no special character
		else
		{
			if (strchr(input_sentence, '-') == NULL)
			{
				front[0] = strtok(input_sentence, "&");
				front[1] = (char*)0;
			}
			else
			{
				front[0] = strtok(input_sentence, " ");
				front[1] = strtok(NULL, "&");
				front[2] = (char*)0;
			}
		}
			
		
		
		// The case is divided into two categories.
		// The first is when there is a special character '|'
		// and the secoid is otherwise.
		// Because others need a 'fork()' once, but '|' needs a fork() twice.
		if (special_character == '|')
		{
			// At first, make pipe
			pipe(fd);

			// Do 'fork()'
			// If pid == 0, this means that this process is 'child'.
			if ((pid = fork()) == 0)
			{
				
				close(1);					// At first, close 1(stdout)
				dup(fd[1]);					// And duplicate fd[1] (write)
				close(fd[0]);				// And close fd[0] and fd[1]
				close(fd[1]);
				execvp(front[0], front);	// Do command that is before '|'.

				exit(0);
			}

			// Do pid = fork() again.
			// This process is the opposite of the above.
			if ((pid = fork()) == 0)
			{
				close(0);
				dup(fd[0]);
				close(fd[0]);
				close(fd[1]);
				execvp(back[0], back);
				exit(0);
			}

			// After of all, close pipe.
			close(fd[0]);
			close(fd[1]);

			// And wait.
			while (wait(NULL) != -1)
			{
				continue;
			}
		}

		// This is not the case with '|'.
		else
		{
			pid = fork();	// At first, do 'pid = fork()'.
			
			// If pid == 0, this means that this proces is child.
			if (pid == 0)
			{
				// This is for checking background running.
				/*
				srand((unsigned int)time(NULL));
				int random = rand();

				for (int i = 0; i < 5; ++i)
				{
					printf("I am Child %d !!!\n", random);
					sleep(1);
				}
				*/

				// This case is that there was not special character like '<', '>', '|'.
				// At this case, just have to run command.
				if (special_character == 'N')
				{
					execvp(front[0], front);
					exit(1);
				}

				// And this case it that there was a special character, '>'.
				else if (special_character == '>')
				{
					// At first, for redirecting output, it is necessary to do function 'open' and 'dup2'
					fdr = open(back[0], O_WRONLY | O_CREAT | O_TRUNC, 0644);
					if (fdr == -1)
					{
						exit(1);
					}

					dup2(fdr, 1);

					close(fdr);	
					execvp(front[0], front);	// And, do the command before '>'
					exit(0);
				}
				// This is the last case, special_character '<'.
				else if (special_character == '<')
				{
					// On the contrary, it is necessary to do 'open' front command.
					fdr = open(back[0], O_RDWR);
					if (fdr == -1)
					{
						exit(1);
					}
		
					dup2(fdr, 0);

					close(fdr);
					execvp(front[0], front);
					exit(0);
				}
			}
			// If pid is over 0, this means that this is parent.
			else  if (pid > 0)
			{
				if (concurrent == 0)
				{
					wait(&status);
				}
			}
		}
	}

	return 0;
}