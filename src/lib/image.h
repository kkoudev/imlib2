#ifndef __IMAGE
#define __IMAGE 1

#include "common.h"
#ifdef BUILD_X11
#include <X11/Xlib.h>
#else
#ifndef X_DISPLAY_MISSING
#define X_DISPLAY_MISSING
#endif
#endif

typedef struct _imlibimage ImlibImage;

#ifdef BUILD_X11
typedef struct _imlibimagepixmap ImlibImagePixmap;
#endif
typedef struct _imlibborder ImlibBorder;
typedef struct _imlibloader ImlibLoader;
typedef struct _imlibimagetag ImlibImageTag;
typedef enum _imlib_load_error ImlibLoadError;

typedef int         (*ImlibProgressFunction) (ImlibImage * im, char percent,
                                              int update_x, int update_y,
                                              int update_w, int update_h);
typedef void        (*ImlibDataDestructorFunction) (ImlibImage * im,
                                                    void *data);

enum _iflags {
   F_NONE = 0,
   F_HAS_ALPHA = (1 << 0),
   F_UNLOADED = (1 << 1),
   F_UNCACHEABLE = (1 << 2),
   F_ALWAYS_CHECK_DISK = (1 << 3),
   F_INVALID = (1 << 4),
   F_DONT_FREE_DATA = (1 << 5),
   F_FORMAT_IRRELEVANT = (1 << 6),
   F_BORDER_IRRELEVANT = (1 << 7),
   F_ALPHA_IRRELEVANT = (1 << 8)
};

typedef enum _iflags ImlibImageFlags;

struct _imlibborder {
   int                 left, right, top, bottom;
};

struct _imlibimagetag {
   char               *key;
   int                 val;
   void               *data;
   void                (*destructor) (ImlibImage * im, void *data);
   ImlibImageTag      *next;
};

struct _imlibimage {
   char               *file;
   int                 w, h;
   DATA32             *data;
   ImlibImageFlags     flags;
   time_t              moddate;
   ImlibBorder         border;
   int                 references;
   ImlibLoader        *loader;
   char               *format;
   ImlibImage         *next;
   ImlibImageTag      *tags;
   char               *real_file;
   char               *key;
};

#ifdef BUILD_X11
struct _imlibimagepixmap {
   int                 w, h;
   Pixmap              pixmap, mask;
   Display            *display;
   Visual             *visual;
   int                 depth;
   int                 source_x, source_y, source_w, source_h;
   Colormap            colormap;
   char                antialias, hi_quality, dither_mask;
   ImlibBorder         border;
   ImlibImage         *image;
   char               *file;
   char                dirty;
   int                 references;
   DATABIG             modification_count;
   ImlibImagePixmap   *next;
};
#endif

struct _imlibloader {
   char               *file;
   int                 num_formats;
   char              **formats;
   void               *handle;
   char                (*load) (ImlibImage * im,
                                ImlibProgressFunction progress,
                                char progress_granularity, char immediate_load);
   char                (*save) (ImlibImage * im,
                                ImlibProgressFunction progress,
                                char progress_granularity);
   ImlibLoader        *next;
};

void                __imlib_AttachTag(ImlibImage * im, const char *key,
                                      int val, void *data,
                                      ImlibDataDestructorFunction destructor);
ImlibImageTag      *__imlib_GetTag(ImlibImage * im, const char *key);
ImlibImageTag      *__imlib_RemoveTag(ImlibImage * im, const char *key);
void                __imlib_FreeTag(ImlibImage * im, ImlibImageTag * t);
void                __imlib_FreeAllTags(ImlibImage * im);

void                __imlib_SetCacheSize(int size);
int                 __imlib_GetCacheSize(void);
ImlibImage         *__imlib_ProduceImage(void);
void                __imlib_ConsumeImage(ImlibImage * im);
ImlibImage         *__imlib_FindCachedImage(const char *file);
void                __imlib_AddImageToCache(ImlibImage * im);
void                __imlib_RemoveImageFromCache(ImlibImage * im);
int                 __imlib_CurrentCacheSize(void);
void                __imlib_CleanupImageCache(void);

