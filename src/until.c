#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <time.h>

#include "../config.h"
#include "until.h"

struct args global_args;
const char *invoked_as;
char cmdbuf[4096];

void usage();
bool parse_args(int argc, char **argv);
int exec_with_timeout();
char *find_executable_path(char *app);

int main(int argc, char *argv[])
{
   if (!parse_args(argc,argv))
   {
       usage();
       return 1;
   }

   if (global_args.cond == Timeout)
       return exec_with_timeout();

   return 0;
}

bool parse_args(int argc, char **argv)
{
    /* Grab first arg, as the program name */
    invoked_as = *argv;
    argc--; argv++;
    memset(&global_args,0,sizeof(args));
    global_args.cond = Timeout;

    /* If we are out of args now, we didnt really pass anything did we? */
    if (!argc)
    {
        printf("Not enough args\n");
        return false;
    }

    /* Timeout is always first argument, if its not an integer, or 0, thats invalid */
    global_args.timeout = atoi(*argv);

    if (global_args.timeout == 0)
    {
        printf("Invalid integer (%s) for timeout\n",*argv);
        return false;
    }
    argc--; argv++;

    /* Next comes a text string to look for if provided */
    if (strncmp(*argv,"-T",2) == 0)
    {
        global_args.cond = StringOut;
        argc--; argv++;
        if (!argc)
        {
            printf("No option given to -T option\n");
            return false;
        }
        global_args.condData = *argv;
        global_args.condDataLen = strlen(*argv);
        argc--; argv++;
    }

    if (!(global_args.executable = find_executable_path(*argv)))
    {
        printf("%s not found in path\n",*argv);
        return false;
    }

    argc--; argv++;
    while (argc)
    {
        global_args.args = (char **)realloc(global_args.args,(global_args.nargs+1) * sizeof(char *));
        global_args.args[global_args.nargs] = *argv;
        global_args.nargs++;
        argc--;
        argv++;
    }
    return true;
}

void usage()
{
    const char *usageStr =
            PACKAGE_STRING "\n\n"
            "A utility that lets you run a command with a timeout to eliminate hung processes\n"
            "\n"
            "Usage: until (timeout) [options] command args...\n"
            "Where timeout is an integer specifying seconds to wait and options are:\n"
            "-T <text to search for>: Search for text in output of command and cancel countdown if found\n"
            "\n"
            "Examples:\n"
            "until 30 -T \"ogin:\" ssh user@somehost (Try to ssh to somehost, if it sees the string login:, its fine, otherwise kill after 30 seconds)\n"
            "until 300 ./myFtpScript.pl (Run myFtpScript.pl, die off if not done in 5 minutes)\n"
            "";
    printf("%s:%s",invoked_as,usageStr);
}

char *find_executable_path(char *app)
{
    memset(cmdbuf,0,sizeof(cmdbuf));
    const char *path = strdup(getenv("PATH"));
    char *pscan = path;
    char dbuf[1024];
    pscan = strtok(pscan,":");
    if (!pscan)
        return NULL;
    while (pscan)
    {
        snprintf(dbuf,sizeof(dbuf),"%s/%s",pscan,app);
        struct stat s;
        if (stat(dbuf,&s) == 0)
        {
            strncpy(cmdbuf,dbuf,strlen(dbuf)+1);
            return cmdbuf;
        }
        pscan = strtok(NULL,":");
    }
    return NULL;
}

int exec_with_timeout()
{

    pid_t pid = fork();
    if (pid == -1)
    {
        perror("fork()");
        return 1;
    }
    if (pid == 0)
    {
        char **args = (char **)malloc((global_args.nargs+2)*sizeof(char *));
        memset(args,0,(global_args.nargs+2)*sizeof(char *));
        *args = global_args.executable;
        for (int i =0; i < global_args.nargs;i++)
            args[i+1] = global_args.args[i];
        if (execvp(global_args.executable,args))
        {
            perror("execvp");
            exit(1);
            return 0;
        }
    } else {
        int rv;
        time_t exptime = time(NULL);
        exptime += global_args.timeout;
        while (time(NULL) < exptime)
        {
            usleep(100000);
            siginfo_t s;
            s.si_pid = 0;
            if (waitid(P_PID,pid,&s,WNOHANG|WEXITED) == -1)
            {
                perror("waitid");
                return 1;
            }
            if (s.si_pid)
            {
                waitpid(pid,&rv,0);
                return s.si_status;
            }
        }
        if (kill(pid,SIGKILL) != 0)
            perror("kill");
        waitpid(pid,&rv,0);
        printf("until: Operation timed out\n");
    }
    return 1;
}