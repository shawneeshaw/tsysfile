#include <stdlib.h>
#include <string.h>

#include "trasysfileutil.h"
#include "trasysfile.h"


//////////////////////////////////////////////////////////////////////////
tsysLineStripper::tsysLineStripper() {
    this->initialization();
}

tsysLineStripper::~tsysLineStripper() {
}

////////////////////////////////////////
void tsysLineStripper::initialization() {
    this->featureCode       = NULL;
    this->featureCode_rev   = NULL;
    this->featureCodeLength = 0;

    memset(this->shift,     0, 256*sizeof(unsigned int));
    memset(this->shift_rev, 0, 256*sizeof(unsigned int));

    this->initBufferLen  = 0;
    this->fsize          = 0;

    this->pBuffer        = NULL;
    this->bufferLen      = 0;

    this->curLineNo      = 0;

    this->curLinePointer = NULL;
    this->nCharInCurLine = 0;

    this->fileOffset     = 0;

    this->lineBeginPos = this->lineEndPos = 0;
}

//////////////////////////////////////////////////////////////////////////
//                           PUBLIC FUNCTIONS                           //
//////////////////////////////////////////////////////////////////////////
tsys_stat tsysLineStripper::At(tsys_i32t lineNo)
{
    //@@preconditions
    assert(lineNo > 0);
    //@@end preconditions

    if(lineNo <= 0) return TSYSFILE_ERROR;

    tsys_i32t numLine = lineNo - this->curLineNo;
    if(numLine < 0){
        return (this->Backward(-numLine));
    }else if(numLine > 0){
        return (this->Forward(numLine));
    }

    return TSYSFILE_NOERROR;
}

tsys_stat tsysLineStripper::Backward(tsys_i32t numLine)
{
    //@@preconditions
    assert(numLine > 0);
    //@@end preconditions

    if(numLine <= 0) return TSYSFILE_ERROR;

    int stat = TSYSFILE_ERROR;
    for(tsys_i32t i = 0; i < numLine; i++) {
        this->lineEndPos = this->lineBeginPos;

        tsys_ui32t prevLineBeginpos;
        stat = this->gotoPrevLineBegin(prevLineBeginpos);
        if(stat == TSYSFILE_ERROR) return TSYSFILE_ERROR;
        assert(prevLineBeginpos >= 0 && prevLineBeginpos <= this->bufferLen);

        this->lineBeginPos = prevLineBeginpos;

        this->curLineNo--;
    }

    assert(this->pBuffer != NULL);
    if(!this->pBuffer) return TSYSFILE_ERROR;
    this->curLinePointer = this->pBuffer + this->lineBeginPos;
    
    if(stat == TSYSFILE_NOERROR_2){
        this->nCharInCurLine = this->lineEndPos - this->lineBeginPos;
    }else{
        this->nCharInCurLine = this->lineEndPos - this->lineBeginPos - this->featureCodeLength;
    }
    assert(this->nCharInCurLine >= 0);

    stat = TSYSFILE_NOERROR;
    
    return stat;
}

tsys_stat tsysLineStripper::Forward(tsys_i32t numLine)
{
    //@@preconditions
    assert(numLine > 0);
    //@@end preconditions

    if(numLine <= 0) return TSYSFILE_ERROR;

    int stat = TSYSFILE_ERROR;
    for(tsys_i32t i = 0; i < numLine; i++) {
        this->lineBeginPos = this->lineEndPos;

        tsys_ui32t nextLineBeginpos;
        stat = this->gotoNextLineBegin(nextLineBeginpos);
        if(stat == TSYSFILE_ERROR) return TSYSFILE_ERROR;
        assert(nextLineBeginpos >= 0 && nextLineBeginpos <= this->bufferLen);

        this->lineEndPos = nextLineBeginpos;

        this->curLineNo++;
    }

    assert(this->pBuffer != NULL);
    if(!this->pBuffer) return 0;
    this->curLinePointer = this->pBuffer + this->lineBeginPos;

    if(stat == TSYSFILE_END){
        this->nCharInCurLine = this->lineEndPos - this->lineBeginPos;
    }else{
        this->nCharInCurLine = this->lineEndPos - this->lineBeginPos - this->featureCodeLength;
    }
    assert(this->nCharInCurLine >= 0);

    stat = TSYSFILE_NOERROR;

    return stat;
}

tsys_stat tsysLineStripper::Jump(tsys_i32t numLine, tsysFile::Position origin)
{
    //@@preconditions
    assert(numLine > 0);
    //@@end preconditions

    if(origin == tsysFile::positionCurrent) {
        if(numLine == 0) return TSYSFILE_NOERROR;

        if(numLine > 0) {
            return (this->Forward(numLine));
        } else {
            return (this->Backward(-numLine));
        }
    }else if(origin == tsysFile::positionBegin) {
        if(numLine < 0) return TSYSFILE_ERROR;

        if(this->mappingNextBlock(0, this->initBufferLen)) {
            this->curLineNo = 0;
            this->lineBeginPos = this->lineEndPos = 0;
            return (this->Forward(numLine));
        }
    }else if(origin == tsysFile::positionEnd) {
        if(numLine < 0) return TSYSFILE_ERROR;
        
        while(this->Forward() != TSYSFILE_ERROR);  //goto the ending of file
        if(numLine == 0) return TSYSFILE_NOERROR;
        return this->Backward(numLine);
    }

    return TSYSFILE_ERROR;
}

