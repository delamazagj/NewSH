//
//  newshell.c
//  It creates a basic shell that support all commands and the built-ins cd, pwd, history, and exit. Also, it
//  supports up to 2 pipes per command line and redirection.
//  In interactive mode, the code takes the commands and options from stdin and executes them in a child processes. To terminate and exit the shell, just type exit
//  In batch mode the code reads the command lines from a file specified as a command line parameter
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

char path[1024]; //current directory

/*
 * modifies the current command line to include white spaces before and after 
 * special characters such as <, >, and | 
 * returns the number of pipes present in the command line
 */
int handleCmdLine(char *commandBuff, char *aux)
{

    int i = 0, j, count = 0;
    int s = strlen(commandBuff);
    for (j = 0; j < s; j++)
    {
        //if any special character is found then spaces are added before and after
        if (commandBuff[j] == '|' || commandBuff[j] == '<' || commandBuff[j] == '>')
        {
            aux[i++] = ' ';
            aux[i++] = commandBuff[j];
            aux[i++] = ' ';
            if (commandBuff[j] == '|')
                count++; //for counting the pipes
        }
        else
            aux[i++] = commandBuff[j];
    }
    aux[i++] = '\n'; //adds a new line char at the end to use it as a token later
    aux[i] = '\0';   // null char at the end
    return count;    //returns number of pipes
}

/*
 * breaks down the command line into different command lines
 * uses ; as delimiter
 * returns number of commands
 * commandBuff stores all different command lines
*/
int nOfCommands(char *buff, char **commandBuff)
{
    int i = 0;
    char *tok;

    tok = strtok(buff, ";\n");
    while (tok != NULL) //if there is input
    {
        commandBuff[i++] = tok;

        tok = strtok(NULL, ";\n");
    }
    if (i != 0)
        return i;
    return 0;
}

/*
 * breaks down the commands inside the pipes 
 * commandBuff is a command line
 * commandPipes will hold all commmands inside the pipes
 */
void getCommandsPipes(char *commandBuff, char **commandPipes)
{
    int i;

    char *tok;
    i = 0;

    tok = strtok(commandBuff, "|\n");
    while (tok != NULL) //if there is input
    {
        commandPipes[i++] = tok;
        tok = strtok(NULL, "|\n");
    }
}

/*
 * adds the new line char at the end of a simple command
 * it first copies the command and options into anothe string 
 * and then adds the next line char and a null char at the end 
 */
void addNextLine(char *cmd, char *simpleCommand)
{
    int i, s = strlen(simpleCommand);
    for (i = 0; i < s; i++)
        cmd[i] = simpleCommand[i];
    cmd[s++] = '\n';
    cmd[s] = '\0';
}

/*
 * takes a string containing the command and options
 * and breaks it down so that the command and options are 
 * stored in an array, cmd.
 * it also checks for redirection files
 * 
 */
