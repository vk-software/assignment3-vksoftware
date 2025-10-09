
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/wait.h>
#include <sys/types.h>

#include "systemcalls.h"



/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/
    int rc = system(cmd);

    if (rc == -1) {
        return false;
    }

    return true;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

    pid_t pid = fork();
    bool result = true;
    int status = -1;
    
    if (pid == -1) {
        result = false;
    }
    
    else if (pid == 0) {
        execv(command[0], command);
        // exec* functions only return if an error has occurred
        exit(-1);
    }
    else {
        pid = waitpid(pid, &status, 0);
        if (pid == -1) {
            result = false;
        }
        
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            result = false;
        }
    }

    va_end(args);

    return result;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    
    int rc = -1;
    bool result = true;
    int status = -1;
    
    int fd = open(outputfile, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    
    if (!fd) {
        result = false;
    }
    
    pid_t pid = fork();

    
    if (pid == -1) {
        result = false;
    }
    
    else if (pid == 0) {
        rc = dup2(fd, 1);
        if (rc == -1) {
            result = false;
        }
        execv(command[0], command);
        // exec* functions only return if an error has occurred
        close(fd);
        exit(-1);
    }
    else {
        pid = waitpid(pid, &status, 0);
        if (pid == -1) {
            result = false;
        }
        
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            result = false;
        }
    }

    va_end(args);
    close(fd);

    return result;
}
