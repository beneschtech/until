AC_INIT([until],[1.0],[bug-until@beneschtech.com])
AM_INIT_AUTOMAKE([-Wall])
AC_PROG_CC
AC_CONFIG_HEADERS([config.h])

AC_TYPE_SIZE_T 
AC_TYPE_PID_T
AC_TYPE_SSIZE_T

AC_CHECK_HEADER_STDBOOL
AC_CHECK_HEADERS([stdlib.h])
AC_CHECK_HEADERS([string.h])
AC_CHECK_HEADERS([unistd.h])

AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([strdup])
AC_CHECK_FUNCS([dup2])
AC_CHECK_FUNCS([strstr])
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC

AC_CONFIG_FILES([
Makefile
src/Makefile
doc/Makefile
])
AC_OUTPUT
