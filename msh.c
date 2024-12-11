// The MIT License (MIT)
// 
// Copyright (c) 2024 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.


#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 32    

int main(int argc, char *argv[]) 
{
    char *command_string = (char *)malloc(MAX_COMMAND_SIZE);
    char error_message[30] = "An error has occurred\n";

    // directories to search for commands in
    char *directories[] = {"/bin/", "/usr/bin/", "/usr/local/bin/", "./"};
    int num_directories = 4;

    FILE *inputFile = stdin;

    // check for file input for batch mode
    if (argc == 2) 
    {
        inputFile = fopen(argv[1], "r");
        if (inputFile == NULL) 
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }
    } 
    else if (argc > 2) // check if theres too many args
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }

    while (1) 
    {
        // Print out the msh prompt
        if (inputFile == stdin) 
        {
            printf("msh> ");
        }

        // read command
        if(fgets(command_string,MAX_COMMAND_SIZE,inputFile) ==NULL) 
        {
            if (feof(inputFile)) 
            {
                break;
            } 
            else 
            {
                write(STDERR_FILENO, error_message, strlen(error_message));
                continue;
            }
        }

        // remove newline character
        //command_string[strcspn(command_string, "\n")] = '\0';

        char *token[MAX_NUM_ARGUMENTS];
        int token_count = 0;                                 
                                                           
        // Pointer to point to the token
        // parsed by strsep
        char *argument_pointer;                                         
                                                            
        char *working_string  = strdup( command_string );                

        // we are going to move the working_string pointer so
        // keep track of its original value so we can deallocate
        // the correct amount at the end

        // Tokenize the input with whitespace used as the delimiter
        while ( ( (argument_pointer = strsep(&working_string, WHITESPACE ) ) != NULL) && (token_count<MAX_NUM_ARGUMENTS)) 
        {
            if (strlen(argument_pointer) > 0) 
            {
                token[token_count++] = strndup(argument_pointer, MAX_COMMAND_SIZE);
            }
        }
        token[token_count] = NULL; // NULL-terminate for execution

        // if command is empty, continue
        if (token_count == 0) 
        {
            free(working_string);
            continue;
        }

        // built in exit/quit
        if (token[0] != NULL && (strcmp(token[0], "exit") == 0 || strcmp(token[0], "quit") == 0)) 
        {
          if(token_count < 2)
          {
            free(working_string);
            for (int i = 0; i < token_count; i++) 
            {
                free(token[i]);
            }
            exit(0);
          }
          else
          {
            write(STDERR_FILENO, error_message, strlen(error_message));
            continue;
          }
        }

        // built in cd
        if (strcmp(token[0], "cd") == 0) 
        {
            if (token_count > 1) 
            {
                if (chdir(token[1]) != 0) 
                {
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
            } 
            else 
            {
                write(STDERR_FILENO, error_message, strlen(error_message));
            }
            free(working_string);
            continue;
        }

        // check for output redirection
        int redirOutput = 0;
        char *outputFile = NULL;

        for (int i = 0; i < token_count; i++) 
        {
            if (token[i] != NULL && strcmp(token[i], ">") == 0) 
            {
                // make sure theres only one arg after '>'
                if (i+1 >= token_count || token[i+1] == NULL) 
                {
                    // no file specified after '>'
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    goto cleanup;
                }
                if (i+2 < token_count && token[i+2] != NULL) 
                {
                    // more than one arg after '>'
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    goto cleanup;  // Skip executing the command
                }

                redirOutput = 1;
                outputFile = token[i + 1]; // file for redirection
                token[i] = NULL;            // NULL command before '>'
                break;                      // stop after finding redirection
            }
        }

        // if output redirection w/o file
        if (outputFile == NULL && redirOutput) 
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
            continue;
        }

        // buffer for command path
        char commandPath[4096];
        int foundCommand = 0;

        // ssearch for command
        for (int i = 0; i < num_directories; i++) 
        {
            snprintf(commandPath,sizeof(commandPath),"%s%s",directories[i],token[0]);
            if (access(commandPath, X_OK) == 0) 
            {
                // command found
                foundCommand = 1;

                // fork() and execute found command
                pid_t pid = fork();
                if (pid == 0) 
                {
                    // child, handle output redirection
                    if (redirOutput && outputFile != NULL) 
                    {
                        FILE *file = fopen(outputFile, "w");
                        if (file == NULL) 
                        {
                            write(STDERR_FILENO, error_message, strlen(error_message));
                            exit(1);
                        }
                        dup2(fileno(file), STDOUT_FILENO);
                        fclose(file);
                    }

                    if (execv(commandPath, token) == -1) 
                    {
                        write(STDERR_FILENO, error_message, strlen(error_message));
                    }
                    // if exec fails, exit
                    exit(1);
                }
                else if (pid < 0) 
                {
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
                else 
                {
                    // parent, wait for child process to finish
                    int status;
                    wait(&status);
                }
                // once found, stop searching
                break;
            }
        }

        // command not found, error
        if (!foundCommand) 
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
        }

        // memory cleanup
        cleanup:
        free(working_string);
        for (int i = 0; i < token_count; i++) 
        {
            free(token[i]);
        }
        continue; 
    }

    free(command_string);
    return 0;
}
