#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "trasysfileutil.h"


#define QS_ASIZE 256
static unsigned int shift[QS_ASIZE];

//---------------------------------------------------------------------//
inline void __qs_shift_construct( const unsigned char* pattern, long pl, int no_case_flag )
{
    long  byte_nbr;

    for (byte_nbr = 0; byte_nbr < QS_ASIZE; byte_nbr++)  shift [byte_nbr] = pl + 1;
    for (byte_nbr = 0; byte_nbr < pl; byte_nbr++) {
        ( !no_case_flag ) ? (shift [(unsigned char) pattern [byte_nbr]] = pl - byte_nbr) : (shift [::toupper((unsigned char) pattern [byte_nbr])] = pl - byte_nbr);
    }
}

//////////////////////////////////////////////////////////////////////////
void* __qs_mem_qfind (const void* in_pattern, long pl, const void* in_source, long sl,
                      int shift_rebuild_flag,
                      int backward_find_flag,
                      int no_case_flag,
                      unsigned int *pre_shift)
{
    unsigned int *shift_ptr = NULL;

    const unsigned char
        *match_base = NULL,
        *match_ptr  = NULL,
        *limit      = NULL;
    const unsigned char
        *block   = (unsigned char *) in_source,
        *pattern = (unsigned char *) in_pattern;

    long block_size   = sl;
    long pattern_size = pl;

    assert(block);

    if ( shift_rebuild_flag && !pre_shift ) {
        __qs_shift_construct ( pattern, pattern_size, no_case_flag );
    }

    if (block == NULL || pattern == NULL)
        return (NULL);

    if (block_size <= 0l)
        return (NULL);

    if (block_size < pattern_size) {
        return (NULL);
    }

    if (pattern_size == 0l)
        return ((void *)block);

    if( !pre_shift ) shift_ptr = shift;
    else shift_ptr = pre_shift;

    if(!backward_find_flag) {
        limit = block + (block_size - pattern_size + 1);
        assert (limit > block);
    }else{
        limit = block - (block_size - pattern_size + 1);
        assert (limit < block);
    }


    for ( match_base = block;
         (!backward_find_flag) ? (match_base < limit) : (match_base > limit);
         (!backward_find_flag) ? (match_base += ((no_case_flag) ? shift_ptr [::toupper(*(match_base + pattern_size))] : shift_ptr [*(match_base + pattern_size)])) :
                                 (match_base -= ((no_case_flag) ? shift_ptr [::toupper(*(match_base - pattern_size))] : shift_ptr [*(match_base - pattern_size)])) ) {
        match_ptr = match_base;
        long sz   = 0;

        while ( (!no_case_flag) ? (*match_ptr == pattern [sz]) : (::toupper(*match_ptr) == ::toupper(pattern [sz]))) {
            if(!backward_find_flag){
                assert (sz <= pattern_size &&  match_ptr == (match_base + sz));
                match_ptr++;
            }else{
                assert (sz <= pattern_size &&  match_ptr == (match_base - sz));
                match_ptr--;

                if(sz > 0) match_base--;
            }
            sz++;

            if (sz >= pattern_size) return ((void*)(match_base));
        }

        if(!backward_find_flag){
            if(((match_base + pattern_size) - block) >= block_size) return NULL;
        }else{
            if((block - (match_base - pattern_size)) >= block_size) return NULL;
        }
    } //end of for(match_base)

    return (NULL);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void *quick_mem_find  (const void    *pattern,  long pl,
                       const void    *source,   long sl,
                       int shift_rebuild_flag, unsigned int *shift)
{
    return __qs_mem_qfind(pattern, pl, source, sl, shift_rebuild_flag, 0, 0, shift);
}

void *quick_mem_find_nc(const void    *pattern,  long pl,
                        const void    *source,   long sl,
                        int shift_rebuild_flag, unsigned int *shift)
{
    return __qs_mem_qfind(pattern, pl, source, sl, shift_rebuild_flag, 0, 1, shift);
}

void *quick_mem_rfind  (const void    *pattern,  long pl,
                        const void    *source,   long sl,
                        int shift_rebuild_flag, unsigned int *shift)
{
    return __qs_mem_qfind(pattern, pl, source, sl, shift_rebuild_flag, 1, 0, shift);

}

void *quick_mem_rfind_nc(const void    *pattern,  long pl,
                         const void    *source,   long sl,
                         int shift_rebuild_flag, unsigned int *shift)
{
    return __qs_mem_qfind(pattern, pl, source, sl, shift_rebuild_flag, 1, 1, shift);
}
