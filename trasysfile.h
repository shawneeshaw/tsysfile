#ifndef TSYS_FILE_H
#define TSYS_FILE_H

#ifdef WIN32  //Windows
  #include <io.h>
  #include <sys/types.h>
  #ifndef GCC
    #include <direct.h>
  #else
    #include <sys/stat.h>
  #endif
  #include <windows.h>
  #include <winnt.h>
#else  //Linux
  #include <unistd.h>
  #include <sys/stat.h>
  #include <sys/mman.h>
  #include <sys/types.h>
  #include <dirent.h>
  #include <fcntl.h>
#endif
#include <assert.h>

#ifdef TSYS_DYNAMIC_LIB
  #ifdef WIN32
    #undef  TSYS_EXPORT
    #undef  TSYS_EXPORT_EXTERN
    #undef  TSYS_IMPORT

    #define TSYS_EXPORT                __declspec(dllexport)
    #define TSYS_EXPORT_EXTERN  extern __declspec(dllexport)
    #define TSYS_IMPORT                __declspec(dllimport)
  #endif
#else
    #define TSYS_EXPORT
    #define TSYS_EXPORT_EXTERN
    #define TSYS_IMPORT
#endif


//----------------------------------------------------------------------//
typedef long                    tsys_i32t;
typedef unsigned long           tsys_ui32t;
#ifndef WIN32
  typedef long long             tsys_i64t;
  typedef unsigned long long    tsys_ui64t;
#else
  #ifdef GCC
    typedef long long           tsys_i64t;
    typedef unsigned long long  tsys_ui64t;
  #else
    typedef __int64             tsys_i64t;
    typedef unsigned __int64    tsys_ui64t;
  #endif
#endif
typedef int   tsys_stat;
typedef bool  tsys_bool;

typedef tsys_ui64t  fpos_64t;  // "64-bit system file position offset"
typedef tsys_ui64t fsize_64t;  // "64-bit system file size"
typedef tsys_ui64t  size_64t;  // "64-bit system size"

#ifndef WIN32
// use a define so that if the sys/types.h header already defines caddr_t
// as it may on BSD systems, we do not break it by redefining again.
#undef  caddr_t
typedef char*        caddr_t;
typedef tsys_ui32t   ccxx_size_t;
typedef tsys_ui64t   ccxx_size_64t;
#else
  #ifndef GCC
    typedef void*    caddr_t;
  #endif
  typedef DWORD      ccxx_size_t;
  typedef tsys_ui64t ccxx_size_64t;
#endif


//----------------------------------------------------------------------//
#ifdef WIN32
  #define TSYSFILE_MAX_VIEW_SIZE 0x40000000 //maximum to 1GB under Windows system
#else
  #define TSYSFILE_MAX_VIEW_SIZE 0x80000000 //maximum to 2GB under Linux system
#endif
#define TSYSFILE_MIN_VIEW_SIZE   0x00000001 //minimum to 1byte
#define TSYSFILE_DEF_VIEW_SIZE      0x80000 //default to 512K

#define TSYSFILE_BEGIN       -1
#define TSYSFILE_END         -1

#define TSYSFILE_ERROR        0
#define TSYSFILE_NOERROR      1
#define TSYSFILE_NOERROR_2    2


//////////////////////////////////////////////////////////////////////////
/**
 * The purpose of this class is to provide common enum variables definition,
 * and offer some utility file operation functions at the same time.
 *
*/
class TSYS_EXPORT tsysFile {
public:
  enum Error {
    errSuccess = 0,
    errNotOpened,
    errMapFailed,
    errInitFailed,
    errOpenDenied,
    errOpenFailed,
    errOpenInUse,
    errReadInterrupted,
    errReadIncomplete,
    errReadFailure,
    errWriteInterrupted,
    errWriteIncomplete,
    errWriteFailure,
    errLockFailure,
    errExtended,
    errZeroLength
  };
  typedef enum Error Error;

