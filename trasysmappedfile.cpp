#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "trasysfile.h"


//////////////////////////////////////////////////////////////////////////
size_t tsysMappedFile::pageAligned(size_t size)
{
	size_t pages = size / this->getPageSize();

	if(size % this->getPageSize()) ++pages;

	return pages * this->getPageSize();
}


//------------------------------------------------------------------------
#ifdef WIN32
#define LODWORD(l)         ((DWORD)(l))
#define HIDWORD(l)         ((DWORD)(((tsys_i64t)(l) >> 32) & 0xFFFFFFFF))

static void makemapname(const char *source, char *target)
{
	unsigned count = 1020;

	while(*source && count--) {
		if(*source == '/' || *source == '\\')
			*(target++) = '_';
		else
			*(target++) = ::toupper(*source);
		++source;
	}

	*target = 0;
}

tsysMappedFile::tsysMappedFile(const char *fname, Access mode, size_64t size) : tsysRandomFile(fname)
{
	DWORD share, page;
	map = INVALID_HANDLE_VALUE;
	fcb.address = NULL;

	switch(mode) {
		case accessReadOnly:
            {
		        share = FILE_SHARE_READ;
		        page  = PAGE_READONLY;
		        prot  = FILE_MAP_READ;
            }
    		break;
		case accessWriteOnly:
            {
		        share = FILE_SHARE_WRITE;
		        page  = PAGE_WRITECOPY;
		        prot  = FILE_MAP_COPY;
            }
		    break;
		case accessReadWrite:
            {
		        share = FILE_SHARE_READ | FILE_SHARE_WRITE;
		        page  = PAGE_READWRITE;
		        prot  = FILE_MAP_WRITE;
            }
            break;
        default:
            break;
	}

    fd = ::CreateFile(pathname,
                      mode,
                      share,
                      NULL,
                      OPEN_ALWAYS,
                      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
                      NULL);
	if(fd == INVALID_HANDLE_VALUE) {
		error(errOpenFailed);
		return;
	}

    makemapname(fname, mapname);
    map = ::CreateFileMapping(fd, NULL, page, HIDWORD(size), LODWORD(size), mapname);
	if(!map) error(errMapFailed);

#ifdef GCC
	fcb.address = (char *)MapViewOfFile(map, prot, 0, 0, this->checkMappingSize(size));
#else
    fcb.address = ::MapViewOfFile(map, prot, 0, 0, this->checkMappingSize(size));
#endif
	fcb.len = (ccxx_size_t)size;
	fcb.pos = 0;
	if(!fcb.address) error(errMapFailed);

    ::GetSystemInfo(&this->sinf);
    this->systemPageSize = this->sinf.dwAllocationGranularity;    
}

tsysMappedFile::tsysMappedFile(const char *fname, Access mode) : tsysRandomFile(fname)
{
	DWORD share, page, dwCreationDisposition, dwFlagsAndAttributes;
	map = INVALID_HANDLE_VALUE;
	fcb.address = NULL;

	switch(mode) {
	    case accessReadOnly:
            {
		        share = FILE_SHARE_READ;
		        page  = PAGE_READONLY;
		        prot  = FILE_MAP_READ;
                dwCreationDisposition = OPEN_EXISTING;
                dwFlagsAndAttributes  = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS;
            }
		    break;
	    case accessWriteOnly:
            {
		        share = FILE_SHARE_WRITE;
		        page  = PAGE_WRITECOPY;
		        prot  = FILE_MAP_COPY;
                dwCreationDisposition = OPEN_ALWAYS;
                dwFlagsAndAttributes  = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS;
            }
		    break;
	    case accessReadWrite:
            {
		        share = FILE_SHARE_READ | FILE_SHARE_WRITE;
		        page  = PAGE_READWRITE;
		        prot  = FILE_MAP_WRITE;
                dwCreationDisposition = OPEN_ALWAYS;
                dwFlagsAndAttributes  = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS;
            }
            break;
        default:
            break;
	}

    fd = ::CreateFile(pathname,
                      mode,
                      share,
                      NULL,
                      dwCreationDisposition,
                      dwFlagsAndAttributes,
                      NULL);
	if(fd == INVALID_HANDLE_VALUE) {
        if( mode == accessReadOnly ) {
            share = FILE_SHARE_READ | FILE_SHARE_WRITE;   //change mode to open again
            fd = ::CreateFile(pathname,
                              mode,
                              share,
                              NULL,
                              dwCreationDisposition,
                              dwFlagsAndAttributes,
                              NULL);
            if(fd == INVALID_HANDLE_VALUE) {
                error(errOpenFailed);
                return;
            }
        }
        else {
            error(errOpenFailed);
            return;
        }
	}
	
    makemapname(fname, mapname);
    map = ::CreateFileMapping(fd, NULL, page, 0, 0, mapname);
	if(!map) error(errMapFailed);

    ::GetSystemInfo(&this->sinf);
    this->systemPageSize = this->sinf.dwAllocationGranularity;
}

