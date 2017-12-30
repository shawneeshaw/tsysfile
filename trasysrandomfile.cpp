#include <stdlib.h>
#include <string.h>

#include "trasysfile.h"


#ifdef WIN32
#define MAKEINT64(Hi,Lo)   ((tsys_i64t)(((tsys_i64t)((DWORD)(Hi))) << 32 | ((DWORD)(Lo))))
#endif


//////////////////////////////////////////////////////////////////////////
tsysRandomFile::tsysRandomFile(const char *name) {
#ifdef WIN32
  fd = INVALID_HANDLE_VALUE;
#else
  fd = -1;
  flags.immediate = false;
#endif
  flags.thrown = flags.initial = flags.temp = false;
  flags.count = 0;

#ifdef _WIN32
  pathname = _strdup(name);
#else
  pathname = strdup(name);
#endif

  errid = errSuccess;
}

tsysRandomFile::tsysRandomFile(const tsysRandomFile &rf) {
#ifdef WIN32
  HANDLE pidHandle = ::GetCurrentProcess();
  HANDLE dupHandle;

  if(rf.fd != INVALID_HANDLE_VALUE) {
    if(!::DuplicateHandle(pidHandle, rf.fd, pidHandle, &dupHandle, 0, FALSE, DUPLICATE_SAME_ACCESS))
      fd = INVALID_HANDLE_VALUE;
    else
      fd = dupHandle;
  }
  else
    fd = INVALID_HANDLE_VALUE;

#else
  if(rf.fd > -1)
    fd = dup(rf.fd);
  else
    fd = -1;

#endif

  flags = rf.flags;
  flags.count = 0;

  if(rf.pathname) {
     if(pathname) free(pathname);
#ifdef _WIN32
     pathname = _strdup(rf.pathname);
#else
     pathname = strdup(rf.pathname);
#endif
  } else {
     pathname = NULL;
  }

  errid = errSuccess;
}

tsysRandomFile::~tsysRandomFile()
{
  final();
}

tsysFile::Error tsysRandomFile::restart(void)
{
  return errOpenFailed;
}

tsysFile::Attr tsysRandomFile::initialize(void)
{
  return attrPublic;
}

void tsysRandomFile::final(void)
{
#ifdef WIN32
  if(fd != INVALID_HANDLE_VALUE) {
     ::CloseHandle(fd);
     if(flags.temp && pathname) ::DeleteFile(pathname);
  }

#else
  if(fd > -1) {
    close(fd);
    if(flags.temp && pathname) remove(pathname);
  }
#endif

  if(pathname) {
    free(pathname);
    pathname = NULL;
  }

#ifdef WIN32
  fd = INVALID_HANDLE_VALUE;
#else
  fd = -1;
#endif
  flags.count   = 0;
  flags.initial = false;
}

tsysRandomFile::Error tsysRandomFile::error(Error id, char *str)
{
  errstr = str;
  errid = id;
  if(!flags.thrown) {
    flags.thrown = true;
  }
  return id;
}

bool tsysRandomFile::initial(void)
{
  bool init;

#ifdef WIN32
  if(fd == INVALID_HANDLE_VALUE) return false;
#else
  if(fd < 0) return false;
#endif

  init = flags.initial;
  flags.initial = false;

  if(!init) return false;

#ifdef WIN32
  Attr access = initialize();
  if(access == attrInvalid) {
    ::CloseHandle(fd);
    if(pathname) ::DeleteFile(pathname);

    fd = INVALID_HANDLE_VALUE;
    error(errInitFailed);
    return false;
  }

#else
  int mode = (int)initialize();
  if(!mode) {
    close(fd);
    fd = -1;
    if(pathname) remove(pathname);

    error(errInitFailed);
    return false;
  }
  fchmod(fd, mode);
#endif

  return init;
}

#ifndef WIN32
tsysRandomFile::Error tsysRandomFile::setCompletion(Complete mode)
{
  long flag = fcntl(fd, F_GETFL);

  if(fd < 0)
    return errNotOpened;

  flags.immediate = false;
#ifdef O_SYNC
  flag &= ~(O_SYNC | O_NDELAY);
#else
  flag &= ~O_NDELAY;
#endif
  switch(mode) {
  case completionImmediate:
#ifdef O_SYNC
    flag |= O_SYNC;
#endif
    flags.immediate = true;
    break;

  case completionDelayed:
    flag |= O_NDELAY;

  case completionDeferred:
    break;
  }
  fcntl(fd, F_SETFL, flag);
  return errSuccess;
}
#endif

fsize_64t tsysRandomFile::length(void)
{
#ifdef WIN32
  if(!fd)
#else
  if(fd < 0)
#endif
    return 0;

  fsize_64t sz = 0;
#ifdef WIN32
  DWORD dwLo = 0l, dwHi = 0l;

  dwLo = ::GetFileSize(fd, &dwHi);
  if(dwLo == INVALID_FILE_SIZE && GetLastError() != NO_ERROR){
     sz = 0;
  }else{
     sz = MAKEINT64(dwHi, dwLo);
  }

#else
  lseek(fd, (off_t)0, SEEK_SET);
  off_t pos = lseek(fd, (off_t)0, SEEK_CUR);
  off_t eof = lseek(fd, (off_t)0, SEEK_END);
  sz = (fsize_64t)eof;
#endif
  return sz;
}

bool tsysRandomFile::operator!(void)
{
#ifdef WIN32
  return (fd == INVALID_HANDLE_VALUE);
#else
  if(fd < 0)
    return true;

  return false;
#endif
}