//######################################################################//
//                           PRIVATE FUNCTIONS                          //
//######################################################################//
tsys_stat tsysLineStripper::gotoNextLineBegin(tsys_ui32t &nextLineBegpos)
{
    if( !this->pBuffer ) return TSYSFILE_ERROR;

    bool flg = false;
    tsys_ui32t newLen = 0;

    for(;;) {
        tsys_ui32t remainedBufferLen = this->bufferLen - this->lineBeginPos; assert(remainedBufferLen >= 0);
        unsigned char *pdest = (unsigned char *)quick_mem_find(this->featureCode, this->featureCodeLength,
                                                               this->pBuffer + this->lineBeginPos, remainedBufferLen,
                                                               1, this->shift);

        if(pdest) {
            nextLineBegpos = pdest - this->pBuffer + this->featureCodeLength;
            assert(nextLineBegpos >= 0);
    
            return TSYSFILE_NOERROR;
        }else {
            if(this->eof(this->fileOffset + this->bufferLen)) {
                if(this->eob(this->lineBeginPos)) return TSYSFILE_ERROR;
    
                nextLineBegpos = this->bufferLen;
                return TSYSFILE_END;
            }
    
            fpos_64t offExpect = this->fileOffset;
            if(newLen == 0){
                newLen    = this->lineEndPos - this->lineBeginPos + 1;
                offExpect = this->fileOffset + this->lineBeginPos;
            }
            fpos_64t offNew = offExpect;
    
            this->nextMappingOffsetandLen(offNew, newLen);
            assert(offExpect >= offNew);
    
            assert(offNew >= this->fileOffset);
            tsys_ui32t posOff = (tsys_ui32t)(offNew - this->fileOffset);
    
            newLen = this->checkBufferSize(newLen);
            if(newLen == TSYSFILE_MAX_VIEW_SIZE && flg == false)
                flg = true;
            else if(newLen == TSYSFILE_MAX_VIEW_SIZE && flg == true)
                return TSYSFILE_ERROR;
    
            if(!this->mappingNextBlock(offNew, newLen)) return TSYSFILE_ERROR;
    
            this->lineBeginPos -= posOff;
            this->lineEndPos   -= posOff;
        }
    }//end of for(;;)
}

tsys_stat tsysLineStripper::gotoPrevLineBegin(tsys_ui32t &prevLineBegpos)
{
    if( !this->pBuffer ) return TSYSFILE_ERROR;

    bool flg = false;
    bool notFeatureCodeEnding = 0;

    for(;;) {
        tsys_ui32t remainedBufferLen;
        if(this->eof(this->fileOffset + this->bufferLen) && this->eob(this->lineEndPos)) {
            if(quick_mem_rfind(this->featureCode_rev, this->featureCodeLength,
                               this->pBuffer+this->lineEndPos-1, this->featureCodeLength,
                               1, this->shift_rev)){
                remainedBufferLen = this->lineEndPos - this->featureCodeLength;
            }else{
                remainedBufferLen = this->lineEndPos;
                notFeatureCodeEnding = 1;
            }
        }else{
            if(this->lineEndPos < this->featureCodeLength)
                remainedBufferLen = 0;
            else
                remainedBufferLen = this->lineEndPos - this->featureCodeLength;
        }

        unsigned char *pdest = (unsigned char *)quick_mem_rfind(this->featureCode_rev, this->featureCodeLength,
                                                                this->pBuffer+remainedBufferLen-1, remainedBufferLen,
                                                                1, this->shift_rev);

        if(pdest) {
            prevLineBegpos = pdest - this->pBuffer + this->featureCodeLength;
            assert(prevLineBegpos >= 0);

            if(notFeatureCodeEnding) return TSYSFILE_NOERROR_2;

            return TSYSFILE_NOERROR;
        }else {
            if(this->bof(this->fileOffset)) {
                if(this->bob(this->lineEndPos)) return TSYSFILE_ERROR;

                prevLineBegpos = 0;
                return TSYSFILE_BEGIN;
            }
            
            tsys_ui32t mustKeepLen = lineEndPos + 1;
            tsys_ui32t newLen = mustKeepLen;
            fpos_64t offNew = this->fileOffset;
            this->prevMappingOffsetandLen(offNew, newLen);
            assert(this->fileOffset > offNew);

            newLen = this->checkBufferSize(newLen);
            if(newLen == TSYSFILE_MAX_VIEW_SIZE && flg == false) flg = true;
            else if(newLen == TSYSFILE_MAX_VIEW_SIZE && flg == true) return TSYSFILE_ERROR;
            
            if(!this->mappingPrevBlock(offNew, newLen)) return TSYSFILE_ERROR;

            this->lineEndPos += (newLen - mustKeepLen);
        }
    }//end of for(;;)
}