  enum Access {
#ifndef WIN32
    accessReadOnly  = O_RDONLY,
    accessWriteOnly = O_WRONLY,
    accessReadWrite = O_RDWR
#else
    accessReadOnly  = GENERIC_READ,
    accessWriteOnly = GENERIC_WRITE,
    accessReadWrite = GENERIC_READ | GENERIC_WRITE
#endif
  };
  typedef enum Access Access;

protected:
  typedef struct _fcb {
    struct _fcb * next;
    caddr_t       address;
    ccxx_size_t   len;
    fpos_64t      pos;      /* off_t pos */
    bool          locked;
  } fcb_t;

public:
#ifdef  WIN32
  enum Open {
    openReadOnly,  // = FILE_OPEN_READONLY,
    openWriteOnly, // = FILE_OPEN_WRITEONLY,
    openReadWrite, // = FILE_OPEN_READWRITE,
    openAppend,    // = FILE_OPEN_APPEND,
    openTruncate   // = FILE_OPEN_TRUNCATE
  };
#else
  enum Open {
    openReadOnly  = O_RDONLY,
    openWriteOnly = O_WRONLY,
    openReadWrite = O_RDWR,
    openAppend    = O_WRONLY | O_APPEND,
#ifdef  O_SYNC
    openSync = O_RDWR | O_SYNC,
#else
    openSync = O_RDWR,
#endif
    openTruncate = O_RDWR | O_TRUNC
  };
  typedef enum Open Open;

/* to be used in future */

#ifndef S_IRUSR
#define S_IRUSR 0400
#define S_IWUSR 0200
#define S_IRGRP 0040
#define S_IWGRP 0020
#define S_IROTH 0004
#define S_IWOTH 0002
#endif

#endif // !WIN32

#ifndef WIN32
  enum Attr {
    attrInvalid = 0,
    attrPrivate = S_IRUSR | S_IWUSR,
    attrGroup   = attrPrivate | S_IRGRP | S_IWGRP,
    attrPublic  = attrGroup   | S_IROTH | S_IWOTH
  };
#else // defined WIN32
  enum Attr {
    attrInvalid = 0,
    attrPrivate,
    attrGroup,
    attrPublic
  };
#endif // !WIN32
  typedef enum Attr Attr;

#ifdef  WIN32
  enum Complete {
    completionImmediate, // = FILE_COMPLETION_IMMEDIATE,
    completionDelayed,   // = FILE_COMPLETION_DELAYED,
    completionDeferred   // = FILE_COMPLETION_DEFERRED
  };

  enum Mapping {
    mappedRead,
    mappedWrite,
    mappedReadWrite
  };
#else
  enum Mapping {
    mappedRead      = accessReadOnly,
    mappedWrite     = accessWriteOnly,
    mappedReadWrite = accessReadWrite
  };

  enum Complete {
    completionImmediate,
    completionDelayed,
    completionDeferred
  };
#endif
  typedef enum Complete Complete;
  typedef enum Mapping Mapping;

public:
  enum Position {
    positionBegin = 0,
    positionEnd,
    positionCurrent
  };
  typedef enum Position Position;
};


//////////////////////////////////////////////////////////////////////////
/**
 * The purpose of this class is to define a base class for low level
 * random file access that is portable between Win32 and POSIX systems.
 * This class is a foundation both for optimized thread shared and
 * traditional locked file access that is commonly used to build
 * database services, rather than the standard C++ streaming file classes.
 *
 * @short Portable random disk file access.
 */
class TSYS_EXPORT tsysRandomFile : public tsysFile
{
private:
  Error errid;
  char *errstr;

protected:
#ifndef WIN32
  int    fd;
  Access access;
#else
  HANDLE fd;
#endif
  char * pathname;

  struct {
    unsigned count : 16;
    bool thrown    :  1;
    bool initial   :  1;
#ifndef WIN32
    bool immediate :  1;
#endif
    bool temp      :  1;
  } flags;

  /**
   * Create an unopened random access file.
   */
  tsysRandomFile(const char *name = NULL);

  /**
   * Default copy constructor.
   */
  tsysRandomFile(const tsysRandomFile &rf);

  /**
   * Post an error event.
   *
   * @return error code.
   * @param errid error code.
   * @param errstr error message string.
   */
  Error error(Error errid, char *errstr = NULL);

