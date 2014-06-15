#include "common.h"

#include <ctype.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "dynamic_filters.h"
#include "file.h"
#include "image.h"
#include "loaderpath.h"
#include "script.h"

static pImlibExternalFilter filters = NULL;
static int          dyn_initialised = 0;

#define MALLOCSHOW
#define FREESHOW
/*
#define FDEBUG
*/

pImlibExternalFilter
__imlib_LoadFilter(char *file)
{
   ImlibExternalFilter *ptr;
   struct imlib_filter_info *info;

   /* printf( "Loading filter %s\n", file ); */
   MALLOCSHOW;
   ptr = malloc(sizeof(ImlibExternalFilter));
   ptr->filename = strdup(file);
   ptr->handle = dlopen(file, RTLD_NOW | RTLD_LOCAL);
   if (!ptr->handle)
     {
        FREESHOW;
        free(ptr->filename);
        FREESHOW;
        free(ptr);
        return NULL;
     }
   ptr->init_filter = dlsym(ptr->handle, "init");
   ptr->deinit_filter = dlsym(ptr->handle, "deinit");
   ptr->exec_filter = dlsym(ptr->handle, "exec");
   if (!ptr->init_filter || !ptr->deinit_filter || !ptr->exec_filter)
     {
        dlclose(ptr->handle);
        FREESHOW;
        free(ptr->filename);
        FREESHOW;
        free(ptr);
        return NULL;
     }
   info = malloc(sizeof(struct imlib_filter_info));
   ptr->init_filter(info);
   ptr->num_filters = info->num_filters;
   ptr->filters = info->filters;
   ptr->name = info->name;
   ptr->author = info->author;
   ptr->description = info->description;

   free(info);

#ifdef FDEBUG
   printf("Filter has %d filters in it.\n", ptr->num_filters);
   for (i = 0; i < ptr->num_filters; i++)
      printf("  -> \"%s\"\n", ptr->filters[i]);
#endif

   ptr->next = NULL;
   return ptr;
}

void
__imlib_dynamic_filters_init()
{
   char              **list;
   int                 num_filters, i = 0;
   ImlibExternalFilter *ptr, *tptr;

   if (!dyn_initialised)
     {
        MALLOCSHOW;
        filters = malloc(sizeof(ImlibExternalFilter));
        filters->filename = (char *)"";
        filters->next = NULL;
        ptr = filters;
#ifdef FDEBUG
        printf("DEBUG: Dynamic filters Initisialising\n");
#endif
        dyn_initialised = 1;
#ifdef FDEBUG
        printf("DEBUG: Loading Filters\n");
#endif
        list = __imlib_ListFilters(&num_filters);
        for (i = num_filters - 1; i >= 0; i--)
          {
             tptr = NULL;
             if ((tptr = __imlib_LoadFilter(list[i])))
               {
                  ptr->next = tptr;
                  ptr = ptr->next;
               }
             if (list[i])
               {
                  FREESHOW;
                  free(list[i]);
               }
          }
        FREESHOW;
        if (list)
           free(list);
     }
}

void
__imlib_dynamic_filters_deinit()
{
}

pImlibExternalFilter
__imlib_get_dynamic_filter(char *name)
{
   pImlibExternalFilter f_ptr;
   int                 i = 0;

   /* scan the filters */
   for (f_ptr = filters->next; f_ptr; f_ptr = f_ptr->next)
     {
        /* scan the methods provided */
        for (i = 0; i < f_ptr->num_filters; i++)
          {
             if (strcmp(f_ptr->filters[i], name) == 0)
               {
#ifdef FDEBUG
                  printf("DEBUG: Found filter \"%s\"\n", name);
#endif
                  return f_ptr;
               }
          }
     }
   return NULL;
}

/* loader dir */
char              **
__imlib_ListFilters(int *num_ret)
{
   char              **list = NULL, **l, *s;
   int                 num, i, pi = 0;

   *num_ret = 0;
   /* same for system loader path */
   s = (char *)malloc(sizeof(SYS_LOADERS_PATH) + 8 + 1);
   sprintf(s, SYS_LOADERS_PATH "/filters");
   l = __imlib_FileDir(s, &num);
   if (num > 0)
     {
        *num_ret += num;
        list = realloc(list, sizeof(char *) * *num_ret);
        for (i = 0; i < num; i++)
          {
             s = (char *)realloc(s,
                                 sizeof(SYS_LOADERS_PATH) + 9 + strlen(l[i]) +
                                 1);
             sprintf(s, SYS_LOADERS_PATH "/filters/%s", l[i]);
             list[pi + i] = strdup(s);
          }
        __imlib_FileFreeDirList(l, num);
     }
   free(s);

   /* List currently contains *everything in there* we need to weed out
    * the .so, .la, .a versions of the same loader or whatever else.
    * dlopen can take an extension-less name and do the Right Thing
    * with it, so that's what we'll give it. */
   list = __imlib_TrimLoaderList(list, num_ret);

   return list;
}
