#include "vfs.hpp"

#include <sys/file.h>

#include <cassert>
#include <filesystem>
#include <iostream>

#include "glog/logging.h"
#include "sqlite3.h"
#include "utils.hpp"

namespace sls::vfs {

int file_close(sqlite3_file *file) {
  auto f = (DatabaseFile *)file;
  LOG(INFO) << "file_close: file=" << f << " name=" << f->name << std::endl;
  return SQLITE_OK;
}

int file_read(sqlite3_file *file, void *buf, int iAmt, sqlite3_int64 iOfst) {
  auto f = (DatabaseFile *)file;
  LOG(INFO) << "file_read: file=" << f << " name=" << f->name
            << " size=" << iAmt << " offset=" << iOfst << std::endl;

  memset(buf, 0, iAmt);

  auto rs = utils::read_from_file(f->name.c_str(), buf, iOfst, iAmt);
  if (rs < iAmt) {
    LOG(INFO) << "file_read: expectedRead=" << iAmt << " actualRead=" << rs
              << std::endl;
    return SQLITE_IOERR_SHORT_READ;
  }

  return SQLITE_OK;
}

int file_write(sqlite3_file *file, const void *buf, int iAmt,
               sqlite3_int64 iOfst) {
  auto f = (DatabaseFile *)file;
  f->block_size = iAmt;
  LOG(INFO) << "file_write: file=" << f << " name=" << f->name
            << " size=" << iAmt << " offset=" << iOfst << std::endl;

  auto ret = utils::write_to_file(f->name.c_str(), buf, iOfst, iAmt);
  if (ret < iAmt) {
    LOG(INFO) << "file_write: expectedWrite=" << iAmt << " actualWrite=" << ret
              << std::endl;
    return SQLITE_IOERR_WRITE;
  }
  return SQLITE_OK;
}

int file_truncate(sqlite3_file *file, sqlite3_int64 size) {
  auto f = (DatabaseFile *)file;
  LOG(INFO) << "file_truncate: file=" << f << " name=" << f->name << std::endl;
  return SQLITE_OK;
}

int file_sync(sqlite3_file *file, int flags) {
  auto f = (DatabaseFile *)file;
  LOG(INFO) << "file_sync: file=" << f << " name=" << f->name << std::endl;
  return SQLITE_OK;
}

int file_size(sqlite3_file *file, sqlite3_int64 *pSize) {
  auto f = (DatabaseFile *)file;
  LOG(INFO) << "file_size: file=" << f << " name=" << f->name << std::endl;

  if (!std::filesystem::exists(f->name)) {
    LOG(ERROR) << "file_size: file not exists: " << f->name << std::endl;
    return SQLITE_NOTFOUND;
  }

  *pSize = std::filesystem::file_size(f->name);
  LOG(INFO) << "file_size: size=" << *pSize << std::endl;

  return SQLITE_OK;
}

int file_lock(sqlite3_file *file, int) {
  auto f = (DatabaseFile *)file;
  LOG(INFO) << "file_lock: file=" << f << " name=" << f->name << std::endl;
  return SQLITE_OK;
}

int file_unlock(sqlite3_file *file, int) {
  auto f = (DatabaseFile *)file;
  LOG(INFO) << "file_unlock: file=" << f << " name=" << f->name << std::endl;
  return SQLITE_OK;
}

int file_check_reserved_lock(sqlite3_file *file, int *pResOut) {
  auto f = (DatabaseFile *)file;
  LOG(INFO) << "file_check_reserved_lock: file=" << f << " name=" << f->name
            << std::endl;
  return SQLITE_OK;
}

int file_control(sqlite3_file *file, int op, void *pArg) {
  auto f = (DatabaseFile *)file;
  LOG(INFO) << "file_control: file=" << f << " name=" << f->name << " op=" << op
            << std::endl;
  return SQLITE_NOTFOUND;
}

int file_sector_size(sqlite3_file *file) {
  auto f = (DatabaseFile *)file;
  LOG(INFO) << "file_sector_size: file=" << f << " name=" << f->name
            << std::endl;
  return 65536;
}

int file_device_characteristics(sqlite3_file *file) {
  auto f = (DatabaseFile *)file;
  LOG(INFO) << "file_device_characteristics: file=" << f << " name=" << f->name
            << std::endl;
  return 0 | SQLITE_IOCAP_ATOMIC64K | SQLITE_IOCAP_SAFE_APPEND |
         SQLITE_IOCAP_POWERSAFE_OVERWRITE | SQLITE_IOCAP_UNDELETABLE_WHEN_OPEN;
}

int vfs_open(sqlite3_vfs *vfs, const char *zName, sqlite3_file *file, int flags,
             int *pOutFlags) {
  LOG(INFO) << "vfs_open: vfs=" << vfs << " name=" << zName << " flags=0x"
            << std::hex << flags << std::endl;
  if (zName == nullptr) {
    LOG(ERROR) << "vfs_open: cannot open empty database name" << std::endl;
    return SQLITE_CANTOPEN;
  }

  auto path = std::string_view(zName);
  if (path == ":memory:") {
    LOG(ERROR) << "vfs_open: cannot open memory database" << std::endl;
    return SQLITE_IOERR;
  }

  // ensure file is exists, otherwise create it
  if (!utils::touch_file(zName)) {
    LOG(ERROR) << "vfs_open: cannot create file: " << zName << std::endl;
    return SQLITE_CANTOPEN;
  }

  int oflags = 0;
  if (flags & SQLITE_OPEN_EXCLUSIVE) oflags |= O_EXCL;
  if (flags & SQLITE_OPEN_CREATE) oflags |= O_CREAT;
  if (flags & SQLITE_OPEN_READONLY) oflags |= O_RDONLY;
  if (flags & SQLITE_OPEN_READWRITE) oflags |= O_RDWR;
  LOG(INFO) << "vfs_open: oflags=" << std::hex << oflags << std::endl;

  static sqlite3_io_methods methods = {
      .iVersion = 1,
      .xClose = file_close,
      .xRead = file_read,
      .xWrite = file_write,
      .xTruncate = file_truncate,
      .xSync = file_sync,
      .xFileSize = file_size,
      .xLock = file_lock,
      .xUnlock = file_unlock,
      .xCheckReservedLock = file_check_reserved_lock,
      .xFileControl = file_control,
      .xSectorSize = file_sector_size,
      .xDeviceCharacteristics = file_device_characteristics,
  };

  auto f = (DatabaseFile *)file;
  memset(f, 0, sizeof(DatabaseFile));
  auto name = std::string(zName);
  f->name = name;
  f->base.pMethods = &methods;

  return SQLITE_OK;
}

int vfs_delete(sqlite3_vfs *vfs, const char *zName, int syncDir) {
  LOG(INFO) << "vfs_delete: vfs=" << vfs << " name=" << zName
            << " syncDir=" << syncDir << std::endl;
  if (std::filesystem::exists(zName)) {
    if (std::filesystem::remove(zName)) {
      return SQLITE_OK;
    } else {
      LOG(ERROR) << "vfs_delete: cannot delete file: " << zName << std::endl;
      return SQLITE_IOERR_DELETE;
    }
  } else {
    LOG(ERROR) << "vfs_delete: file not exists: " << zName << std::endl;
    return SQLITE_IOERR_DELETE;
  }
}

int vfs_access(sqlite3_vfs *vfs, const char *zName, int flags, int *pResOut) {
  LOG(INFO) << "vfs_access: vfs=" << vfs << " flags=" << std::hex << flags
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
  LOG(INFO) << "vfs_access: pResOut=" << std::hex << *pResOut << std::endl;
  return SQLITE_OK;
}

int vfs_full_pathname(sqlite3_vfs *vfs, const char *zName, int nOut,
                      char *zOut) {
  LOG(INFO) << "vfs_full_pathname: vfs=" << vfs << " name=" << zName
            << std::endl;
  strcpy(zOut, zName);
  return SQLITE_OK;
}

void *vfs_dl_open(sqlite3_vfs *vfs, const char *zFilename) {
  LOG(INFO) << "vfs_dl_open: vfs=" << vfs << std::endl;
  return SQLITE_OK;
}

void vfs_dl_error(sqlite3_vfs *vfs, int nByte, char *zErrMsg) {
  LOG(INFO) << "vfs_dl_error: vfs=" << vfs << " nByte=" << nByte << std::endl;
  sqlite3_snprintf(nByte, zErrMsg, "Loadable extensions are not supported");
  zErrMsg[nByte - 1] = '\0';
}

void (*vfs_dl_sym(sqlite3_vfs *vfs, void *pH, const char *zSymbol))(void) {
  LOG(INFO) << "vfs_dl_sym: vfs=" << vfs << std::endl;
  return SQLITE_OK;
}

void vfs_dl_close(sqlite3_vfs *vfs, void *pHandle) {
  LOG(INFO) << "vfs_dl_close: vfs=" << vfs << std::endl;
}

int vfs_randomness(sqlite3_vfs *vfs, int nByte, char *zOut) {
  LOG(INFO) << "vfs_randomness: vfs=" << vfs << " nByte=" << nByte << std::endl;
  return SQLITE_OK;
}

int vfs_sleep(sqlite3_vfs *vfs, int microseconds) {
  LOG(INFO) << "vfs_sleep: vfs=" << vfs << " microseconds=" << microseconds
            << std::endl;
  sleep(microseconds / 1000000);
  usleep(microseconds % 1000000);
  return microseconds;
}

int vfs_current_time(sqlite3_vfs *vfs, double *) {
  LOG(INFO) << "vfs_current_time: vfs=" << vfs << std::endl;
  return SQLITE_OK;
}

// https://www.sqlite.org/capi3ref.html
// https://github.com/sqlite/sqlite/blob/master/src/test_demovfs.c
void register_vfs() {
  static auto slsVfs = sqlite3_vfs{
      .iVersion = 1,
      .szOsFile = sizeof(DatabaseFile),
      .mxPathname = 1024,
      .pNext = 0,
      .zName = "slsql",
      .pAppData = 0,
      .xOpen = vfs_open,
      .xDelete = vfs_delete,
      .xAccess = vfs_access,
      .xFullPathname = vfs_full_pathname,
      .xDlOpen = vfs_dl_open,
      .xDlError = vfs_dl_error,
      .xDlSym = vfs_dl_sym,
      .xDlClose = vfs_dl_close,
      .xRandomness = vfs_randomness,
      .xSleep = vfs_sleep,
      .xCurrentTime = vfs_current_time,
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