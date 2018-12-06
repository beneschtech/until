#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <time.h>
#include <poll.h>

#include "../config.h"
#include "until.h"

struct args global_args;
const char *invoked_as;
char cmdbuf[4096];

void usage();
bool parse_args(int argc, char **argv);
int exec_with_timeout();
int exec_match_text();
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

   if (global_args.cond == StringOut)
       return exec_match_text();

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
    char *path = strdup(getenv("PATH"));
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

int exec_match_text()
{
    int pipefds[2];
    if (pipe(pipefds) != 0)
    {
        perror("pipe()");
        return 1;
    }
    pid_t pid = fork();
    if (pid == -1)
    {
        perror("fork()");
        return 1;
    }
    if (pid == 0) // child
    {
        close(pipefds[0]); // close unused read end of pipe
        if (dup2(pipefds[1],STDOUT_FILENO) == -1)
        {
            perror("dup2");
            exit(1);
            return 1;
        }
        // Now all data coming out of stdout is going to the write end of the pipe we just created
        char **args = (char **)malloc((global_args.nargs+2)*sizeof(char *));
        memset(args,0,(global_args.nargs+2)*sizeof(char *));
        *args = global_args.executable;
        for (int i =0; i < global_args.nargs;i++)
            args[i+1] = global_args.args[i];
        if (execvp(global_args.executable,args)) // Should never return
        {
            perror("execvp");
            exit(1);
        }
        return 1; // If we get there theres a problem
    }
    // parent process
    bool timeoutactive = true;
    close(pipefds[1]); // close unused write end of pipe
    struct pollfd p;
    p.fd = pipefds[0];
    p.events = POLLIN;
    int prv = 0;
    time_t exptime = time(NULL);
    exptime += global_args.timeout;
    while ((prv = poll(&p,1,1000)) >= 0)
    {
        if (prv == 0) // just timed out
        {
            if (timeoutactive && time(NULL) >= exptime)
            {
                int rv;
                if (kill(pid,SIGKILL) != 0)
                    perror("kill");
                waitpid(pid,&rv,0);
                printf("until: Operation timed out\n");
                return 1;
            }
	    continue;
        }
        // Read up all the data coming in
        char *msg = NULL;
        char buf[4096];
        if (timeoutactive)
        {
            msg = (char *)malloc(sizeof(buf));
        }
        size_t msgSize = 0;
        ssize_t rsz = 0;
        do {
            memset(buf,0,sizeof(buf));
            if (msgSize)
            {
                msg = (char *)realloc(msg,msgSize + sizeof(buf));
            }
            rsz = read(pipefds[0],buf,sizeof(buf));
            if (rsz > 0) // put out to the terminal data as we get it
                write(STDOUT_FILENO,buf,rsz);
            if (timeoutactive)
            {
                memcpy(&msg[msgSize],buf,rsz);
                msgSize += rsz;
            }
        } while(rsz == sizeof(buf));
        if (rsz == 0)
	{
	    // Check if child process ended
	    siginfo_t s;
	    s.si_pid = 0;
	    int rv;
	    if (waitid(P_PID,pid,&s,WNOHANG|WEXITED|WNOWAIT) == -1)
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
        if (timeoutactive && strstr(msg,global_args.condData))
            timeoutactive = false;
	free(msg);
    }
    return 1; // Somethis is screwed up, lets just leave
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
