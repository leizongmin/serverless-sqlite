# serverless-sqlite

Serverless SQLite database read from and write to Object Storage Service, run on FaaS platform.

**NOTES: This repository is still in the early development stage.**

## Goals

- Use Object Storage Service as the underlying storage engine, such as AWS S3, Aliyun OSS, etc.
- Run on FaaS platform, multiple query instances can share the same database, can be scaled horizontally.
- Provide query interface via HTTP protocol.

## Why?

This is my first C++ project. I'm studying how to build a serverless database recently. Just for fun.

## Development

### Requirements

- C++ 17 compiler, such as GCC 9, Clang 10.
- Unix-like operating system, such as Linux, FreeBSD, macOS.
- Development tools:
  - **clang-format**: used to format the source code. Run the following command to install it:
    - Ubuntu or Debian: `apt install clang-format`
    - macOS: `brew install clang-format`

### Setup

- Initialize git submodule: `git submodule init && git submodule update`
- Install `sqlite3` library: `./vcpkg/vcpkg install sqlite3`
- Install `glog` library: `./vcpkg/vcpkg install glog`

## Reference

### libcephsqlite

This SQLite VFS may be used for storing and accessing a SQLite database backed by RADOS. This allows you to fully decentralize your database using Ceph’s object store for improved availability, accessibility, and use of storage.
Link: https://docs.ceph.com/en/latest/rados/api/libcephsqlite/

> - `PRAGMA page_size = 65536` - bigger page size
> - `PRAGMA cache_size = 4096` - cache 4096 pages or 256MB (with 64K page_cache).
> - `PRAGMA journal_mode = PERSIST` - don't deletes the journal for every transaction
> - `PRAGMA temp_store=memory` - put the temporary tables on memory

### sqlite-s3vfs

Python virtual filesystem for SQLite to read from and write to S3.
Link: https://github.com/uktrade/sqlite-s3vfs

> sqlite-s3vfs stores the SQLite database in **fixed-sized blocks, and each is stored as a separate object** in S3. SQLite stores its data in fixed-size pages, and always **writes exactly a page at a time**. This virtual filesystem **translates page reads and writes to block reads and writes**. In the case of SQLite pages being the **same size** as blocks, which is the case by default, each page write results in exactly one block write.
>
> Separate objects are required since S3 does not support the partial replace of an object; to change even 1 byte, it must be re-uploaded in full.

## License

```text
MIT License

Copyright (c) 2021 Zongmin Lei <leizongmin@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
