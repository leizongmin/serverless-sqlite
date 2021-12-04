#include "vfs.hpp"

#include <sys/file.h>

#include <cassert>
#include <filesystem>
#include <iostream>

#include "glog/logging.h"
#include "sqlite3.h"
#include "utils.hpp"

namespace sls::vfs {

int slsFileClose(sqlite3_file *file) {
  auto f = (slsFile *)file;
  LOG(INFO) << "slsFileClose: file=" << f << " name=" << f->name << std::endl;
  return SQLITE_OK;
}

int slsFileRead(sqlite3_file *file, void *buf, int iAmt, sqlite3_int64 iOfst) {
  auto f = (slsFile *)file;
  LOG(INFO) << "slsFileRead: file=" << f << " name=" << f->name
            << " size=" << iAmt << " offset=" << iOfst << std::endl;

  memset(buf, 0, iAmt);

  auto rs = utils::readFromFile(f->name.c_str(), buf, iOfst, iAmt);
  if (rs < iAmt) {
    LOG(INFO) << "slsFileRead: expectedRead=" << iAmt << " actualRead=" << rs
              << std::endl;
    return SQLITE_IOERR_SHORT_READ;
  }

  return SQLITE_OK;
}

int slsFileWrite(sqlite3_file *file, const void *buf, int iAmt,
                 sqlite3_int64 iOfst) {
  auto f = (slsFile *)file;
  f->blockSize = iAmt;
  LOG(INFO) << "slsFileWrite: file=" << f << " name=" << f->name
            << " size=" << iAmt << " offset=" << iOfst << std::endl;

  auto ret = utils::writeToFile(f->name.c_str(), buf, iOfst, iAmt);
  if (ret < iAmt) {
    LOG(INFO) << "slsFileWrite: expectedWrite=" << iAmt
              << " actualWrite=" << ret << std::endl;
    return SQLITE_IOERR_WRITE;
  }
  return SQLITE_OK;
}

int slsFileTruncate(sqlite3_file *file, sqlite3_int64 size) {
  auto f = (slsFile *)file;
  LOG(INFO) << "slsFileTruncate: file=" << f << " name=" << f->name
            << std::endl;
  return SQLITE_OK;
}

int slsFileSync(sqlite3_file *file, int flags) {
  auto f = (slsFile *)file;
  LOG(INFO) << "slsFileSync: file=" << f << " name=" << f->name << std::endl;
  return SQLITE_OK;
}

int slsFileFileSize(sqlite3_file *file, sqlite3_int64 *pSize) {
  auto f = (slsFile *)file;
  LOG(INFO) << "slsFileFileSize: file=" << f << " name=" << f->name
            << std::endl;

  if (!std::filesystem::exists(f->name)) {
    LOG(ERROR) << "slsFileFileSize: file not exists: " << f->name << std::endl;
    return SQLITE_NOTFOUND;
  }

  *pSize = std::filesystem::file_size(f->name);
  LOG(INFO) << "slsFileFileSize: size=" << *pSize << std::endl;

  return SQLITE_OK;
}

int slsFileLock(sqlite3_file *file, int) {
  auto f = (slsFile *)file;
  LOG(INFO) << "slsFileLock: file=" << f << " name=" << f->name << std::endl;
  return SQLITE_OK;
}

int slsFileUnlock(sqlite3_file *file, int) {
  auto f = (slsFile *)file;
  LOG(INFO) << "slsFileUnlock: file=" << f << " name=" << f->name << std::endl;
  return SQLITE_OK;
}

int slsFileCheckReservedLock(sqlite3_file *file, int *pResOut) {
  auto f = (slsFile *)file;
  LOG(INFO) << "slsFileCheckReservedLock: file=" << f << " name=" << f->name
            << std::endl;
  return SQLITE_OK;
}

int slsFileFileControl(sqlite3_file *file, int op, void *pArg) {
  auto f = (slsFile *)file;
  LOG(INFO) << "slsFileFileControl: file=" << f << " name=" << f->name
            << " op=" << op << std::endl;
  return SQLITE_NOTFOUND;
}

int slsFileSectorSize(sqlite3_file *file) {
  auto f = (slsFile *)file;
  LOG(INFO) << "slsFileSectorSize: file=" << f << " name=" << f->name
            << std::endl;
  return 65536;
}

int slsFileDeviceCharacteristics(sqlite3_file *file) {
  auto f = (slsFile *)file;
  LOG(INFO) << "slsFileDeviceCharacteristics: file=" << f << " name=" << f->name
            << std::endl;
  return 0 | SQLITE_IOCAP_ATOMIC64K | SQLITE_IOCAP_SAFE_APPEND |
         SQLITE_IOCAP_POWERSAFE_OVERWRITE | SQLITE_IOCAP_UNDELETABLE_WHEN_OPEN;
}

int slsVfsOpen(sqlite3_vfs *vfs, const char *zName, sqlite3_file *file,
               int flags, int *pOutFlags) {
  LOG(INFO) << "slsVfsOpen: vfs=" << vfs << " name=" << zName << " flags=0x"
            << std::hex << flags << std::endl;
  if (zName == nullptr) {
    LOG(ERROR) << "slsVfsOpen: cannot open empty database name" << std::endl;
    return SQLITE_CANTOPEN;
  }

  auto path = std::string_view(zName);
  if (path == ":memory:") {
    LOG(ERROR) << "slsVfsOpen: cannot open memory database" << std::endl;
    return SQLITE_IOERR;
  }

  // ensure file is exists, otherwise create it
  if (!utils::touchFile(zName)) {
    LOG(ERROR) << "slsVfsOpen: cannot create file: " << zName << std::endl;
    return SQLITE_CANTOPEN;
  }

  int oflags = 0;
  if (flags & SQLITE_OPEN_EXCLUSIVE) oflags |= O_EXCL;
  if (flags & SQLITE_OPEN_CREATE) oflags |= O_CREAT;
  if (flags & SQLITE_OPEN_READONLY) oflags |= O_RDONLY;
  if (flags & SQLITE_OPEN_READWRITE) oflags |= O_RDWR;
  LOG(INFO) << "slsVfsOpen: oflags=" << std::hex << oflags << std::endl;

  static sqlite3_io_methods methods = {
      .iVersion = 1,
      .xClose = slsFileClose,
      .xRead = slsFileRead,
      .xWrite = slsFileWrite,
      .xTruncate = slsFileTruncate,
      .xSync = slsFileSync,
      .xFileSize = slsFileFileSize,
      .xLock = slsFileLock,
      .xUnlock = slsFileUnlock,
      .xCheckReservedLock = slsFileCheckReservedLock,
      .xFileControl = slsFileFileControl,
      .xSectorSize = slsFileSectorSize,
      .xDeviceCharacteristics = slsFileDeviceCharacteristics,
  };

  auto f = (slsFile *)file;
  memset(f, 0, sizeof(slsFile));
  auto name = std::string(zName);
  f->name = name;
  f->base.pMethods = &methods;

  return SQLITE_OK;
}

int slsVfsDelete(sqlite3_vfs *vfs, const char *zName, int syncDir) {
  LOG(INFO) << "slsVfsDelete: vfs=" << vfs << " name=" << zName
            << " syncDir=" << syncDir << std::endl;
  if (std::filesystem::exists(zName)) {
    if (std::filesystem::remove(zName)) {
      return SQLITE_OK;
    } else {
      LOG(ERROR) << "slsVfsDelete: cannot delete file: " << zName << std::endl;
      return SQLITE_IOERR_DELETE;
    }
  } else {
    LOG(ERROR) << "slsVfsDelete: file not exists: " << zName << std::endl;
    return SQLITE_IOERR_DELETE;
  }
}

int slsVfsAccess(sqlite3_vfs *vfs, const char *zName, int flags, int *pResOut) {
  LOG(INFO) << "slsVfsAccess: vfs=" << vfs << " flags=" << std::hex << flags
            << std::endl;
  int rc;             /* access() return code */
  int eAccess = F_OK; /* Second argument to access() */

  assert(flags == SQLITE_ACCESS_EXISTS       /* access(zPath, F_OK) */
         || flags == SQLITE_ACCESS_READ      /* access(zPath, R_OK) */
         || flags == SQLITE_ACCESS_READWRITE /* access(zPath, R_OK|W_OK) */
  );

  if (flags == SQLITE_ACCESS_READWRITE) eAccess = R_OK | W_OK;
  if (flags == SQLITE_ACCESS_READ) eAccess = R_OK;

  rc = access(zName, eAccess);
  *pResOut = (rc == 0);
  LOG(INFO) << "slsVfsAccess: pResOut=" << std::hex << *pResOut << std::endl;
  return SQLITE_OK;
}

int slsVfsFullPathname(sqlite3_vfs *vfs, const char *zName, int nOut,
                       char *zOut) {
  LOG(INFO) << "slsVfsFullPathname: vfs=" << vfs << " name=" << zName
            << std::endl;
  strcpy(zOut, zName);
  return SQLITE_OK;
}

void *slsVfsDlOpen(sqlite3_vfs *vfs, const char *zFilename) {
  LOG(INFO) << "slsVfsDlOpen: vfs=" << vfs << std::endl;
  return SQLITE_OK;
}

void slsVfsDlError(sqlite3_vfs *vfs, int nByte, char *zErrMsg) {
  LOG(INFO) << "slsVfsDlError: vfs=" << vfs << " nByte=" << nByte << std::endl;
  sqlite3_snprintf(nByte, zErrMsg, "Loadable extensions are not supported");
  zErrMsg[nByte - 1] = '\0';
}

void (*slsVfs_slsVfsDlSym(sqlite3_vfs *vfs, void *pH,
                          const char *zSymbol))(void) {
  LOG(INFO) << "slsVfs_slsVfsDlSym: vfs=" << vfs << std::endl;
  return SQLITE_OK;
}

void slsVfsDlClose(sqlite3_vfs *vfs, void *pHandle) {
  LOG(INFO) << "slsVfsDlClose: vfs=" << vfs << std::endl;
}

int slsVfsRandomness(sqlite3_vfs *vfs, int nByte, char *zOut) {
  LOG(INFO) << "slsVfsRandomness: vfs=" << vfs << " nByte=" << nByte
            << std::endl;
  return SQLITE_OK;
}

int slsVfsSleep(sqlite3_vfs *vfs, int microseconds) {
  LOG(INFO) << "slsVfsSleep: vfs=" << vfs << " microseconds=" << microseconds
            << std::endl;
  sleep(microseconds / 1000000);
  usleep(microseconds % 1000000);
  return microseconds;
}

int slsVfsCurrentTime(sqlite3_vfs *vfs, double *) {
  LOG(INFO) << "slsVfsCurrentTime: vfs=" << vfs << std::endl;
  return SQLITE_OK;
}

// https://www.sqlite.org/capi3ref.html
// https://github.com/sqlite/sqlite/blob/master/src/test_demovfs.c
void registerVfs() {
  static auto slsVfs = sqlite3_vfs{
      .iVersion = 1,
      .szOsFile = sizeof(slsFile),
      .mxPathname = 1024,
      .pNext = 0,
      .zName = "slsql",
      .pAppData = 0,
      .xOpen = slsVfsOpen,
      .xDelete = slsVfsDelete,
      .xAccess = slsVfsAccess,
      .xFullPathname = slsVfsFullPathname,
      .xDlOpen = slsVfsDlOpen,
      .xDlError = slsVfsDlError,
      .xDlSym = slsVfs_slsVfsDlSym,
      .xDlClose = slsVfsDlClose,
      .xRandomness = slsVfsRandomness,
      .xSleep = slsVfsSleep,
      .xCurrentTime = slsVfsCurrentTime,
      .xGetLastError = nullptr,
      .xCurrentTimeInt64 = nullptr,
      .xSetSystemCall = nullptr,
      .xGetSystemCall = nullptr,
      .xNextSystemCall = nullptr,
  };
  auto rc = sqlite3_vfs_register(&slsVfs, 1);
  if (rc != SQLITE_OK) {
    LOG(FATAL) << "cannot register slsql vfs: " << rc << std::endl;
  }
}
}  // namespace sls::vfs