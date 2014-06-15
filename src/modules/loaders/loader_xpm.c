#include "loader_common.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static FILE        *rgb_txt = NULL;

static void
xpm_parse_color(char *color, int *r, int *g, int *b)
{
   char                buf[4096];

   /* is a #ff00ff like color */
   if (color[0] == '#')
     {
        int                 len;
        char                val[32];

        len = strlen(color) - 1;
        if (len < 96)
          {
             int                 i;

             len /= 3;
             for (i = 0; i < len; i++)
                val[i] = color[1 + i + (0 * len)];
             val[i] = 0;
             sscanf(val, "%x", r);
             for (i = 0; i < len; i++)
                val[i] = color[1 + i + (1 * len)];
             val[i] = 0;
             sscanf(val, "%x", g);
             for (i = 0; i < len; i++)
                val[i] = color[1 + i + (2 * len)];
             val[i] = 0;
             sscanf(val, "%x", b);
             if (len == 1)
               {
                  *r = (*r << 4) | *r;
                  *g = (*g << 4) | *g;
                  *b = (*b << 4) | *b;
               }
             else if (len > 2)
               {
                  *r >>= (len - 2) * 4;
                  *g >>= (len - 2) * 4;
                  *b >>= (len - 2) * 4;
               }
          }
        return;
     }
   /* look in rgb txt database */
   if (!rgb_txt)
      rgb_txt = fopen("/usr/share/X11/rgb.txt", "r");
   if (!rgb_txt)
      rgb_txt = fopen("/usr/X11R6/lib/X11/rgb.txt", "r");
   if (!rgb_txt)
      rgb_txt = fopen("/usr/openwin/lib/X11/rgb.txt", "r");
   if (!rgb_txt)
      return;
   fseek(rgb_txt, 0, SEEK_SET);
   while (fgets(buf, 4000, rgb_txt))
     {
        if (buf[0] != '!')
          {
             int                 rr, gg, bb;
             char                name[4096];

             sscanf(buf, "%i %i %i %[^\n]", &rr, &gg, &bb, name);
             if (!strcasecmp(name, color))
               {
                  *r = rr;
                  *g = gg;
                  *b = bb;
                  return;
               }
          }
     }
}

static void
xpm_parse_done(void)
{
   if (rgb_txt)
      fclose(rgb_txt);
   rgb_txt = NULL;
}

