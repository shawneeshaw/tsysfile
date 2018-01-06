#include <stdlib.h>
#include <assert.h>

#include "trasysfile.h"

int main()
{
    char fname[] = "bottle.stl";

    tsysASCIILineReader tsReader(fname);
    assert(tsReader.getErrorNumber() == tsysFile::errSuccess);

    const char * lineString;
    tsys_ui32t   llen;
    while( tsReader.Forward() != TSYSFILE_ERROR ) {
        lineString = NULL; llen = 0;
        tsReader.getline(lineString, llen);

        if( llen > 0 ) {
        }
    } //end of while

    while( tsReader.Backward() != TSYSFILE_ERROR ) {
        lineString = NULL; llen = 0;
        tsReader.getline(lineString, llen);

        if( llen > 0 ) {
        }
    } //end of while

    return 0;
}