  /**
   * Post an extended string error message.
   *
   * @return errExtended.
   * @param err error string.
   */
  inline Error error(char *err) {return error(errExtended, err);};

  /**
   * Used to enable or disable throwing of exceptions on
   * errors.
   *
   * @param enable true if errors will be thrown.
   */
  inline void setError(bool enable) {flags.thrown = !enable;};

#ifndef WIN32
  /**
   * Used to set file completion modes.
   *
   * @return errSuccess if okay.
   * @param mode completion mode.
   * @todo implement in win32
   */
  Error setCompletion(Complete mode);
#endif

  /**
   * Used to set the temporary attribute for the file.  Temporary
   * files are automatically deleted when closed.
   *
   * @param enable true for marking as temporary.
   */
  inline void setTemporary(bool enable) {flags.temp = enable;};

  /**
   * This method is used to initialize a newly created file as
   * indicated by the "initial" flag.  This method also returns
   * the file access permissions that should be associated with
   * the file.  This method should never be called directly, but
   * is instead used to impliment the "Initial" method.  Typically
   * one would use this to build an empty database shell when a
   * previously empty database file is created.
   *
   * @return access, or attrInvalid if should be removed.
   */
  virtual Attr initialize(void);

  /**
   * Close the file.
   */
  void final(void);

public:
  /**
   * Destroy a random access file or it's derived class.
   */
  virtual ~tsysRandomFile();

  /**
   * This method should be called right after a tsysRandomFile derived
   * object has been created.  This method will invoke initialize
   * if the object is newly created, and set file access permissions
   * appropriately.
   *
   * @return true if file had to be initialized.
   */
  bool initial(void);

  /**
   * Get current file capacity. Support over 2GB huge file.
   *
   * @return total file size in bytes.
   */
  fsize_64t length(void);
  fsize_64t getCapacity(void) { return length(); };

  /**
   * This method is commonly used to close and re-open an existing
   * database.  This may be used when the database has been unlinked
   * and an external process provides a new one to use.
   */
  virtual Error restart(void);

  /**
   * Return current error id.
   *
   * @return last error identifier set.
   */
  inline Error getErrorNumber(void)
    {
        return errid;
    };

  /**
   * Return current error string.
   *
   * @return last error string set.
   */
  inline char *getErrorString(void) {return errstr;};

  bool operator!(void);
};

//////////////////////////////////////////////////////////////////////////
/**
 * Create and map a disk file into memory.  This portable class works
 * under both POSIX via mmap and under the win32 API. A mapped file
 * can be referenced directly by it's memory segment. One can map
 * and unmap portions of a file on demand, and update
 * changed memory pages mapped from files immediately through sync().
 *
 * @author David Sugar <dyfet@ostel.com>
 * @short Map a named disk file into memory.
 */
class TSYS_EXPORT tsysMappedFile : public tsysRandomFile {
private:
  fcb_t fcb;
  int   prot;
#ifdef  WIN32
  HANDLE map;
  char   mapname[1024];

  SYSTEM_INFO sinf;
#else
  int flg;
#endif

public:
  /**
   * Open a file for mapping.  More than one segment of a file
   * may be mapped into separate regions of memory.
   *
   * @param fname file name to access for mapping.
   * @param mode  access mode to map file.
   */
  tsysMappedFile(const char *fname, Access mode);

  /**
   * Create if not exists, and map a file of specified size
   * into memory.
   *
   * @param fname file name to access for mapping.
   * @param mode access mode to map file.
   * @param size of file to map.
   */
  tsysMappedFile(const char *fname, Access mode, size_64t size);

  /**
   * Map a portion or all of a specified file in the specified
   * shared memory access mode.  Valid mapping modes include
   * mappedRead, mappedWrite, and mappedReadWrite.
   *
   * @param fname pathname of file to map into memory.
   * @param offset from start of file to begin mapping in bytes.
   * @param size of mapped area in bytes.
   * @param mode to map file.
   */
  tsysMappedFile(const char *fname, fpos_64t offset, size_64t size, Access mode);

  /**
   * Release a mapped section of memory associated with a file.  The
   * mapped area is updated back to disk.
   */
  virtual ~tsysMappedFile();

