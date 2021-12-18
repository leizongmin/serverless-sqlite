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
- `pageSize`: EVIDENCE-OF: R-51873-39618 The page size for a database file is determined by the 2-byte integer located
  at an offset of 16 bytes from the beginning of the database file.
    - `pBt->pageSize = (zDbHeader[16]<<8) | (zDbHeader[17]<<16)`
- transaction:
    - [sqlite3PagerCommitPhaseOne](https://github.com/sqlite/sqlite/blob/master/src/pager.c).
    - [sqlite3PagerCommitPhaseTwo](https://github.com/sqlite/sqlite/blob/master/src/pager.c).
    - [sqlite3PagerRollback](https://github.com/sqlite/sqlite/blob/master/src/pager.c).

## Logs

### journal_mode=PERSIST

#### Open Database

- vfs_full_pathname: name=test.db
- vfs_full_pathname: name=test.db
- vfs_open: name=test.db flags=0x146 -- **open db file**
- file_device_characteristics: name=test.db
- file_device_characteristics: name=test.db
- file_device_characteristics: name=test.db
- file_read: name=test.db size=100 offset=0 -- **read db [0:100] at 1st page**
- file_control: name=test.db op=15
- file_control: name=test.db op=30
- file_control: name=test.db op=14
- file_lock: name=test.db -- **lock db file**
- vfs_access: flags=0
- vfs_access: flags=0
- file_size: name=test.db -- **get db size**
- file_read: name=test.db size=65536 offset=0 -- **read db 1st page**
- file_device_characteristics: name=test.db
- file_unlock: name=test.db -- **unlock db file**

#### Select Data First Time

- file_lock: name=test.db -- **lock db file**
- vfs_access: flags=0
- file_read: name=test.db size=16 offset=24 -- **read db [24:40] at 1st page**
- vfs_access: flags=0
- file_size: name=test.db -- **get db size**
- file_read: name=test.db size=65536 offset=65536 -- **read db 2nd page**
- file_device_characteristics: name=test.db
- file_unlock: name=test.db -- **unlock db file**

#### Select Data Again

- file_lock: name=test.db -- **lock db file**
- file_check_reserved_lock: name=test.db
- file_size: name=test.db -- **get db size**
- file_read: file=0x1b073e0 name=test.db-journal size=1 offset=0 -- **read journal [0:1]**
- file_read: name=test.db size=16 offset=24 -- **read db [24:40] at 1st page**
- vfs_access: flags=0
- file_size: name=test.db -- **get db size**
- file_device_characteristics: name=test.db
- file_unlock: name=test.db -- **unlock db file**

#### Insert Data

Part 1:

- file_lock: name=test.db -- **lock db file**
- vfs_access: flags=0
- file_read: name=test.db size=16 offset=24 -- **read db [24:40] at 1st page**
- vfs_access: flags=0
- file_size: name=test.db -- **get db size**
- file_lock: name=test.db -- **lock db file**

Part 2:

- file_control: name=test.db op=20
- vfs_open: name=test.db-journal flags=0x806 -- **open journal file**
- file_device_characteristics: name=test.db
- file_write: file=0x1b073e0 name=test.db-journal size=512 offset=0 -- **write journal [0:512]**
- file_write: file=0x1b073e0 name=test.db-journal size=4 offset=512 -- **write journal [512:516]**
- file_write: file=0x1b073e0 name=test.db-journal size=65536 offset=516 -- **write journal [516:66052]**
- file_write: file=0x1b073e0 name=test.db-journal size=4 offset=66052 -- **write journal [66052:66056]**

Part 3:

- file_lock: name=test.db -- **lock db file**
- file_write: file=0x1b073e0 name=test.db-journal size=4 offset=66056 -- **write journal [66056:66060]**
- file_write: file=0x1b073e0 name=test.db-journal size=65536 offset=66060 -- **write journal [66060:131596]**
- file_write: file=0x1b073e0 name=test.db-journal size=4 offset=131596 -- **write journal [131596:131600]**
- file_device_characteristics: name=test.db
- file_sync: file=0x1b073e0 name=test.db-journal -- **sync journal file**

Part 3:

- file_write: name=test.db size=65536 offset=0 -- **write db 1st page**
- file_write: name=test.db size=65536 offset=65536 -- **write db 2nd page**
- file_control: name=test.db op=21
- file_sync: name=test.db -- **sync db file**

Part 4:

- file_write: file=0x1b073e0 name=test.db-journal size=28 offset=0 -- **write journal [0:28]**
- file_sync: file=0x1b073e0 name=test.db-journal -- **sync journal file**
- file_control: name=test.db op=22
- file_unlock: name=test.db -- **unlock db file**
- file_device_characteristics: name=test.db
- file_unlock: name=test.db -- **unlock db file**

#### Close Database

- file_control: name=test.db op=20
- file_sync: file=0x1b073e0 name=test.db-journal -- **sync journal file**
- file_size: file=0x1b073e0 name=test.db-journal -- **get journal size**
- file_device_characteristics: name=test.db
- file_unlock: name=test.db -- **unlock db file**
- file_close: file=0x1b073e0 name=test.db-journal -- **close journal file**
- file_close: name=test.db -- **close db file**

### journal_mode=OFF

#### Open Database

- vfs_full_pathname: name=test.db
- vfs_full_pathname: name=test.db
- vfs_open: name=test.db flags=0x146 -- **open db file**
- file_device_characteristics: name=test.db
- file_device_characteristics: name=test.db
- file_device_characteristics: name=test.db
- file_read: name=test.db size=100 offset=0 -- **read db [0:100] at 1st page**
- file_control: name=test.db op=15
- file_control: name=test.db op=30
- file_control: name=test.db op=14
- file_lock: name=test.db -- **lock db file**
- vfs_access: flags=0
- vfs_access: flags=0
- file_size: name=test.db -- **get db size**
- file_read: name=test.db size=65536 offset=0 -- **read db 1st page**
- file_device_characteristics: name=test.db
- file_unlock: name=test.db -- **unlock db file**

#### Select Data First Time

- file_lock: name=test.db -- **lock db file**
- vfs_access: flags=0
- file_read: name=test.db size=16 offset=24 -- **read db [24:40] at 1st page**
- vfs_access: flags=0
- file_size: name=test.db -- **get db size**
- file_read: name=test.db size=65536 offset=65536 -- **read db 2nd page**
- file_device_characteristics: name=test.db
- file_unlock: name=test.db -- **unlock db file**

#### Select Data Again

- file_lock: name=test.db -- **lock db file**
- vfs_access: flags=0
- file_read: name=test.db size=16 offset=24 -- **read db [24:40] at 1st page**
- vfs_access: flags=0
- file_size: name=test.db -- **get db size**
- file_device_characteristics: name=test.db
- file_unlock: name=test.db -- **unlock db file**

#### Insert Data

Part 1:

- file_lock: name=test.db -- **lock db file**
- vfs_access: flags=0
- file_read: name=test.db size=16 offset=24 -- **read db [24:40] at 1st page**
- vfs_access: flags=0
- file_size: name=test.db -- **get db size**
- file_lock: name=test.db -- **lock db file**

Part 2:

- file_lock: name=test.db -- **lock db file**
- file_write: name=test.db size=65536 offset=0 -- **write db 1st page**
- file_write: name=test.db size=65536 offset=65536 -- **write db 2nd page**
- file_control: name=test.db op=21
- file_sync: name=test.db -- **sync db file**

Part 3:

- file_control: name=test.db op=22
- file_unlock: name=test.db -- **unlock db file**
- file_device_characteristics: name=test.db
- file_unlock: name=test.db -- **unlock db file**

#### Close Database

- file_control: name=test.db op=20
- file_device_characteristics: name=test.db
- file_unlock: name=test.db -- **unlock db file**
- file_close: name=test.db -- **close db file**
