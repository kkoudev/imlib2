/* Farbfeld (http://tools.suckless.org/farbfeld) */
#include <arpa/inet.h>
#include <stdint.h>

#include "loader_common.h"

#define LEN(x) (sizeof((x)) / sizeof(*(x)))

char
load(ImlibImage * im, ImlibProgressFunction progress,
     char progress_granularity, char immediate_load)
{
   FILE               *f;
   size_t              rowlen, i, j;
   uint32_t            hdr[2 + 1 + 1], w, h;
   uint16_t           *row;
   uint8_t            *dat;

   /* open the file for reading */
   if (!(f = fopen(im->real_file, "rb")))
     {
        return 0;
     }

   /* read and check the header */
   if (!im->data)
     {
        if (fread(hdr, sizeof(uint32_t), LEN(hdr), f) != LEN(hdr) ||
            memcmp("farbfeld", hdr, sizeof("farbfeld") - 1))
          {
             fclose(f);
             return 0;
          }
        im->w = ntohl(hdr[2]);
        im->h = ntohl(hdr[3]);
        if (!IMAGE_DIMENSIONS_OK(im->w, im->h))
          {
             fclose(f);
             return 0;
          }

        /* set format */
        if (!im->loader)
          {
             if (!(im->format = strdup("ff")))
               {
                  fclose(f);
                  return 0;
               }
          }
        SET_FLAG(im->flags, F_HAS_ALPHA);
     }

   /* load the data */
   if (im->loader || immediate_load || progress)
     {
        w = im->w;
        h = im->h;
        rowlen = w * (sizeof("RGBA") - 1);

        free(im->data);
        if (!(im->data = malloc(rowlen * h)) ||
            !(row = malloc(rowlen * sizeof(uint16_t))))
          {
             free(im->data);
             im->data = NULL;
             free(row);
             fclose(f);
             return 0;
          }

        dat = (uint8_t *) im->data;
        for (i = 0; i < h; i++, dat += rowlen)
          {
             if (fread(row, sizeof(uint16_t), rowlen, f) != rowlen)
               {
                  free(im->data);
                  im->data = NULL;
                  free(row);
                  fclose(f);
                  return 0;
               }
             for (j = 0; j < rowlen; j += 4)
               {
                  /*
                   * 16-Bit to 8-Bit (RGBA -> BGRA)
                   * 255 * 257 = 65535 = 2^16-1 = UINT16_MAX
                   */
                  dat[j + 2] = ntohs(row[j + 0]) / 257;
                  dat[j + 1] = ntohs(row[j + 1]) / 257;
                  dat[j + 0] = ntohs(row[j + 2]) / 257;
                  dat[j + 3] = ntohs(row[j + 3]) / 257;
               }
          }
        if (progress)
          {
             progress(im, 100, 0, 0, im->w, im->h);
          }

        free(row);
     }

   fclose(f);
   return 1;
}

char
save(ImlibImage * im, ImlibProgressFunction progress, char progress_gran)
{
   FILE               *f;
   size_t              rowlen, i, j;
   uint32_t            tmp32;
   uint16_t           *row;
   uint8_t            *dat;

   /* open the file for writing */
   if (!(f = fopen(im->real_file, "wb")))
     {
        return 0;
     }

   /* write header */
   fputs("farbfeld", f);
   tmp32 = htonl(im->w);
   if (fwrite(&tmp32, sizeof(uint32_t), 1, f) != 1)
     {
        fclose(f);
        return 0;
     }
   tmp32 = htonl(im->h);
   if (fwrite(&tmp32, sizeof(uint32_t), 1, f) != 1)
     {
        fclose(f);
        return 0;
     }

   /* write data */
   rowlen = im->w * (sizeof("RGBA") - 1);
   if (!(row = malloc(rowlen * sizeof(uint16_t))))
     {
        fclose(f);
        return 0;
     }
   dat = (uint8_t *) im->data;
   for (i = 0; i < (uint32_t) im->h; ++i, dat += rowlen)
     {
        for (j = 0; j < rowlen; j += 4)
          {
             /*
              * 8-Bit to 16-Bit
              * 255 * 257 = 65535 = 2^16-1 = UINT16_MAX
              */
             row[j + 0] = htons(dat[j + 2] * 257);
             row[j + 1] = htons(dat[j + 1] * 257);
             row[j + 2] = htons(dat[j + 0] * 257);
             row[j + 3] = htons(dat[j + 3] * 257);
          }
        if (fwrite(row, sizeof(uint16_t), rowlen, f) != rowlen)
          {
             free(row);
             fclose(f);
             return 0;
          }
     }
   if (progress)
     {
        progress(im, 100, 0, 0, im->w, im->h);
     }

   free(row);
   fclose(f);
   return 1;
}

void
formats(ImlibLoader * l)
{
   l->num_formats = 1;
   l->formats = malloc(sizeof(char *));
   *(l->formats) = strdup("ff");
}