  /**
   * Synchronize the contents of the mapped portion of memory with
   * the disk file and wait for completion.  This assures the memory
   * mapped from the file is written back.
   */
  void sync(void);

  /**
   * Synchronize a segment of memory mapped from a segment fetch.
   *
   * @param address memory address to update.
   * @param len size of segment.
   */
  void sync(caddr_t address, size_t len);

  /**
   * Map a portion of the memory mapped from the file back to the
   * file and do not wait for completion.  This is useful when mapping
   * a database file and updating a single record.
   *
   * @param offset offset into the mapped region of memory.
   * @param len length of partial region (example, record length).
   */
  void update(size_t offset = 0, size_t len = 0);

  /**
   * Update a mapped region back to disk as specified by address
   * and length.
   *
   * @param address address of segment.
   * @param len length of segment.
   */
  void update(caddr_t address, size_t len);

  /**
   * Release (unmap) a memory segment.
   *
   * @param address address of memory segment to release.
   * @param len length of memory segment to release.
   */
  void release(caddr_t address, size_t len);

  /**
   * Fetch a pointer to an offset within the memory mapped portion
   * of the disk file.  This really is used for convenience of matching
   * operations between Update and Fetch, as one could simply have
   * accessed the base pointer where the file was mapped directly.
   *
   * @param offset from start of mapped memory.
   */
  inline caddr_t fetch(fpos_64t offset = 0) { return ((char *)(fcb.address)) + offset; };

  /**
   * Fetch and map a portion of a disk file to a logical memory
   * block.
   *
   * @return pointer to memory segment.
   * @param pos offset of file segment to map.
   * @param len size of memory segment to map.
   */
  caddr_t fetch(fpos_64t pos, tsys_ui32t len);

  /**
   * Lock the currently mapped portion of a file.
   *
   * @return true if pages are locked.
   */
  bool lock(void);

  /**
   * Unlock a locked mapped portion of a file.
   */
  void unlock(void);

  /**
   * Compute map size to aligned page boundary.
   *
   * @param size request.
   * @return page aligned size.
   */
  size_t pageAligned(size_t size);

  /**
   * Get the system page size.
   *
   * @param not requested.
   * @return system page size.
   */
  inline size_t getPageSize() { return this->systemPageSize; };

protected:
  /**
   * get the valid mapping view size ( 1 byte <= viewSize <= 1GB(win)/2GB(linux) )
   */
  tsys_ui32t checkMappingSize(size_64t len);

  /**
   * System Page Granularity or Page Size
   */
  size_t systemPageSize;
};

inline tsys_ui32t tsysMappedFile::checkMappingSize(size_64t len)
{
    size_64t newLen = len;

    if(newLen <= 0) newLen = TSYSFILE_DEF_VIEW_SIZE;

    fsize_64t fsz = this->length();
    if(fsz < newLen) newLen = fsz;

    if(TSYSFILE_MAX_VIEW_SIZE < newLen) newLen = TSYSFILE_MAX_VIEW_SIZE;

    return (tsys_ui32t)newLen;
}


//////////////////////////////////////////////////////////////////////////
//                  tsysLineStripper class
//////////////////////////////////////////////////////////////////////////
class TSYS_EXPORT tsysLineStripper {
public:
    /** goto the appointed "line" in the file */
    tsys_stat at(tsys_i32t lineNo);

    /** backward one or more "lines" in the file */
    tsys_stat backward(tsys_i32t numLine = 1);

    /** forward one or more "lines" in the file */
    tsys_stat forward(tsys_i32t numLine = 1);

    /** Position the "line" from the location indicated by origin argument */
    tsys_stat seek(tsys_i32t numLine, tsysFile::Position origin);

public:
    /** Get line No. */
    inline tsys_ui32t linNO() { return this->curLineNo; };

    /** Check if at the end of file */
    inline int isEOF() { return (this->eof(this->fileOffset + this->bufferLen)); };

public:
    /** Get offset bytes */
    inline fpos_64t getFileOffset() { return this->fileOffset; };

protected:
    /** Get a "line" data from file without feature code(s) at the current position */
    inline void strip(void *& data, tsys_ui32t &len) { assert(data == NULL); data = (void *)this->curLinePointer; len = this->nCharInCurLine; };

protected:
    /** read data from file to buffer */
    virtual void * readBuffer(fpos_64t begPos, tsys_ui32t len) = 0;

