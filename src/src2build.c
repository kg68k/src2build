// src2build
// Copyright (C) 2025 TcbnErik
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

static const char ProgramAndVersion[] = "src2build 0.0.0";
static const char Copyright[] = "2025 TcbnErik";

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _MSC_VER
#include <direct.h>

#include "dirent.h"
#else
#include <dirent.h>
#include <unistd.h>
#endif

#ifdef USE_DOSCALL
#include <sys/dos.h>
#include <sys/xglob.h>  // _errcnv(), _toslash()

enum { OPENMODE_READ = 0 };

// DOS形式の日時(0b_yyyy_yyym_mmmd_dddd_tttt_tfff_fffs_ssss)を変換せずに扱う。
// 最上位ビットまで使用しているので符号なしで扱うこと。
typedef unsigned int NativeTime;
#else
typedef time_t NativeTime;
#endif

#include "compat.h"

#ifdef _WIN32
#define PATH_DELIM "\\"
#else
#define PATH_DELIM "/"
#endif

// u8tosj.s
extern void CreateU2SArray(void);
extern size_t Utf8toSjis(size_t length, char* buffer);

#ifndef __human68k__
void CreateU2SArray(void) {}
size_t Utf8toSjis(size_t length, UNUSED char* buffer) { return length; }
#endif

typedef struct {
#if defined(USE_DOSCALL) && (FILENAME_MAX < (3 + 65))
#error "FILENAME_MAX is too small."
#endif
  char buf[FILENAME_MAX];
} CwdBuf;

enum {
  HUMAN68K_FILENAME_MAX = (18 + 1 + 3),
};

typedef struct {
  char name[HUMAN68K_FILENAME_MAX + 1];
  char isDirectory;
  NativeTime ctime;
  size_t size;
} Entry;

typedef struct {
  const char* path;
  size_t count;
  size_t capacity;
  Entry* list;
} Entries;

typedef struct {
  const char* fromPath;
  const char* toPath;
  const char* srcdirFilename;
  CwdBuf origCwd;
} Config;

static void Src2Build(const char* fromPath, const char* toPath);
static CwdBuf GetCwd(void);
static void Chdir(const char* path);
static void FileWrite(const char* path, const char* buf, size_t size);
#ifdef USE_DOSCALL
NORETURN static void DosFatal(const char* message, int rc) GCC_NORETURN;
#endif
NORETURN static void Fatal(const char* message) GCC_NORETURN;
NORETURN static void Fatal2(const char* mes1, const char* mes2) GCC_NORETURN;

static void Free(void* p) { free(p); }

static void* Malloc(size_t size) {
  void* p = malloc(size);
  if (!p) Fatal(nullptr);
  return p;
}

static void* Realloc(void* p, size_t size) {
  void* q = realloc(p, size);
  if (!q) Fatal(nullptr);
  return q;
}

static char* Strdup(char* s) {
  char* p = strdup(s);
  if (!p) Fatal(nullptr);
  return p;
}

static char* ConcatPath(const char* s1, const char* s2) {
  char* buf = Malloc(strlen(s1) + strlen(PATH_DELIM) + strlen(s2) + 1);
  strcat(strcat(strcpy(buf, s1), PATH_DELIM), s2);
  return buf;
}

static int FileExists(const char* path) {
#ifdef USE_DOSCALL
  return (_dos_chmod(path, -1) >= 0);
#else
  struct stat st;
  return (stat(path, &st) == 0);
#endif
}

static int DirExists(const char* path) {
#ifdef USE_DOSCALL
  int atr = _dos_chmod(path, -1);
  return (atr >= 0) && ((atr & _DOS_IFDIR) != 0);
#else
  struct stat st;
  return (stat(path, &st) == 0) && S_ISDIR(st.st_mode);
#endif
}

static int AscendToFile(const char* filename) {
  CwdBuf prevCwd = GetCwd();
  CwdBuf newCwd = {{0}};
  int chdirCount;

  for (chdirCount = 0;; chdirCount += 1) {
    // ディレクトリでもオープン時にエラーになるのでここでは気にしない
    if (FileExists(filename)) return chdirCount;

    Chdir("..");

    // chdir() 前後でカレントディレクトリが変わらなければ
    // ルートディレクトリに到達していたので、ファイル未発見で終了する。
    // なお chdir("..") では0が返されるので返り値での判定はできない。
    newCwd = GetCwd();
    if (strcmp(prevCwd.buf, newCwd.buf) == 0) return -1;

    prevCwd = newCwd;
  }
}

static void TrimRightControlCode(char* s) {
  char* t = s + strlen(s);
  while (s < t) {
    if (*--t >= 0x20) break;
    *t = '\0';
  }
}

