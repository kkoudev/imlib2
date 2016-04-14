#ifndef __FILE_H
#define __FILE_H 1

#include "common.h"

char               *__imlib_FileKey(const char *file);
char               *__imlib_FileRealFile(const char *file);
char               *__imlib_FileExtension(const char *file);
int                 __imlib_FileExists(const char *s);
int                 __imlib_FileIsFile(const char *s);
int                 __imlib_FileIsDir(const char *s);
char              **__imlib_FileDir(const char *dir, int *num);
void                __imlib_FileFreeDirList(char **l, int num);
void                __imlib_FileDel(const char *s);
time_t              __imlib_FileModDate(const char *s);
char               *__imlib_FileHomeDir(int uid);
int                 __imlib_FilePermissions(const char *s);
int                 __imlib_FileCanRead(const char *s);
int                 __imlib_IsRealFile(const char *s);

int                 __imlib_ItemInList(char **list, int size, char *item);

char              **__imlib_ListModules(const char *what, int *num_ret);

#endif
