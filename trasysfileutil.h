#ifndef TSYS_FILEUTIL_H
#define TSYS_FILEUTIL_H


void *quick_mem_find    (const void    *pattern,  long pl,
                         const void    *source,   long sl,
                         int shift_rebuild_flag, unsigned int *shift);

void *quick_mem_find_nc (const void    *pattern,  long pl,
                         const void    *source,   long sl,
                         int shift_rebuild_flag, unsigned int *shift);

void *quick_mem_rfind   (const void    *pattern,  long pl,
                         const void    *source,   long sl,
                         int shift_rebuild_flag, unsigned int *shift);

void *quick_mem_rfind_nc(const void    *pattern,  long pl,
                         const void    *source,   long sl,
                         int shift_rebuild_flag, unsigned int *shift);

#endif //TSYS_FILEUTIL_H
