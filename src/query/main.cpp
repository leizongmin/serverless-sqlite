#include <filesystem>
#include <iostream>

#include "glog/logging.h"
#include "sqlite3.h"
#include "utils.hpp"
#include "vfs.hpp"

// dump sqlite exec results
int dump_exec(void *arg, int argc, char **argv, char **column) {
  int i;
  LOG(ERROR) << std::endl;
  for (i = 0; i < argc; i++) {
    LOG(ERROR) << column[i] << ": " << argv[i] << std::endl;
  }
  LOG(ERROR) << "------" << std::endl;
  return 0;
}

// write example file
void write_example_file(void) {
  char s1[128] = "Hello\0World!";
  char s2[128] = "Just for test!";
  sls::utils::write_to_file("example.txt", s1, 0, sizeof(s1));
  sls::utils::write_to_file("example.txt", s2, sizeof(s1), sizeof(s2));
  char buf[12];
  sls::utils::read_from_file("example.txt", buf, 0, sizeof(buf));
  LOG(ERROR) << "Read: " << buf << std::endl;
}

int test_sqlite_unix_vfs(const std::string dbUrl, const int flags) {
  sqlite3 *db;
  auto rc = sqlite3_open_v2(dbUrl.c_str(), &db, flags, nullptr);
  if (rc != SQLITE_OK) {
    LOG(ERROR) << "cannot open database: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_close(db);
    return 1;
  }

  // change the page size to 64K bytes
  rc = sqlite3_exec(db, "PRAGMA page_size = 65536;", dump_exec, nullptr,
                    nullptr);
  if (rc != SQLITE_OK) {
    LOG(ERROR) << "cannot set pragma: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_close(db);
    return 1;
  }

  rc = sqlite3_exec(db, "CREATE TABLE test (id INTEGER PRIMARY KEY, name TEXT)",
                    dump_exec, nullptr, nullptr);
  if (rc != SQLITE_OK) {
    LOG(ERROR) << "cannot create table: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_close(db);
    return 1;
  }

  rc = sqlite3_exec(db,
                    "INSERT INTO test (id, name) VALUES (1, 'hello'), (2, "
                    "'world'), (3, 'OK')",
                    dump_exec, nullptr, nullptr);
  if (rc != SQLITE_OK) {
    LOG(ERROR) << "cannot insert rows: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_close(db);
    return 1;
  }

  rc = sqlite3_exec(db, "SELECT * FROM test", dump_exec, nullptr, nullptr);
  if (rc != SQLITE_OK) {
    LOG(ERROR) << "cannot select rows: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_close(db);
    return 1;
  }

  rc = sqlite3_close(db);
  if (rc != SQLITE_OK) {
    LOG(ERROR) << "cannot close: " << sqlite3_errmsg(db) << std::endl;
    return 1;
  }

  LOG(ERROR) << "database created" << std::endl;
  return 0;
}

void test_register_sls_vfs(void) {
  LOG(ERROR) << "register new vfs" << std::endl;
  sls::vfs::register_vfs();
}

int test_sls_vfs(const std::string dbUrl, const int flags) {
  sqlite3 *db;
  auto rc = sqlite3_open_v2(dbUrl.c_str(), &db, flags, nullptr);
  if (rc != SQLITE_OK) {
    LOG(ERROR) << "cannot open database: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_close(db);
    return 1;
  }

  // turn off journal_mode
  rc = sqlite3_exec(db, "PRAGMA journal_mode = PERSIST;", dump_exec, nullptr,
                    nullptr);
  if (rc != SQLITE_OK) {
    LOG(ERROR) << "cannot set pragma: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_close(db);
    return 1;
  }

  LOG(ERROR) << "select rows" << std::endl;
  rc = sqlite3_exec(db, "SELECT * FROM test", dump_exec, nullptr, nullptr);
  if (rc != SQLITE_OK) {
    LOG(ERROR) << "cannot select rows: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_close(db);
    return 1;
  }

  LOG(ERROR) << "insert rows" << std::endl;
  rc = sqlite3_exec(db, "INSERT INTO test (id, name) VALUES (4, 'Oh...')",
                    dump_exec, nullptr, nullptr);
  if (rc != SQLITE_OK) {
    LOG(ERROR) << "cannot insert rows: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_close(db);
    return 1;
  }

  LOG(ERROR) << "select rows" << std::endl;
  rc = sqlite3_exec(db, "SELECT * FROM test", dump_exec, nullptr, nullptr);
  if (rc != SQLITE_OK) {
    LOG(ERROR) << "cannot select rows: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_close(db);
    return 1;
  }

  LOG(ERROR) << "close database" << std::endl;
  rc = sqlite3_close(db);
  if (rc != SQLITE_OK) {
    LOG(ERROR) << "cannot close: " << sqlite3_errmsg(db) << std::endl;
    return 1;
  }

  return 0;
}

int main(int argc, char **argv, char **envp) {
  sls::utils::dump_env(envp);
  google::InitGoogleLogging(argv[0]);

  LOG(ERROR) << "CWD: " << std::filesystem::current_path().c_str() << std::endl;
  LOG(ERROR) << "SQLite version: " << sqlite3_libversion() << std::endl;

  write_example_file();

  // create an example database and demo data
  const std::string dbName = "test.db";
  const std::string dbUrl = "file:" + dbName + "?cache=shared";
  const int flags =
      SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_URI;
  std::filesystem::remove(dbName);
  int code = 0;
  code = test_sqlite_unix_vfs(dbUrl, flags);
  if (code != 0) {
    return code;
  }

  // register the slsql vfs, and test it
  test_register_sls_vfs();
  code = test_sls_vfs(dbUrl, flags);
  if (code != 0) {
    return code;
  }

  LOG(ERROR) << "end" << std::endl;
  return 0;
}