#ifdef BUILD_X11
ImlibImagePixmap   *__imlib_ProduceImagePixmap(void);
void                __imlib_ConsumeImagePixmap(ImlibImagePixmap * ip);
ImlibImagePixmap   *__imlib_FindCachedImagePixmap(ImlibImage * im, int w, int h,
                                                  Display * d, Visual * v,
                                                  int depth, int sx, int sy,
                                                  int sw, int sh, Colormap cm,
                                                  char aa, char hiq, char dmask,
                                                  DATABIG modification_count);
ImlibImagePixmap   *__imlib_FindCachedImagePixmapByID(Display * d, Pixmap p);
void                __imlib_AddImagePixmapToCache(ImlibImagePixmap * ip);
void                __imlib_RemoveImagePixmapFromCache(ImlibImagePixmap * ip);
void                __imlib_CleanupImagePixmapCache(void);
#endif
void                __imlib_RemoveAllLoaders(void);
ImlibLoader        *__imlib_FindBestLoaderForFile(const char *file,
                                                  int for_save);
ImlibLoader        *__imlib_FindBestLoaderForFileFormat(const char *file,
                                                        char *format,
                                                        int for_save);
void                __imlib_SetImageAlphaFlag(ImlibImage * im, char alpha);
ImlibImage         *__imlib_CreateImage(int w, int h, DATA32 * data);
ImlibImage         *__imlib_LoadImage(const char *file,
                                      ImlibProgressFunction progress,
                                      char progress_granularity,
                                      char immediate_load, char dont_cache,
                                      ImlibLoadError * er);
#ifdef BUILD_X11
ImlibImagePixmap   *__imlib_FindImlibImagePixmapByID(Display * d, Pixmap p);
#endif
void                __imlib_FreeImage(ImlibImage * im);

#ifdef BUILD_X11
void                __imlib_FreePixmap(Display * d, Pixmap p);
#endif
void                __imlib_FlushCache(void);

#ifdef BUILD_X11
void                __imlib_DirtyPixmapsForImage(ImlibImage * im);
#else
#define	__imlib_DirtyPixmapsForImage(x) /* x */
#endif
void                __imlib_DirtyImage(ImlibImage * im);
void                __imlib_SaveImage(ImlibImage * im, const char *file,
                                      ImlibProgressFunction progress,
                                      char progress_granularity,
                                      ImlibLoadError * er);

#define IMAGE_HAS_ALPHA(im) ((im)->flags & F_HAS_ALPHA)
#define IMAGE_IS_UNLOADED(im) ((im)->flags & F_UNLOADED)
#define IMAGE_IS_UNCACHEABLE(im) ((im)->flags & F_UNCACHEABLE)
#define IMAGE_ALWAYS_CHECK_DISK(im) ((im)->flags & F_ALWAYS_CHECK_DISK)
#define IMAGE_IS_VALID(im) (!((im)->flags & F_INVALID))
#define IMAGE_FREE_DATA(im) (!((im)->flags & F_DONT_FREE_DATA))

#define SET_FLAG(flags, f) ((flags) |= (f))
#define UNSET_FLAG(flags, f) ((flags) &= (~f))

/* The maximum pixmap dimension is 65535. */
/* However, for now, use 46340 (46340^2 < 2^31) to avoid buffer overflow issues. */
/* Reduced further to 32767, so that (w * h * sizeof(DATA32)) won't exceed ULONG_MAX */
#define X_MAX_DIM 32767

#define IMAGE_DIMENSIONS_OK(w, h) \
   ( ((w) > 0) && ((h) > 0) && ((w) < X_MAX_DIM) && ((h) < X_MAX_DIM) )

#endif