char
load(ImlibImage * im, ImlibProgressFunction progress, char progress_granularity,
     char immediate_load)
{
   DATA32             *ptr, *end;
   FILE               *f;

   int                 pc, c, i, j, k, w, h, ncolors, cpp, comment, transp,
      quote, context, len, done, r, g, b, backslash;
   char               *line, s[256], tok[256], col[256];
   int                 lsz = 256;
   struct _cmap {
      char                str[6];
      unsigned char       transp;
      short               r, g, b;
   }                  *cmap;

   short               lookup[128 - 32][128 - 32];
   float               per = 0.0, per_inc = 0.0;
   int                 last_per = 0, last_y = 0;
   int                 count, pixels;

   done = 0;
   transp = -1;

   /* if immediate_load is 1, then dont delay image laoding as below, or */
   /* already data in this image - dont load it again */

   if (im->data)
     {
        xpm_parse_done();
        return 0;
     }
   f = fopen(im->real_file, "rb");
   if (!f)
     {
        xpm_parse_done();
        return 0;
     }
   if (fread(s, 1, 9, f) != 9)
     {
        fclose(f);
        xpm_parse_done();
        return 0;
     }
   rewind(f);
   s[9] = 0;
   if (strcmp("/* XPM */", s))
     {
        fclose(f);
        xpm_parse_done();
        return 0;
     }

   i = 0;
   j = 0;
   cmap = NULL;
   w = 10;
   h = 10;
   ptr = NULL;
   end = NULL;
   c = ' ';
   comment = 0;
   quote = 0;
   context = 0;
   pixels = 0;
   count = 0;
   line = malloc(lsz);
   if (!line)
      return 0;

   backslash = 0;
   memset(lookup, 0, sizeof(lookup));
   while (!done)
     {
        pc = c;
        c = fgetc(f);
        if (c == EOF)
           break;

        if (!quote)
          {
             if ((pc == '/') && (c == '*'))
                comment = 1;
             else if ((pc == '*') && (c == '/') && (comment))
                comment = 0;
          }

        if (comment)
           continue;

        if ((!quote) && (c == '"'))
          {
             quote = 1;
             i = 0;
          }
        else if ((quote) && (c == '"'))
          {
             line[i] = 0;
             quote = 0;
             if (context == 0)
               {
                  /* Header */
                  sscanf(line, "%i %i %i %i", &w, &h, &ncolors, &cpp);
                  if ((ncolors > 32766) || (ncolors < 1))
                    {
                       fprintf(stderr,
                               "IMLIB ERROR: XPM files with colors > 32766 or < 1 not supported\n");
                       free(line);
                       fclose(f);
                       xpm_parse_done();
                       return 0;
                    }
                  if ((cpp > 5) || (cpp < 1))
                    {
                       fprintf(stderr,
                               "IMLIB ERROR: XPM files with characters per pixel > 5 or < 1not supported\n");
                       free(line);
                       fclose(f);
                       xpm_parse_done();
                       return 0;
                    }
                  if (!IMAGE_DIMENSIONS_OK(w, h))
                    {
                       fprintf(stderr,
                               "IMLIB ERROR: Invalid image dimension: %dx%d\n",
                               w, h);
                       free(line);
                       fclose(f);
                       xpm_parse_done();
                       return 0;
                    }
                  im->w = w;
                  im->h = h;
                  if (!im->format)
                     im->format = strdup("xpm");

                  cmap = malloc(sizeof(struct _cmap) * ncolors);

                  if (!cmap)
                    {
                       free(line);
                       fclose(f);
                       xpm_parse_done();
                       return 0;
                    }

                  per_inc = 100.0 / (((float)w) * h);

                  if (im->loader || immediate_load || progress)
                    {
                       im->data =
                          (DATA32 *) malloc(sizeof(DATA32) * im->w * im->h);
                       if (!im->data)
                         {
                            free(cmap);
                            free(line);
                            fclose(f);
                            xpm_parse_done();
                            return 0;
                         }
                       ptr = im->data;
                       pixels = w * h;
                       end = ptr + (pixels);
                    }
                  else
                    {
                       free(cmap);
                       free(line);
                       fclose(f);
                       xpm_parse_done();
                       return 1;
                    }

                  j = 0;
                  context++;
               }
             else if (context == 1)
               {
                  /* Color Table */
                  if (j < ncolors)
                    {
                       int                 slen;
                       int                 hascolor, iscolor;

                       iscolor = 0;
                       hascolor = 0;
                       tok[0] = 0;
                       col[0] = 0;
                       s[0] = 0;
                       len = strlen(line);
                       strncpy(cmap[j].str, line, cpp);
                       cmap[j].str[cpp] = 0;
                       cmap[j].r = -1;
                       cmap[j].transp = 0;
                       for (k = cpp; k < len; k++)
                         {
                            if (line[k] != ' ')
                              {
                                 s[0] = 0;
                                 sscanf(&line[k], "%255s", s);
                                 slen = strlen(s);
                                 k += slen;
                                 if (!strcmp(s, "c"))
                                    iscolor = 1;
                                 if ((!strcmp(s, "m")) || (!strcmp(s, "s"))
                                     || (!strcmp(s, "g4"))
                                     || (!strcmp(s, "g"))
                                     || (!strcmp(s, "c")) || (k >= len))
                                   {
                                      if (k >= len)
                                        {
                                           if (col[0])
                                             {
                                                if (strlen(col) <
                                                    (sizeof(col) - 2))
                                                   strcat(col, " ");
                                                else
                                                   done = 1;
                                             }
                                           if (strlen(col) + strlen(s) <
                                               (sizeof(col) - 1))
                                              strcat(col, s);
                                        }
                                      if (col[0])
                                        {
                                           if (!strcasecmp(col, "none"))
                                             {
                                                transp = 1;
                                                cmap[j].transp = 1;
                                             }
                                           else
                                             {
                                                if ((((cmap[j].r < 0) ||
                                                      (!strcmp(tok, "c")))
                                                     && (!hascolor)))
                                                  {
                                                     r = 0;
                                                     g = 0;
                                                     b = 0;
                                                     xpm_parse_color(col, &r,
                                                                     &g, &b);
                                                     cmap[j].r = r;
                                                     cmap[j].g = g;
                                                     cmap[j].b = b;
                                                     if (iscolor)
                                                        hascolor = 1;
                                                  }
                                             }
                                        }
                                      strcpy(tok, s);
                                      col[0] = 0;
                                   }
                                 else
                                   {
                                      if (col[0])
                                        {
                                           if (strlen(col) < (sizeof(col) - 2))
                                              strcat(col, " ");
                                           else
                                              done = 1;
                                        }
                                      if (strlen(col) + strlen(s) <
                                          (sizeof(col) - 1))
                                         strcat(col, s);
                                   }
                              }
                         }
                    }
                  j++;
                  if (j >= ncolors)
                    {
                       if (cpp == 1)
                          for (i = 0; i < ncolors; i++)
                             lookup[(int)cmap[i].str[0] - 32][0] = i;
                       if (cpp == 2)
                          for (i = 0; i < ncolors; i++)
                             lookup[(int)cmap[i].str[0] -
                                    32][(int)cmap[i].str[1] - 32] = i;
                       context++;
                    }

                  if (transp >= 0)
                    {
                       SET_FLAG(im->flags, F_HAS_ALPHA);
                    }
                  else
                    {
                       UNSET_FLAG(im->flags, F_HAS_ALPHA);
                    }
               }
             else
               {
                  /* Image Data */
                  i = 0;
                  if (cpp == 0)
                    {
                       /* Chars per pixel = 0? well u never know */
                    }
                  if (cpp == 1)
                    {
#define PIX_RGB(_r, _g, _b) (((_r) << 16) | ((_g) << 8) | (_b))
#define PIX_ARGB(_r, _g, _b) ((0xff << 24) | PIX_RGB(_r, _g, _b))

#define CM1_TRANS() cmap[lookup[col[0] - ' '][0]].transp
#define CM1_R()     (unsigned char)cmap[lookup[col[0] - ' '][0]].r
#define CM1_G()     (unsigned char)cmap[lookup[col[0] - ' '][0]].g
#define CM1_B()     (unsigned char)cmap[lookup[col[0] - ' '][0]].b
                       for (i = 0;
                            ((i < 65536) && (ptr < end) && (line[i])); i++)
                         {
                            col[0] = line[i];
                            r = CM1_R();
                            g = CM1_G();
                            b = CM1_B();
                            if (transp && CM1_TRANS())
                               *ptr++ = PIX_RGB(r, g, b);
                            else
                               *ptr++ = PIX_ARGB(r, g, b);
                            count++;
                         }
                    }
                  else if (cpp == 2)
                    {
#define CM2_TRANS() cmap[lookup[col[0] - ' '][col[1] - ' ']].transp
#define CM2_R()     (unsigned char)cmap[lookup[col[0] - ' '][col[1] - ' ']].r
#define CM2_G()     (unsigned char)cmap[lookup[col[0] - ' '][col[1] - ' ']].g
#define CM2_B()     (unsigned char)cmap[lookup[col[0] - ' '][col[1] - ' ']].b
                       for (i = 0;
                            ((i < 65536) && (ptr < end) && (line[i])); i++)
                         {
                            col[0] = line[i++];
                            col[1] = line[i];
                            r = CM2_R();
                            g = CM2_G();
                            b = CM2_B();
                            if (transp && CM2_TRANS())
                               *ptr++ = PIX_RGB(r, g, b);
                            else
                               *ptr++ = PIX_ARGB(r, g, b);
                            count++;
                         }
                    }
                  else
                    {
#define CM0_TRANS(_j) cmap[_j].transp
#define CM0_R(_j)     (unsigned char)cmap[_j].r
#define CM0_G(_j)     (unsigned char)cmap[_j].g
#define CM0_B(_j)     (unsigned char)cmap[_j].b
                       for (i = 0;
                            ((i < 65536) && (ptr < end) && (line[i])); i++)
                         {
                            for (j = 0; j < cpp; j++, i++)
                              {
                                 col[j] = line[i];
                              }
                            col[j] = 0;
                            i--;
                            for (j = 0; j < ncolors; j++)
                              {
                                 if (!strcmp(col, cmap[j].str))
                                   {
                                      r = CM0_R(j);
                                      g = CM0_G(j);
                                      b = CM0_B(j);
                                      if (transp && CM0_TRANS(j))
                                         *ptr++ = PIX_RGB(r, g, b);
                                      else
                                         *ptr++ = PIX_ARGB(r, g, b);
                                      count++;
                                      j = ncolors;
                                   }
                              }
                         }
                    }
                  per += per_inc;
                  if (progress && (((int)per) != last_per)
                      && (((int)per) % progress_granularity == 0))
                    {
                       last_per = (int)per;
                       if (!(progress(im, (int)per, 0, last_y, w, i)))
                         {
                            fclose(f);
                            free(cmap);
                            free(line);
                            xpm_parse_done();
                            return 2;
                         }
                       last_y = i;
                    }
               }
          }

        /* Scan in line from XPM file */
        if ((quote) && (c != '"'))
          {
             if (c < 32)
                c = 32;
             else if (c > 127)
                c = 127;
             if (c == '\\')
               {
                  if (++backslash < 2)
                    {
                       line[i++] = c;
                    }
                  else
                    {
                       backslash = 0;
                    }
               }
             else
               {
                  backslash = 0;
                  line[i++] = c;
               }
          }

        if (i >= lsz)
          {
             lsz += 256;
             line = realloc(line, lsz);
          }

        if (((ptr) && ((ptr - im->data) >= (w * h * (int)sizeof(DATA32)))) ||
            ((context > 1) && (count >= pixels)))
           done = 1;
     }

   if (progress)
     {
        progress(im, 100, 0, last_y, w, h);
     }

   fclose(f);
   free(cmap);
   free(line);

   xpm_parse_done();

   return 1;
}

void
formats(ImlibLoader * l)
{
   static const char  *const list_formats[] = { "xpm" };
   int                 i;

   l->num_formats = sizeof(list_formats) / sizeof(char *);
   l->formats = malloc(sizeof(char *) * l->num_formats);

   for (i = 0; i < l->num_formats; i++)
      l->formats[i] = strdup(list_formats[i]);
}
