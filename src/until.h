/*
 *  until - Run other commands with a timeout
    Copyright (C) 2019 Chris Benesch <chris@beneschtech.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
  */
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