static void FileGets(const char* path, size_t size, char* buf) {
#ifdef USE_DOSCALL
  struct _inpptr inpbuf;
  int rc;
  int fd = _dos_open(path, OPENMODE_READ);
  if (fd < 0) DosFatal(path, fd);

  inpbuf.max = (char)size;
  inpbuf.buffer[0] = '\0';
  rc = _dos_fgets(&inpbuf, fd);
  if (rc < 0) DosFatal(path, rc);

  _dos_close(fd);

  strcpy(buf, inpbuf.buffer);
#else
  FILE* file = fopen(path, "rb");
  if (!file) Fatal(path);
  if (!fgets(buf, size, file)) Fatal(path);
  fclose(file);
#endif
}

static char* GetSrcPath(int* outChdirCount, const char* filename) {
  char buf[FILENAME_MAX];

  *outChdirCount = AscendToFile(filename);
  if (*outChdirCount < 0) Fatal2(filename, " が見つかりません。");

  FileGets(filename, sizeof(buf), buf);
  TrimRightControlCode(buf);
  if (buf[0] == '\0') Fatal2(filename, " にパス名がありません。");

  return Strdup(buf);
}

static int InitSrcDir(Config* config) {
  char* s = ConcatPath(config->origCwd.buf, "src\n");
  FileWrite(config->srcdirFilename, s, strlen(s));
  Free(s);

  printf("%s を作成しました。\n", config->srcdirFilename);
  return EXIT_SUCCESS;
}

static int PrintVersion(const char* progAndVer, const char* copyright) {
  printf(
      "%s\n"
      "Copyright (C) %s\n"
      "\n"
      "This program is free software; you may redistribute it under the terms "
      "of\n"
      "the GNU General Public License version 3 or (at your option) any later "
      "version.\n"
      "This program has absolutely no warranty.\n",
      progAndVer, copyright);

  return EXIT_SUCCESS;
}

static int PrintHelp(const char* srcdir_txt) {
  printf(
      "usage: src2build [srcdir]\n"
      "  ソースディレクトリのファイルを"
#ifdef __human68k__
      "UTF-8からShift_JISに変換して"
#endif
      "buildディレクトリに書き込みます。\n"
      "  srcdirを省略した場合は %s を検索して読み込みます。\n",
      srcdir_txt);

  return EXIT_SUCCESS;
}

static int PrintError(const char* message) {
  fprintf(stderr, "%s\n", message);
  return EXIT_FAILURE;
}

static int ParseArguments(int argc, char* argv[], Config* config) {
  int i;
  for (i = 1; i < argc; i++) {
    char* arg = argv[i];

    if (arg[0] == '-') {
      if (strcmp(arg, "--init-srcdir") == 0) return InitSrcDir(config);

      if (strcmp(arg, "--version") == 0)
        return PrintVersion(ProgramAndVersion, Copyright);
      if (strcmp(arg, "--help") == 0) return PrintHelp(config->srcdirFilename);

      return PrintError("不正なオプションです。");
    }

    if (config->fromPath) return PrintError("複数のパスは指定できません。");
    config->fromPath = arg;
  }

  return -1;  // success
}

int main(int argc, char* argv[]) {
  Config config = {nullptr, "build", "srcdir.txt", GetCwd()};
  int chdirCount = 0;

  int r = ParseArguments(argc, argv, &config);
  if (r >= 0) return r;

  if (!config.fromPath) {
    config.fromPath = GetSrcPath(&chdirCount, config.srcdirFilename);
  }

  CreateU2SArray();
  Src2Build(config.fromPath, config.toPath);

  // srcdir.txt を探すために親ディレクトリに移動していた場合は、
  // 終了前にもとのディレクトリに戻す
  if (chdirCount) Chdir(config.origCwd.buf);

  return EXIT_SUCCESS;
}

