#ifndef __LOADER_COMMON_H
#define __LOADER_COMMON_H 1

#include "config.h"
#include "common.h"
#include "image.h"

EAPI char           load(ImlibImage * im, ImlibProgressFunction progress,
                         char progress_granularity, char immediate_load);
EAPI char           load_scale(ImlibImage * im, ImlibProgressFunction progress,
                         char progress_granularity, char immediate_load,
                         int scale_width, int scale_height);
EAPI char           save(ImlibImage * im, ImlibProgressFunction progress,
                         char progress_granularity);
EAPI void           formats(ImlibLoader * l);

#endif /* __LOADER_COMMON_H */