tsysMappedFile::tsysMappedFile(const char *fname, fpos_64t pos, size_64t len, Access mode) : tsysRandomFile(fname)
{
	DWORD share, page;
	map = INVALID_HANDLE_VALUE;
	fcb.address = NULL;

	switch(mode) {
    	case accessReadOnly:
            {
	    	    share = FILE_SHARE_READ;
		        page  = PAGE_READONLY;
		        prot  = FILE_MAP_READ;
            }
		    break;
	    case accessWriteOnly:
            {
                share = FILE_SHARE_WRITE;
		        page  = PAGE_WRITECOPY;
		        prot  = FILE_MAP_COPY;
            }
		    break;
	    case accessReadWrite:
            {
		        share = FILE_SHARE_READ | FILE_SHARE_WRITE;
		        page  = PAGE_READWRITE;
		        prot  = FILE_MAP_WRITE;
            }
            break;
        default:
            break;
	}

    fd = ::CreateFile(pathname,
                      mode,
                      share,
                      NULL,
                      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
                      NULL);
	if(fd == INVALID_HANDLE_VALUE) {
		error(errOpenFailed);
		return;
	}
	
    makemapname(fname, mapname);
    map = ::CreateFileMapping(fd, NULL, page, 0, 0, mapname);
	if(!map) {
		error(errMapFailed);
		return;
	}

#ifdef GCC
    fcb.address = (char *)MapViewOfFile(map, prot, HIDWORD(pos), LODWORD(pos), this->checkMappingSize(len));
#else
    fcb.address = ::MapViewOfFile(map, prot, HIDWORD(pos), LODWORD(pos), this->checkMappingSize(len));
#endif
	fcb.len = (ccxx_size_t)len;
	fcb.pos = pos;
	if(!fcb.address) error(errMapFailed);

    ::GetSystemInfo(&this->sinf);
    this->systemPageSize = this->sinf.dwAllocationGranularity;
}

tsysMappedFile::~tsysMappedFile()
{
	if(fcb.address) {
		unlock();
        ::UnmapViewOfFile(fcb.address);
	}

    if(map != INVALID_HANDLE_VALUE) ::CloseHandle(map);

	final();
}

void tsysMappedFile::sync(void)
{
}

void tsysMappedFile::sync(caddr_t address, size_t len)
{
}

void tsysMappedFile::release(caddr_t address, size_t len)
{
	if(fcb.address) {
		unlock();
        ::UnmapViewOfFile(fcb.address);
	}
	fcb.address = NULL;
}

caddr_t tsysMappedFile::fetch(fpos_64t pos, tsys_ui32t len)
{
	if(fcb.address) {
		unlock();
        ::UnmapViewOfFile(fcb.address);
	}

#ifdef GCC
    fcb.address = (char *)MapViewOfFile(map, prot, HIDWORD(pos), LODWORD(pos), this->checkMappingSize(len));
#else
    fcb.address = ::MapViewOfFile(map, prot, HIDWORD(pos), LODWORD(pos), this->checkMappingSize(len));
#endif
	fcb.len = (ccxx_size_t)len;
	fcb.pos = pos;
    if(!fcb.address){
        error(errMapFailed);
    }

	return fcb.address;
}

void tsysMappedFile::update(size_t offset, size_t len)
{
}

void tsysMappedFile::update(caddr_t address, size_t len)
{
}

bool tsysMappedFile::lock(void)
{
	unlock();
    if(::VirtualLock(fcb.address, fcb.len)) fcb.locked = true;

	return fcb.locked;
}

void tsysMappedFile::unlock(void)
{
	if(!fcb.address) fcb.locked = false;

	if(!fcb.locked) return;

    ::VirtualUnlock(fcb.address, fcb.len);
	fcb.locked = false;
}