    /** check the validation of buffer size */
    virtual tsys_ui32t checkBufferSize(tsys_ui64t len) = 0;

    /** determine the offset address and size of next reading buffer */
    virtual int nextMappingOffsetandLen(fpos_64t &off, tsys_ui32t &len) = 0;
    /** determine the offset address and size of previous reading buffer */
    virtual int prevMappingOffsetandLen(fpos_64t &off, tsys_ui32t &len) = 0;

protected:
    char *featureCode;
    char *featureCode_rev; //for reverse seeking
    unsigned int featureCodeLength;

    unsigned int shift[256];
    unsigned int shift_rev[256];

    unsigned char * pBuffer;  //current data buffer
    tsys_ui32t bufferLen;     //the length of current buffer
    tsys_ui32t initBufferLen; //the initial buffer size determined by user

    fsize_64t  fsize;         //the whole size of file
    fpos_64t   fileOffset;    //the offset (at the beginning of current buffer) far away the beginning of whole file 

private:    
    tsys_ui32t curLineNo;     //the current line No.

    unsigned char *curLinePointer;  //to point the address of current line
    tsys_ui32t nCharInCurLine;      //number of characters in current line, excluding newline character(s)

    tsys_ui32t lineBeginPos;   //Range:[0, bufferLen]
    tsys_ui32t lineEndPos;     //Range:[0, bufferLen]

protected:
    //map next file block
    inline int mappingNextBlock(fpos_64t begPos, tsys_ui32t len);
    //map previous file block
    inline int mappingPrevBlock(fpos_64t begPos, tsys_ui32t len);

private:
    //move to the beginning of next line at the current position
    tsys_stat gotoNextLineBegin(tsys_ui32t &nextLinepos);
    //move to the beginning of previous line from the current position
    tsys_stat gotoPrevLineBegin(tsys_ui32t &prevLinepos);

private:
    // is beginning of file?
    inline int bof(fpos_64t fpos)    { return (fpos <= 0) ? 1 : 0; }
    // is end of file?
    inline int eof(fpos_64t fpos)    { return (fpos >= this->fsize) ? 1 : 0; }

    //is beginning of buffer?
    inline int bob(tsys_ui32t bpos ) { return (bpos <= 0) ? 1 : 0; }
    //is end of buffer?
    inline int eob(tsys_ui32t bpos ) { return (bpos >= this->bufferLen ) ? 1 : 0; }

private:
    //do some variables initialization
    void initialization();

public:
    tsysLineStripper();
    virtual ~tsysLineStripper();
};

inline int tsysLineStripper::mappingNextBlock(fpos_64t begPos, tsys_ui32t len)
{
    //@@preconditions
    if(this->eof(begPos) || len <= 0) return 0;
    //@@end preconditions

    this->bufferLen = this->checkBufferSize(len); 
    if( (begPos + this->bufferLen) > this->fsize) {
        this->bufferLen = (tsys_ui32t)(this->fsize - begPos);
    }
    
    this->pBuffer = (unsigned char *)this->readBuffer(begPos, this->bufferLen);
    if( this->pBuffer ) {
        this->fileOffset = begPos;
        return 1;
    }
    
    return 0;
}

inline int tsysLineStripper::mappingPrevBlock(fpos_64t begPos, tsys_ui32t len)
{
    if(len <= 0) return 0;
    
    this->bufferLen = len;
    
    this->pBuffer = (unsigned char *)this->readBuffer(begPos, this->bufferLen);
    if(this->pBuffer) {
        this->fileOffset = begPos;        
        return 1;
    }
    
    return 0;
}


