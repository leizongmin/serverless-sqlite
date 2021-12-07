## Summary

- call `lock` every time before query or update, and call `unlock` after operation.
- call `lock`, `unlock`, `size` every operation.
  - at [sqlite3OsLock](https://github.com/sqlite/sqlite/blob/master/src/os.c).
  - at [pagerLockDb](https://github.com/sqlite/sqlite/blob/master/src/pager.c).
  - at [pager_wait_on_lock](https://github.com/sqlite/sqlite/blob/master/src/pager.c).
    - `do { rc = pagerLockDb(pPager, locktype); }while( rc==SQLITE_BUSY && pPager->xBusyHandler(pPager->pBusyHandlerArg) )`
    - [sqliteDefaultBusyCallback](https://github.com/sqlite/sqlite/blob/master/src/main.c).
      - [sqlite3OsSleep](https://github.com/sqlite/sqlite/blob/master/src/os.c) sleep 1000,000μs (1s).
  - at [sqlite3PagerSharedLock](https://github.com/sqlite/sqlite/blob/master/src/pager.c).
  - at [lockBtree](https://github.com/sqlite/sqlite/blob/master/src/btree.c).
  - at [sqlite3BtreeBeginTrans](https://github.com/sqlite/sqlite/blob/master/src/btree.c).
- `read` the data of 1st page a lot.
  - read `[0:100]` on open.
    - at [sqlite3OsRead](https://github.com/sqlite/sqlite/blob/master/src/os.c).
    - at [sqlite3PagerReadFileheader](https://github.com/sqlite/sqlite/blob/master/src/pager.c).
    - at [sqlite3BtreeOpen](https://github.com/sqlite/sqlite/blob/master/src/btree.c), zDbHeader=100.
    - at [openDatabase](https://github.com/sqlite/sqlite/blob/master/src/main.c), open the backend database driver.
  - read `[24:40]` on query or update.
    - at [sqlite3OsRead](https://github.com/sqlite/sqlite/blob/master/src/os.c).
    - at [sqlite3PagerSharedLock](https://github.com/sqlite/sqlite/blob/master/src/pager.c).
    - at [lockBtree](https://github.com/sqlite/sqlite/blob/master/src/btree.c).
    - at [sqlite3BtreeBeginTrans](https://github.com/sqlite/sqlite/blob/master/src/btree.c).
- always call `sync` at the end of one or multiple `write`.
  - at [sqlite3OsSync](https://github.com/sqlite/sqlite/blob/master/src/os.c).
  - at [sqlite3PagerSync](https://github.com/sqlite/sqlite/blob/master/src/pager.c).
  - at [sqlite3PagerCommitPhaseOne](https://github.com/sqlite/sqlite/blob/master/src/pager.c).
- `pageSize`: EVIDENCE-OF: R-51873-39618 The page size for a database file is
  determined by the 2-byte integer located at an offset of 16 bytes from the beginning of the database file.
  - `pBt->pageSize = (zDbHeader[16]<<8) | (zDbHeader[17]<<16)`
- transaction:
  - [sqlite3PagerCommitPhaseOne](https://github.com/sqlite/sqlite/blob/master/src/pager.c).
  - [sqlite3PagerCommitPhaseTwo](https://github.com/sqlite/sqlite/blob/master/src/pager.c).
  - [sqlite3PagerRollback](https://github.com/sqlite/sqlite/blob/master/src/pager.c).

## Logs

### journal_mode=PERSIST

#### Open Database

- slsVfsFullPathname: name=test.db
- slsVfsFullPathname: name=test.db
- slsVfsOpen: name=test.db flags=0x146 -- **open db file**
- slsFileDeviceCharacteristics: name=test.db
- slsFileDeviceCharacteristics: name=test.db
- slsFileDeviceCharacteristics: name=test.db
- slsFileRead: name=test.db size=100 offset=0 -- **read db [0:100] at 1st page**
- slsFileFileControl: name=test.db op=15
- slsFileFileControl: name=test.db op=30
- slsFileFileControl: name=test.db op=14
- slsFileLock: name=test.db -- **lock db file**
- slsVfsAccess: flags=0
- slsVfsAccess: flags=0
- slsFileFileSize: name=test.db -- **get db size**
- slsFileRead: name=test.db size=65536 offset=0 -- **read db 1st page**
- slsFileDeviceCharacteristics: name=test.db
- slsFileUnlock: name=test.db -- **unlock db file**

#### Select Data First Time

- slsFileLock: name=test.db -- **lock db file**
- slsVfsAccess: flags=0
- slsFileRead: name=test.db size=16 offset=24 -- **read db [24:40] at 1st page**
- slsVfsAccess: flags=0
- slsFileFileSize: name=test.db -- **get db size**
- slsFileRead: name=test.db size=65536 offset=65536 -- **read db 2nd page**
- slsFileDeviceCharacteristics: name=test.db
- slsFileUnlock: name=test.db -- **unlock db file**

#### Select Data Again

- slsFileLock: name=test.db -- **lock db file**
- slsFileCheckReservedLock: name=test.db
- slsFileFileSize: name=test.db -- **get db size**
- slsFileRead: file=0x1b073e0 name=test.db-journal size=1 offset=0 -- **read journal [0:1]**
- slsFileRead: name=test.db size=16 offset=24 -- **read db [24:40] at 1st page**
- slsVfsAccess: flags=0
- slsFileFileSize: name=test.db -- **get db size**
- slsFileDeviceCharacteristics: name=test.db
- slsFileUnlock: name=test.db -- **unlock db file**

#### Insert Data

Part 1:

- slsFileLock: name=test.db -- **lock db file**
- slsVfsAccess: flags=0
- slsFileRead: name=test.db size=16 offset=24 -- **read db [24:40] at 1st page**
- slsVfsAccess: flags=0
- slsFileFileSize: name=test.db -- **get db size**
- slsFileLock: name=test.db -- **lock db file**

Part 2:

- slsFileFileControl: name=test.db op=20
- slsVfsOpen: name=test.db-journal flags=0x806 -- **open journal file**
- slsFileDeviceCharacteristics: name=test.db
- slsFileWrite: file=0x1b073e0 name=test.db-journal size=512 offset=0 -- **write journal [0:512]**
- slsFileWrite: file=0x1b073e0 name=test.db-journal size=4 offset=512 -- **write journal [512:516]**
- slsFileWrite: file=0x1b073e0 name=test.db-journal size=65536 offset=516 -- **write journal [516:66052]**
- slsFileWrite: file=0x1b073e0 name=test.db-journal size=4 offset=66052 -- **write journal [66052:66056]**

Part 3:

- slsFileLock: name=test.db -- **lock db file**
- slsFileWrite: file=0x1b073e0 name=test.db-journal size=4 offset=66056 -- **write journal [66056:66060]**
- slsFileWrite: file=0x1b073e0 name=test.db-journal size=65536 offset=66060 -- **write journal [66060:131596]**
- slsFileWrite: file=0x1b073e0 name=test.db-journal size=4 offset=131596 -- **write journal [131596:131600]**
- slsFileDeviceCharacteristics: name=test.db
- slsFileSync: file=0x1b073e0 name=test.db-journal -- **sync journal file**

Part 3:

- slsFileWrite: name=test.db size=65536 offset=0 -- **write db 1st page**
- slsFileWrite: name=test.db size=65536 offset=65536 -- **write db 2nd page**
- slsFileFileControl: name=test.db op=21
- slsFileSync: name=test.db -- **sync db file**

Part 4:

- slsFileWrite: file=0x1b073e0 name=test.db-journal size=28 offset=0 -- **write journal [0:28]**
- slsFileSync: file=0x1b073e0 name=test.db-journal -- **sync journal file**
- slsFileFileControl: name=test.db op=22
- slsFileUnlock: name=test.db -- **unlock db file**
- slsFileDeviceCharacteristics: name=test.db
- slsFileUnlock: name=test.db -- **unlock db file**

#### Close Database

- slsFileFileControl: name=test.db op=20
- slsFileSync: file=0x1b073e0 name=test.db-journal -- **sync journal file**
- slsFileFileSize: file=0x1b073e0 name=test.db-journal -- **get journal size**
- slsFileDeviceCharacteristics: name=test.db
- slsFileUnlock: name=test.db -- **unlock db file**
- slsFileClose: file=0x1b073e0 name=test.db-journal -- **close journal file**
- slsFileClose: name=test.db -- **close db file**

### journal_mode=OFF

#### Open Database

- slsVfsFullPathname: name=test.db
- slsVfsFullPathname: name=test.db
- slsVfsOpen: name=test.db flags=0x146 -- **open db file**
- slsFileDeviceCharacteristics: name=test.db
- slsFileDeviceCharacteristics: name=test.db
- slsFileDeviceCharacteristics: name=test.db
- slsFileRead: name=test.db size=100 offset=0 -- **read db [0:100] at 1st page**
- slsFileFileControl: name=test.db op=15
- slsFileFileControl: name=test.db op=30
- slsFileFileControl: name=test.db op=14
- slsFileLock: name=test.db -- **lock db file**
- slsVfsAccess: flags=0
- slsVfsAccess: flags=0
- slsFileFileSize: name=test.db -- **get db size**
- slsFileRead: name=test.db size=65536 offset=0 -- **read db 1st page**
- slsFileDeviceCharacteristics: name=test.db
- slsFileUnlock: name=test.db -- **unlock db file**

#### Select Data First Time

- slsFileLock: name=test.db -- **lock db file**
- slsVfsAccess: flags=0
- slsFileRead: name=test.db size=16 offset=24 -- **read db [24:40] at 1st page**
- slsVfsAccess: flags=0
- slsFileFileSize: name=test.db -- **get db size**
- slsFileRead: name=test.db size=65536 offset=65536 -- **read db 2nd page**
- slsFileDeviceCharacteristics: name=test.db
- slsFileUnlock: name=test.db -- **unlock db file**

#### Select Data Again

- slsFileLock: name=test.db -- **lock db file**
- slsVfsAccess: flags=0
- slsFileRead: name=test.db size=16 offset=24 -- **read db [24:40] at 1st page**
- slsVfsAccess: flags=0
- slsFileFileSize: name=test.db -- **get db size**
- slsFileDeviceCharacteristics: name=test.db
- slsFileUnlock: name=test.db -- **unlock db file**

#### Insert Data

Part 1:

- slsFileLock: name=test.db -- **lock db file**
- slsVfsAccess: flags=0
- slsFileRead: name=test.db size=16 offset=24 -- **read db [24:40] at 1st page**
- slsVfsAccess: flags=0
- slsFileFileSize: name=test.db -- **get db size**
- slsFileLock: name=test.db -- **lock db file**

Part 2:

- slsFileLock: name=test.db -- **lock db file**
- slsFileWrite: name=test.db size=65536 offset=0 -- **write db 1st page**
- slsFileWrite: name=test.db size=65536 offset=65536 -- **write db 2nd page**
- slsFileFileControl: name=test.db op=21
- slsFileSync: name=test.db -- **sync db file**

Part 3:

- slsFileFileControl: name=test.db op=22
- slsFileUnlock: name=test.db -- **unlock db file**
- slsFileDeviceCharacteristics: name=test.db
- slsFileUnlock: name=test.db -- **unlock db file**

#### Close Database

- slsFileFileControl: name=test.db op=20
- slsFileDeviceCharacteristics: name=test.db
- slsFileUnlock: name=test.db -- **unlock db file**
- slsFileClose: name=test.db -- **close db file**
