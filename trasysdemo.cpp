#include <stdlib.h>
#include <assert.h>

#include "trasysfile.h"

int main()
{
    tsysASCIILineReader tsReader("bottle.stl");
    assert(tsReader.getErrorNumber() == tsysFile::errSuccess);

    const char * lineString;
    tsys_ui32t   llen;
    
    //traverse file line by line from the beginning to the ending
    while( tsReader.forward() != TSYSFILE_ERROR ) {
        lineString = NULL; llen = 0;
        tsReader.getline(lineString, llen);

        if( llen > 0 ) {
            //do your actions...
        }
    } //end of while

    //traverse file line by line from the ending to the beginning
    while( tsReader.backward() != TSYSFILE_ERROR ) {
        lineString = NULL; llen = 0;
        tsReader.getline(lineString, llen);

        if( llen > 0 ) {
            //do your actions...
        }
    } //end of while

    return 0;
}
