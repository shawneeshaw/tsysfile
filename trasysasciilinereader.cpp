#include <stdlib.h>
#include <string.h>

#include "trasysfile.h"


static char winNewline[]     = "\r\n";
static char winNewline_Rev[] = "\n\r";  //reversed newline characters
static char unxNewline[]     = "\n";
static char macNewline[]     = "\r";


inline static void _shift_construct( const char* pattern, long pl, unsigned int *shift, long QS_ASIZE, int no_case_flag )
{
    assert(pattern);
    assert(pl > 0);
    assert(shift);
    assert(QS_ASIZE == 256);

    long byte_nbr;
    
    for (byte_nbr = 0; byte_nbr < QS_ASIZE; byte_nbr++)
        shift [byte_nbr] = pl + 1;
    for (byte_nbr = 0; byte_nbr < pl; byte_nbr++) {
        if( no_case_flag )
            shift [::toupper((unsigned char) pattern [byte_nbr])] = pl - byte_nbr;
        else
            shift [(unsigned char) pattern [byte_nbr]] = pl - byte_nbr;
    }
}


//////////////////////////////////////////////////////////////////////////
tsysASCIILineReader::tsysASCIILineReader(const char *fname, tsys_ui32t mappingLen) : tsysMappedFile(fname, tsysFile::accessReadOnly)
{
    this->fsize = this->length();

    size_64t mlen = mappingLen;
    if( mlen == 0 ) mlen = this->fsize;

    this->initBufferLen = this->checkBufferSize(mlen);

    if( this->fsize > 0 ) {
        assert(this->initBufferLen > 0);

        int ret = this->preRead();  //to determine the feature code(newline character) of the file
        assert(ret == 1);
    }
}

tsysASCIILineReader::~tsysASCIILineReader()
{
    final();
}

//######################################################################//
//                           PRIVATE FUNCTIONS                          //
//######################################################################//
int tsysASCIILineReader::preRead()
{
    //@@preconditions
    //@@end preconditions

    this->bufferLen = this->initBufferLen;
    while(this->mappingNextBlock(0, this->bufferLen)){
        if(this->pBuffer) {
            int ret = this->checkNewLineChars((const char *)this->pBuffer, this->bufferLen);
            if(ret == 1) return 1;

            if(this->bufferLen == this->fsize) {
                if( ret == -1 ) {
                    //it's '\r' newline character
                    this->featureCode       = macNewline;
                    this->featureCode_rev   = macNewline;
                    this->featureCodeLength = 1;
                    return 1;
                }else {
                    //no newline character(s) in whole file 
                    return 1;   
                }
            }
                
            if(this->bufferLen == TSYSFILE_MAX_VIEW_SIZE) {  //can not detect the newline character(s) until reaching the maximum mapping size
                this->pBuffer   = NULL;
                this->bufferLen = 0;
                return 0;
            }
                
            this->bufferLen = this->checkBufferSize(2 * this->bufferLen);   //extend mapping size
            assert(this->bufferLen > 0);
        }else{
            return 0;
        }
    }

    return 0;
}

int tsysASCIILineReader::checkNewLineChars(const char *buffer, tsys_ui32t len)
{
    //@@preconditions
    assert(buffer != NULL);
    assert(len > 0);
    //@@end preconditions

    tsys_ui32t pos = 0;
    while( pos < len ){
        if( buffer[pos] == '\n' ){
            this->featureCode       = unxNewline;
            this->featureCode_rev   = unxNewline;
            this->featureCodeLength = 1;

            _shift_construct(this->featureCode,     this->featureCodeLength, this->shift,     256, 0);
            _shift_construct(this->featureCode_rev, this->featureCodeLength, this->shift_rev, 256, 0);
            
            return 1;
        }else if(buffer[pos] == '\r') {
            if((pos + 1) >= len) return -1;  //extend reading buffer to see if the next character is '\n'?
            
            if(buffer[pos+1] == '\n'){
                this->featureCode       = winNewline;
                this->featureCode_rev   = winNewline_Rev;
                this->featureCodeLength = 2;
            }else{
                this->featureCode       = macNewline;
                this->featureCode_rev   = macNewline;
                this->featureCodeLength = 1;
            }
            
            _shift_construct(this->featureCode,     this->featureCodeLength, this->shift,     256, 0);
            _shift_construct(this->featureCode_rev, this->featureCodeLength, this->shift_rev, 256, 0);

            return 1;     
        }

        pos++;
    }
    
    return 0;
}