//////////////////////////////////////////////////////////////////////////
//                    tsysASCIILineReader class
//////////////////////////////////////////////////////////////////////////
class TSYS_EXPORT tsysASCIILineReader : public tsysMappedFile, tsysLineStripper {
public:
    /** Trim Blanks Mode */
    enum BlankTrimMode {
        EAT_NONE  = -1,
        EAT_FRONT =  0,
        EAT_END,
        EAT_BOTH
    };
  typedef enum BlankTrimMode BlankTrimMode;

public:
    /** Get a line from file without newline character(s) at the current position */
    inline void getline(const char *&string, tsys_ui32t &len, BlankTrimMode mode = tsysASCIILineReader::EAT_NONE);

private:
    // do some pre-processing, such as newline character checking, initial buffer reading
    int preRead();

    // query which kind of newline character is.
    int checkNewLineChars(const char *buffer, tsys_ui32t len);

protected:
    inline void *readBuffer(fpos_64t begPos, tsys_ui32t len);
    inline tsys_ui32t checkBufferSize(size_64t len);

    inline int nextMappingOffsetandLen(fpos_64t &off, tsys_ui32t &len);
    inline int prevMappingOffsetandLen(fpos_64t &off, tsys_ui32t &len);

public:
    tsysASCIILineReader(const char *fname, tsys_ui32t mappingLen = TSYSFILE_DEF_VIEW_SIZE * 2 /* 1MB */);
    virtual ~tsysASCIILineReader();
};

//---------------------------------------------------------------------//
void tsysASCIILineReader::getline(const char *&string, tsys_ui32t &len, BlankTrimMode mode)
{
#ifdef _DEBUG
    void *data = NULL;
#else
    void *data;
#endif
    this->strip(data, len);

    assert(string == NULL);
    string = (const char *)data;

    if(mode != tsysASCIILineReader::EAT_NONE && len > 0){
        if(mode == tsysASCIILineReader::EAT_FRONT) {
            while((*string == ' ' || *string == '\t') && len > 0){
                string += 1;
                len -= 1;
            }
        }else if(mode == tsysASCIILineReader::EAT_END) {
            const char *ptr = string + len - 1;
            while((*ptr == ' ' || *ptr == '\t') && len > 0) {
                ptr -= 1;
                len -= 1;
            }
        }else if(mode == tsysASCIILineReader::EAT_BOTH) {
            const char *ptr = string + len - 1;
            while((*string == ' ' || *string == '\t') && len > 0){
                string += 1;
                len -= 1;
            }
            while((*ptr == ' ' || *ptr == '\t') && len > 0) {
                ptr -= 1;
                len -= 1;
            }
        }
    }
}

//######################################################################//
//                           OVERLOAD FUNCTIONS                         //
//######################################################################//
void *tsysASCIILineReader::readBuffer(fpos_64t begPos, tsys_ui32t len)
{
    assert(begPos >= 0);
    assert(len > 0);
    
    return (this->fetch(begPos, len));
}

tsys_ui32t tsysASCIILineReader::checkBufferSize(size_64t len)
{
    return (this->checkMappingSize(len));
}

int tsysASCIILineReader::nextMappingOffsetandLen(fpos_64t &off, tsys_ui32t &newLen)
{
    fpos_64t off_sav = off;
    
    tsys_ui32t backwardBytes = (tsys_ui32t)(off % this->systemPageSize);
    if(backwardBytes > 0){
        if(off <= backwardBytes)
            off = 0;
        else
            off -= backwardBytes;
    }
    assert(off <= off_sav);
    
    newLen += (tsys_ui32t)(off_sav - off);
    if(newLen >= this->initBufferLen)
        newLen += this->initBufferLen;
    else
        newLen = this->initBufferLen;

    return 1;
}

int tsysASCIILineReader::prevMappingOffsetandLen(fpos_64t &off, tsys_ui32t &newLen)
{
    fpos_64t off_sav = off;
    
    size_t nn = 0;
    if(newLen >= this->initBufferLen) {
        nn = 1;
    }else{
        nn = (this->initBufferLen - newLen) / this->systemPageSize;
        if(nn == 0) nn = 1;
    }
    
    if(off > nn*this->systemPageSize)
        off -= (nn*this->systemPageSize);
    else
        off = 0;
    
    newLen += (tsys_ui32t)(off_sav - off);
    
    return 1;
}

#endif //TSYS_FILE_H