#else  //Linux
tsysMappedFile::tsysMappedFile(const char *fname, Access mode, size_64t size) : tsysRandomFile(fname)
{
    fd = open(fname, (int)mode | O_CREAT, 0660);
    
    if(fd < 0) {
        error(errOpenFailed);
        return;
    }
    
    switch(mode) {
    case O_RDONLY:
        prot = PROT_READ;
        break;
    case O_WRONLY:
        prot = PROT_WRITE;
        break;
    default:
        prot = PROT_READ | PROT_WRITE;
    }
    
    flg = MAP_SHARED;

    lseek(fd, (off_t)size, SEEK_SET);
    fcb.address = (caddr_t)mmap(NULL, size, prot, flg, fd, (off_t)0);
    fcb.len = size;
    fcb.pos = 0;
    if((caddr_t)(fcb.address) == (caddr_t)(MAP_FAILED)) {
        close(fd);
        fd = -1;
        error(errMapFailed);
    }

    this->systemPageSize = (size_t)sysconf(_SC_PAGESIZE);
}

tsysMappedFile::tsysMappedFile(const char *fname, Access mode) : tsysRandomFile(fname)
{
	fd = open(fname, (int)mode);
    if(fd < 0 && mode != accessReadOnly) {
		fd = ::open(pathname, O_CREAT | O_RDWR | O_TRUNC, (int)attrPrivate);
    }

	if(fd < 0) {
		error(errOpenFailed);
		return;
	}

	switch(mode) {
	case O_RDONLY:
		prot = PROT_READ;
		break;
	case O_WRONLY:
		prot = PROT_WRITE;
		break;
	default:
		prot = PROT_READ | PROT_WRITE;
	}

    flg = MAP_PRIVATE | MAP_NORESERVE;

    this->systemPageSize = (size_t)sysconf(_SC_PAGESIZE);
}

tsysMappedFile::tsysMappedFile(const char *fname, fpos_64t pos, size_64t len, Access mode) : tsysRandomFile(fname)
{
	fd = open(fname, (int)mode);
	if(fd < 0) {
		error(errOpenFailed);
		return;
	}

	switch(mode) {
	case O_RDONLY:
		prot = PROT_READ;
		break;
	case O_WRONLY:
		prot = PROT_WRITE;
		break;
	default:
		prot = PROT_READ | PROT_WRITE;
	}

    flg = MAP_SHARED;

	lseek(fd, (off_t)(pos + len), SEEK_SET);
	fcb.address = (caddr_t)mmap(NULL, len, prot, flg, fd, (off_t)pos);
	fcb.len = len;
	fcb.pos = pos;
	if((caddr_t)(fcb.address) == (caddr_t)(MAP_FAILED)) {
		close(fd);
		fd = -1;
		error(errMapFailed);
	}

    this->systemPageSize = (size_t)sysconf(_SC_PAGESIZE);
}

tsysMappedFile::~tsysMappedFile()
{
	unlock();
	final();
}

void tsysMappedFile::sync(void)
{
	msync(fcb.address, fcb.len, MS_SYNC);
}

void tsysMappedFile::sync(caddr_t address, size_t len)
{
    msync(address, len, MS_SYNC);
}

void tsysMappedFile::release(caddr_t address, size_t len)
{
	if(address)
		fcb.address = address;

	if(len)
		fcb.len = (ccxx_size_t)len;

	if(fcb.locked)
		unlock();

	munmap(fcb.address, fcb.len);
}

caddr_t tsysMappedFile::fetch(fpos_64t pos, tsys_ui32t len)
{
    if(fcb.address) {
        unlock();
        munmap(fcb.address, fcb.len);
    }

    fcb.len = len;
	fcb.pos = pos;
	lseek(fd, (off_t)(fcb.pos + len), SEEK_SET);
	fcb.address = (caddr_t)mmap(NULL, len, prot, MAP_SHARED, fd, (off_t)pos);
    if(!fcb.address){
        error(errMapFailed);
    }

	return fcb.address;
}

bool tsysMappedFile::lock(void)
{
	unlock();
	if(!mlock(fcb.address, fcb.len)) fcb.locked = true;

	return fcb.locked;
}

void tsysMappedFile::unlock(void)
{
	if(!fcb.address) fcb.locked = false;

	if(!fcb.locked)	return;

	munlock(fcb.address, fcb.len);
	fcb.locked = false;
}

void tsysMappedFile::update(size_t offset, size_t len)
{
	int mode = MS_ASYNC;
	caddr_t address;

	if(flags.immediate)	mode = MS_SYNC;

	address = fcb.address;
	address += offset;
	if(!len) len = fcb.len;
	msync(address, len, mode);
}

void tsysMappedFile::update(caddr_t address, size_t len)
{
	int mode = MS_ASYNC;
	if(flags.immediate) mode = MS_SYNC;

	msync(address, len, mode);
}
#endif // ndef WIN32