static int Mkdir(const char* path) {
#ifdef USE_DOSCALL
  int rc = _dos_mkdir(path);
  if (rc < 0) {
    errno = _errcnv(rc);
    return -1;
  }
  return 0;
#else
  return mkdir(path, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
#endif
}

static Entries* CreateEntries(const char* path) {
  Entries* entries = Malloc(sizeof(*entries));
  *entries = (Entries){path, 0, 0, nullptr};
  return entries;
}

static void FreeEntries(Entries* entries) {
  Free(entries->list);
  Free(entries);
}

static void EnlargeEntryArray(Entries* entries) {
  const size_t initialCapacity = 16;
  const size_t increaseRatio = 2;

  size_t newCapacity =
      entries->capacity ? entries->capacity * increaseRatio : initialCapacity;
  entries->capacity = newCapacity;
  entries->list = Realloc(entries->list, sizeof(Entry) * newCapacity);
}

static void ShrinkEntryArray(Entries* entries) {
  size_t count = entries->count;
  if (entries->capacity == count) return;

  entries->capacity = count;
  entries->list = Realloc(entries->list, sizeof(Entry) * count);
}

static Entry* AllocateEntry(Entries* entries) {
  if (entries->capacity <= entries->count) EnlargeEntryArray(entries);

  return &entries->list[entries->count++];
}

static int isDotOrDotDot(const char* name) {
  if (name[0] == '.') {
    if (name[1] == '\0') return 1;  // "."
    if (name[1] == '.') {
      if (name[2] == '\0') return 1;  // ".."
    }
  }
  return 0;
}

#ifdef USE_DOSCALL

static Entry CreateEntryByFilbuf(const struct _filbuf* filbuf) {
  Entry entry = {
      {0},
      (filbuf->atr & _DOS_IFDIR) != 0,
      (filbuf->date << 16) | filbuf->time,
      filbuf->filelen  //
  };
  strcpy(entry.name, filbuf->name);
  return entry;
}

static Entries* ReadEntries(const char* path) {
  struct _filbuf filbuf;
  Entries* entries;

  char* wildcard = ConcatPath(path, "*.*");
  int rc = _dos_files(&filbuf, wildcard, 0xff);
  Free(wildcard);

  entries = CreateEntries(path);
  for (; rc >= 0; rc = _dos_nfiles(&filbuf)) {
    if (isDotOrDotDot(filbuf.name)) continue;
    if (filbuf.atr & (_DOS_IFVOL | _DOS_ISYS | _DOS_IHIDDEN)) continue;

    *(AllocateEntry(entries)) = CreateEntryByFilbuf(&filbuf);
  }
  ShrinkEntryArray(entries);

  return entries;
}

#else

static int IsDirOrFile(const struct dirent* dent) {
#if defined(__human68k__) && defined(__LIBC__)
  return S_ISDIR(dent->d_mode) || S_ISREG(dent->d_mode);
#else
  return dent->d_type == DT_DIR || dent->d_type == DT_REG;
#endif
}

static Entry CreateEntryByDirent(const struct dirent* dent, const char* path) {
  Entry entry;
  struct stat st;

  char* fullPath = ConcatPath(path, dent->d_name);
  if (stat(fullPath, &st) != 0) Fatal(fullPath);
  Free(fullPath);

  entry = (Entry){{0}, S_ISDIR(st.st_mode), st.st_ctime, st.st_size};
  // 呼び出し元で strlen(dent->d_name) <= HUMAN68K_FILENAME_MAX
  // を確認しておくこと
  strcpy(entry.name, dent->d_name);
  return entry;
}

static Entries* ReadEntries(const char* path) {
  Entries* entries = CreateEntries(path);

  DIR* dir = opendir(path);
  if (!dir) Fatal(path);

  for (;;) {
    struct dirent* dent;

    errno = 0;
    dent = readdir(dir);
    if (!dent) {
      if (errno == 0) break;  // end of directory
      Fatal(path);
    }
    if (isDotOrDotDot(dent->d_name)) continue;
    if (!IsDirOrFile(dent)) continue;

    if (strlen(dent->d_name) > HUMAN68K_FILENAME_MAX) {
      fprintf(stderr, "%s/%s: ファイル名が長すぎます。\n", path, dent->d_name);
      continue;
    }

    *(AllocateEntry(entries)) = CreateEntryByDirent(dent, path);
  }
  ShrinkEntryArray(entries);

  if (closedir(dir) != 0) Fatal(path);
  return entries;
}

#endif

static void ProcessDirectory(const char* fromPath, const char* toPath,
                             const Entry* entry) {
  char* newFromPath = ConcatPath(fromPath, entry->name);
  char* newToPath = ConcatPath(toPath, entry->name);

  Src2Build(newFromPath, newToPath);

  Free(newToPath);
  Free(newFromPath);
}

static int GetFileCtime(const char* path, NativeTime* outCtime) {
#ifdef USE_DOSCALL
  struct _filbuf filbuf;
  if (_dos_files(&filbuf, path, 0xff) < 0) return 0;
  *outCtime = (filbuf.date << 16) | filbuf.time;
#else
  struct stat st;
  if (stat(path, &st) != 0) return 0;
  *outCtime = st.st_ctime;
#endif

  return 1;
}

static char* FileRead(const char* path, size_t size, size_t* outSize) {
  char* buf = Malloc(size);

#ifdef USE_DOSCALL
  int readSize;
  int fd = _dos_open(path, OPENMODE_READ);
  if (fd < 0) DosFatal(path, fd);

  readSize = _dos_read(fd, buf, size);
  _dos_close(fd);
  if (readSize < 0) DosFatal(path, readSize);
  *outSize = readSize;
#else
  FILE* file = fopen(path, "rb");
  if (!file) Fatal(path);

  *outSize = fread(buf, 1, size, file);

  fclose(file);
#endif

  return buf;
}

static void FileWrite(const char* path, const char* buf, size_t size) {
#ifdef USE_DOSCALL
  int rc;
  int fd = _dos_create(path, _DOS_IFREG);
  if (fd < 0) DosFatal(path, fd);

  rc = _dos_write(fd, buf, size);
  _dos_close(fd);  // 容量不足時にファイルを削除するため、先にクローズしておく
  if (rc != size) {
    _dos_delete(path);
    DosFatal(path, (rc < 0) ? rc : _DOSE_DISKFULL);
  }
#else
  size_t writeSize;
  int err = 0;

  FILE* file = fopen(path, "wb");
  if (!file) Fatal(path);

  errno = 0;
  writeSize = fwrite(buf, 1, size, file);
  if (writeSize != size) {
    err = (ferror(file) && errno) ? errno : ENOSPC;
  }

  if (fclose(file) != 0) {
    if (err == 0) err = errno;
  }
  if (err) {
    // fwrite()かfclose()でエラーになったらファイルを削除してエラー終了する。
    // エラー表示は先に発生したエラーについて表示されるよう細工している。
    remove(path);
    errno = err;
    Fatal(path);
  }
#endif
}

static void ProcessFile(const char* fromPath, const char* toPath,
                        const Entry* entry) {
  char* fromFilePath = ConcatPath(fromPath, entry->name);
  char* toFilePath = ConcatPath(toPath, entry->name);

  NativeTime toFileCtime = (NativeTime)0;
  if (GetFileCtime(toFilePath, &toFileCtime)) {
    if (entry->ctime <= toFileCtime) {
      return;  // skip
    }
    printf("update %s\n", toFilePath);
  } else {
    printf("create %s\n", toFilePath);
  }

  {
    size_t size = 0;
    char* buf = FileRead(fromFilePath, entry->size, &size);

    size = Utf8toSjis(size, buf);
    FileWrite(toFilePath, buf, size);
    Free(buf);
  }
}

static void ProcessEntries(const Entries* entries, const char* toPath,
                           char isDir) {
  const size_t count = entries->count;
  const Entry* list = entries->list;
  size_t i;

  for (i = 0; i < count; i += 1) {
    const Entry* entry = &list[i];
    if (entry->isDirectory != isDir) continue;

    if (isDir) {
      ProcessDirectory(entries->path, toPath, entry);
    } else {
      ProcessFile(entries->path, toPath, entry);
    }
  }
}

static void Src2Build(const char* fromPath, const char* toPath) {
  Entries* entries = ReadEntries(fromPath);

  if (!DirExists(toPath)) {
    printf("mkdir %s\n", toPath);
    if (Mkdir(toPath) != 0) {
      if (errno != EEXIST) Fatal(toPath);
    }
  }

  ProcessEntries(entries, toPath, 1);  // 先にディレクトリを処理
  ProcessEntries(entries, toPath, 0);  // 次にファイルを処理
  FreeEntries(entries);
}

static CwdBuf GetCwd(void) {
  CwdBuf cwd = {{0}};

#ifdef USE_DOSCALL
  int rc;
  char* p = cwd.buf;
  *p++ = _dos_curdrv() + 'A';
  *p++ = ':';
  *p++ = '\\';
  rc = _dos_curdir(0, p);
  if (rc < 0) DosFatal(nullptr, rc);
  _toslash(cwd.buf);
#else
  if (!getcwd(cwd.buf, sizeof(cwd.buf))) Fatal(nullptr);
#endif

  return cwd;
}

static void Chdir(const char* path) {
#ifdef USE_DOSCALL
  int rc = _dos_chdir(path);
  if (rc < 0) DosFatal(path, rc);
#else
  if (chdir(path) != 0) Fatal(path);
#endif
}

#ifdef USE_DOSCALL
static void DosFatal(const char* message, int rc) {
  errno = _errcnv(rc);
  Fatal(message);
}
#endif

static void Fatal(const char* message) {
  perror(message);
  exit(EXIT_FAILURE);
}

static void Fatal2(const char* mes1, const char* mes2) {
  fprintf(stderr, "%s%s\n", mes1, mes2);
  exit(EXIT_FAILURE);
}