void getCommand(int fout, int fin, char **cmd, char *simpleCommand)
{
    int i = 0;
    char *tok = strtok(simpleCommand, " \n");
    while (tok != NULL)
    {
        if (strcmp(tok, ">") == 0) // if and output file is present
        {
            tok = strtok(NULL, " \n"); // gets the output file
            //creates or modifies the file
            fout = open(tok, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
            dup2(fout, fileno(stdout)); //will write to file instead of stdout
            close(fout);
        }

        else if (strcmp(tok, "<") == 0) // if and input file is present
        {
            tok = strtok(NULL, " \n"); // gets the input file
            fin = open(tok, O_RDONLY); //opens for reading only
            dup2(fin, fileno(stdin));  //will take input from file instead of stdin
            close(fin);
        }

        else
            cmd[i++] = tok;

        tok = strtok(NULL, " \n");
    }
    cmd[i] = NULL; //adds NULL at the end to be use for execvp
}

/*
 * function for when one pipe is present, two commands
 * commandPipes stores the commands and their options
 */
void pipe1(char **commandPipes)
{
    char cmd[1024], cmd2[1024]; //to store the commands and their options in strings
    //add new line char to each string containing commands with options
    addNextLine(cmd, commandPipes[0]);
    addNextLine(cmd2, commandPipes[1]);
    //commands and options array to be use in execvp
    char *command1[1024];
    char *command2[1024];

    int fd[2]; //array for the pipe
    pipe(fd);  //creating the pipe
    pid_t pid;
    pid = fork();
    if (pid < 0) //if there is an error forking
    {
        fprintf(stderr, "Fork Error\n");
        exit(1);
    }
    else if (pid == 0) // first child ->  first command
    {
        int fout = -1, fin = -1; //to be used if redirection is present

        //gets the command and its parameters with redirection files if present
        getCommand(fout, fin, command1, cmd);

        dup2(fd[1], fileno(stdout)); // write end of pipe will be stdout
        close(fd[0]);
        close(fd[1]);
        execvp(command1[0], command1);                           //executes the command
        fprintf(stderr, "%s: command not found\n", command1[0]); //if command is not found

        exit(1);
    }
    else
    {
        pid = fork();
        if (pid < 0) //if there is an error forking
        {
            fprintf(stderr, "Fork Error\n");
            exit(1);
        }
        else if (pid == 0) //second child -> second command
        {
            int fout2 = -1, fin2 = -1; //to be used if redirection is present

            //gets the command and its parameters with redirection files if present
            getCommand(fout2, fin2, command2, cmd2);

            dup2(fd[0], fileno(stdin)); // read end of pipe will be stdin
            close(fd[0]);
            close(fd[1]);
            execvp(command2[0], command2);                           //executes the command
            fprintf(stderr, "%s: command not found\n", command2[0]); //if command is not found

            exit(1);
        }
    }
    close(fd[0]);
    close(fd[1]);
    int i;
    for (i = 0; i < 2; i++) //parent process waits for 2 childs
    {
        wait(NULL);
    }
}

/*
 * function for when 2 pipes are present, three commands
 * commandPipes stores the commands and their options
 */
void pipe2(char **commandPipes)
{
    char cmd[1024], cmd2[1024], cmd3[1024]; //to store the commands and their options in strings
    //add new line char to each string containing commands with options
    addNextLine(cmd, commandPipes[0]);
    addNextLine(cmd2, commandPipes[1]);
    addNextLine(cmd3, commandPipes[2]);
    //commands and options array to be use in execvp
    char *command1[1024];
    char *command2[1024];
    char *command3[1024];

    int fd1[2], fd2[2];
    pipe(fd1);
    pipe(fd2);
    pid_t pid = fork();
    if (pid < 0) //if there is an error forking
    {
        fprintf(stderr, "Fork Error\n");
        exit(1);
    }
    else if (pid == 0) //first child -> first command
    {
        int fout = -1, fin = -1; //to be used if redirection is present
        //gets the commands and its options with redirection files
        getCommand(fout, fin, command1, cmd);

        dup2(fd1[1], fileno(stdout)); // write end of pipe will be stdout
        close(fd1[0]);
        close(fd1[1]);
        close(fd2[0]);
        close(fd2[1]);
        execvp(command1[0], command1);                           //executes the command
        fprintf(stderr, "%s: command not found\n", command1[0]); //if command is not found

        exit(1);
    }
    else
    {
        pid = fork();
        if (pid < 0) //if there is an error forking
        {
            fprintf(stderr, "Fork Error\n");
            exit(1);
        }
        else if (pid == 0) //second child -> second command
        {
            int fout2 = -1, fin2 = -1; //to be used if redirection is present

            //gets the commands and its options with redirection files
            getCommand(fout2, fin2, command2, cmd2);

            dup2(fd1[0], fileno(stdin));  // read end of pipe will be stdin
            dup2(fd2[1], fileno(stdout)); // write end of pipe 2 will be stdout
            close(fd1[0]);
            close(fd1[1]);
            close(fd2[0]);
            close(fd2[1]);
            execvp(command2[0], command2);                           //executes the command
            fprintf(stderr, "%s: command not found\n", command2[0]); //if command is not found

            exit(1);
        }
        else
        {
            pid = fork();
            if (pid < 0) //if there is an error forking
            {
                fprintf(stderr, "Fork Error\n");
                exit(1);
            }
            else if (pid == 0) //third child -> third command
            {
                int fout3 = -1, fin3 = -1; //to be used if redirection is present

                //gets the commands and its options with redirection files
                getCommand(fout3, fin3, command3, cmd3);

                dup2(fd2[0], fileno(stdin)); // read end of pipe 2 will be stdin
                close(fd1[0]);
                close(fd1[1]);
                close(fd2[0]);
                close(fd2[1]);
                execvp(command3[0], command3);                           //executes the command
                fprintf(stderr, "%s: command not found\n", command3[0]); //if command is not found

                exit(1);
            }
        }
    }
    close(fd1[0]);
    close(fd1[1]);
    close(fd2[0]);
    close(fd2[1]);
    int i;
    for (i = 0; i < 3; i++) //parent process waits for 3 childs
    {
        wait(NULL);
    }
}

/*
 * function for when no pipe is present, one commands
 * commandPipes stores the commands and their options
 */
void pipe0(char **commandPipes)
{
    char cmd[1024];                    //for holding the command
    addNextLine(cmd, commandPipes[0]); //adds new line char at the end of command
    char *command1[1024];              //command array for execvp

    pid_t pid = fork();
    if (pid < 0) //if there is an error forking
    {
        fprintf(stderr, "Fork Error\n");
        exit(1);
    }
    else if (pid == 0) //child
    {
        int fout = -1, fin = -1;              //for redirection
        getCommand(fout, fin, command1, cmd); //gets the command with options and redirection

        execvp(command1[0], command1);                           //executes the command
        fprintf(stderr, "%s: command not found\n", command1[0]); //if command is not found

        exit(1);
    }
    else
    {
        wait(NULL); //parent process waits for child to finish
    }
}

/*
 * gets the current directory
 * and prints it
 */
void getPwd()
{
    getcwd(path, sizeof(path));
    printf("%s\n", path);
}

/*
 * sets the current directory to the content of add
 * and updates path
 */
void setCd(char *add)
{
    chdir(add);
    getcwd(path, sizeof(path));
}

/*
 * gets a simple command
 */
int getSimpleCommand(char **simplecommand, char *basic)
{
    int i = 0;
    char *tok = strtok(basic, " \n");
    while (tok != NULL)
    {
        simplecommand[i++] = tok;
        tok = strtok(NULL, " \n");
    }
    return i;
}

//prints the name of the shell in a friendly way
void greetings()
{
    sleep(1);
    printf("                            __         ____\n");
    sleep(1);
    printf("   ____  ___ _      _______/ /_  ___  / / /\n");
    sleep(1);
    printf("  / __ \\/ _ \\ | /| / / ___/ __ \\/ _ \\/ / / \n");
    sleep(1);
    printf(" / / / /  __/ |/ |/ (__  ) / / /  __/ / / \n");
    sleep(1);
    printf("/_/ /_/\\___/|__/|__/____/_/ /_/\\___/_/_/ \n\n");
    sleep(1);
}

//main function
int main(int argc, char *argv[])
{

    //char history[256][1000]; //history
    //int commandIndex = 0;
    pid_t parent;
    struct sigaction act;
    char basic[1024];
    char *simplecommand[1024];
    char *commandBuff[1024]; //stores command lines
    char *commandPipes[3];   //stores commands in between pipes
    char modifiedCMD[1024];  //stores modified input with spaces before and after special characters
    char promt[512];         //the buffer to store the input from the user
    char his[1000][512];
    int i, hisCount = 0, flag = 0; //flag to check if exit command was typed

    if (argc == 2) //batch mode
    {
        FILE *fp = fopen(argv[1], "r"); //opens for reading only
        if (fp == NULL)
        {
            fprintf(stderr, "Couldn't open file for reading");
            exit(2);
        }

        while (fgets(promt, 512, fp) && flag == 0) //reads until EOF or exit cmd
        {
            printf("%s", promt);
            strcpy(his[hisCount++], promt); //copies to history
            int nOfc = nOfCommands(promt, commandBuff);
            if (nOfc != 0)
            {
                for (i = 0; i < nOfc; i++)
                {
                    //resets memory
                    memset(commandPipes, '\0', sizeof(commandPipes));
                    memset(basic, '\0', sizeof(promt));
                    memset(simplecommand, '\0', sizeof(simplecommand));
                    getcwd(path, sizeof(path));

                    //returns the number of pipes
                    int nOfPipes = handleCmdLine(commandBuff[i], modifiedCMD);

                    strcpy(basic, modifiedCMD); //copies the modified cmd to basic

                    //gets the simple command
                    int simple = getSimpleCommand(simplecommand, basic);

                    if (simple == 1 && strcmp(simplecommand[0], "exit") == 0) //if exit was typed then change flag
                        flag = 1;
                    else if (simple == 1 && strcmp(simplecommand[0], "history") == 0) //built in history
                    {
                        int count;
                        for (count = 0; count < hisCount; count++)
                            printf("%d\t%s", count + 1, his[count]);
                    }
                    else if (simple == 1 && strcmp(simplecommand[0], "pwd") == 0) //built in pwd
                        getPwd();

                    //cd with no path sets current directory to home
                    else if (simple == 1 && strcmp(simplecommand[0], "cd") == 0)
                        setCd(getenv("HOME"));
                    else if (simple > 1 && strcmp(simplecommand[0], "cd") == 0) //built in cd
                        setCd(simplecommand[1]);
                    else
                    {

                        char *aux = modifiedCMD;
                        //gets the commands inside pipes
                        getCommandsPipes(aux, commandPipes);

                        if (nOfPipes == 0)
                        {
                            pipe0(commandPipes); //no pipes present
                        }
                        else if (nOfPipes == 1)
                        {
                            pipe1(commandPipes); //for 1 pipe
                        }
                        else if (nOfPipes == 2)
                        {
                            pipe2(commandPipes); //for 2 pipes
                        }
                        else
                        {
                            //maximum of 2 pipes accepted by the shell
                            printf("This shell only accepts a maximum of 2 pipes per command line\n");
                        }
                    }
                }
            }
            //resets memory
            memset(promt, '\0', sizeof(promt));
            memset(commandBuff, '\0', sizeof(commandBuff));
        }
        fclose(fp);
    }
    else if (argc == 1) //interactive mode
    {
        greetings();
        printf("\e[1;1H\e[2J"); //clears the screen
        while (flag == 0)
        {

            tcsetpgrp(fileno(stdin), getpgrp());
            act.sa_handler = SIG_IGN;
            int sig;
            for (sig = 1; sig < 28; sig++)
            {
                if (sig != 9 && sig != 17 && sig != 19 && sig != 23)
                    assert(sigaction(sig, &act, NULL) == 0);
            }

            printf("newshell>> ");
            fgets(promt, 512, stdin); //reads the whole line from stdin

            strcpy(his[hisCount++], promt); //copies the line to history

            //gets the commands lines separated by ;
            //nOfc holds the numbers of command lines
            int nOfc = nOfCommands(promt, commandBuff);

            if (nOfc != 0)
            {
                for (i = 0; i < nOfc; i++)
                {
                    //resets memory
                    memset(commandPipes, '\0', sizeof(commandPipes));
                    memset(basic, '\0', sizeof(promt));
                    memset(simplecommand, '\0', sizeof(simplecommand));
                    getcwd(path, sizeof(path));

                    //gets the number of pipes
                    int nOfPipes = handleCmdLine(commandBuff[i], modifiedCMD);

                    strcpy(basic, modifiedCMD); //copies the modified command to basic

                    int simple = getSimpleCommand(simplecommand, basic);      //gets the number of simple commands
                    if (simple == 1 && strcmp(simplecommand[0], "exit") == 0) //if exit was typed then change flag
                        flag = 1;
                    else if (simple == 1 && strcmp(simplecommand[0], "history") == 0) //history builtin
                    {
                        int count;
                        for (count = 0; count < hisCount; count++)
                            printf("%d\t%s", count + 1, his[count]);
                    }

                    else if (simple == 1 && strcmp(simplecommand[0], "pwd") == 0) //pwd builtin
                        getPwd();

                    //cd with no path sets current directory to home
                    else if (simple == 1 && strcmp(simplecommand[0], "cd") == 0)
                        setCd(getenv("HOME"));
                    else if (simple > 1 && strcmp(simplecommand[0], "cd") == 0) //cd builtin
                        setCd(simplecommand[1]);
                    else
                    {
                        parent = fork();
                        if (parent < 0)
                        {
                            //forking error
                            fprintf(stderr, "Fork Error\n");
                            exit(2);
                        }
                        else if (parent == 0)
                        {
                            setpgrp();
                            act.sa_handler = SIG_DFL;
                            for (sig = 1; sig < 28; sig++)
                            {
                                //enables signals for child
                                if (sig != 9 && sig != 17 && sig != 19 && sig != 23)
                                    assert(sigaction(sig, &act, NULL) == 0);
                            }

                            char *aux = modifiedCMD;
                            getCommandsPipes(aux, commandPipes); //gets the commands inside pipes

                            if (nOfPipes == 0)
                            {
                                //there is no pipe
                                pipe0(commandPipes);
                            }
                            else if (nOfPipes == 1)
                            {
                                //1 pipe found
                                pipe1(commandPipes);
                            }
                            else if (nOfPipes == 2)
                            {
                                //two pipes found
                                pipe2(commandPipes);
                            }
                            else
                            {
                                //this shell only accepts a maximum of 2 pipes
                                fprintf(stderr, "This shell only accepts a maximum of 2 pipes per command line\n");
                            }
                            exit(1);
                        }
                        //ignores signals for parent
                        setpgid(parent, parent);
                        signal(SIGTTOU, SIG_IGN);
                        tcsetpgrp(fileno(stdin), getpgid(parent));
                        wait(NULL); //wait for child to finish
                    }
                }
            }

            //resetting memory
            memset(promt, '\0', sizeof(promt));
            memset(commandBuff, '\0', sizeof(commandBuff));
        }
    }
    else
    {
        //if more than one argument is specified in command line
        fprintf(stderr, "Too many arguments specified!\n");
        fprintf(stderr, "usage: %s [batch file]\n", argv[0]);
    }

    return 0;
}
