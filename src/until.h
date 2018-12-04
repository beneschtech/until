#ifndef UNTIL_H
#define UNTIL_H

#include <sys/types.h>
#include <stdbool.h>

enum CancelCondition {
     Timeout,
     StringOut
};

typedef struct args {
    unsigned int timeout;
    int cond;
    void *condData;
    size_t condDataLen;
    char *executable;
    char **args;
    unsigned nargs;
} args;

#endif // UNTIL_H
