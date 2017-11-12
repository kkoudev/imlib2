#include "config.h"

#include <stdlib.h>

#include "file.h"
#include "image.h"
#include "loaderpath.h"

static char       **
__imlib_TrimLoaderList(char **list, int *num)
{
   int                 i, n, size = 0;

   if (!list)
      return NULL;

   n = *num;

   for (i = 0; i < n; i++)
     {
        char               *ext;

        if (!list[i])
           continue;
        ext = strrchr(list[i], '.');
        if ((ext) && (
#ifdef __CYGWIN__
                        (!strcasecmp(ext, ".dll")) ||
#endif
                        (!strcasecmp(ext, ".so"))))
          {
             /* Don't add the same loader multiple times... */
             if (!__imlib_ItemInList(list, size, list[i]))
               {
                  list[size++] = list[i];
                  continue;
               }
          }
        free(list[i]);
     }
   if (!size)
     {
        free(list);
        list = NULL;
     }
   else
     {
        list = realloc(list, size * sizeof(char *));
     }
   *num = size;
   return list;
}

char              **
__imlib_ListModules(const char *what, int *num_ret)
{
   char              **list = NULL, **l;
   char                path[1024];
   int                 num, i;

   *num_ret = 0;

   snprintf(path, sizeof(path), "%s/%s", SYS_LOADERS_PATH, what);
   l = __imlib_FileDir(path, &num);
   if (num <= 0)
      return NULL;

   list = malloc(num * sizeof(char *));
   if (list)
     {
        for (i = 0; i < num; i++)
          {
             snprintf(path, sizeof(path), "%s/%s/%s",
                      SYS_LOADERS_PATH, what, l[i]);
             list[i] = strdup(path);
          }
        *num_ret = num;
     }
   __imlib_FileFreeDirList(l, num);

   /* List currently contains *everything in there* we need to weed out
    * the .so, .la, .a versions of the same loader or whatever else.
    * dlopen can take an extension-less name and do the Right Thing
    * with it, so that's what we'll give it. */
   list = __imlib_TrimLoaderList(list, num_ret);

   return list;
}
