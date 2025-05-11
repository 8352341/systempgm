# 시스템 프로그래밍 정리 노트

## 목차

- [파일 작업 기본](#file-basic-operations)
  - [파일 복사 (`copy.c`)](#file-copy)
  - [파일 디스크립터 복제 (`dup`, `dup2`)](#file-descriptor-duplication)
  - [파일 위치 포인터 (`lseek`)](#file-position-pointer)
- [레코드 기반 파일 입출력](#record-based-file-io)
  - [구조체 정의 (`student.h`)](#struct-definition)
  - [데이터베이스 생성 (`dbcreate.c`)](#db-create)
  - [데이터베이스 조회 (`dbquery.c`)](#db-query)
  - [데이터베이스 업데이트 (`dbupdate.c`)](#db-update)
  - [레코드 수정 과정](#record-modification-process)
- [파일 시스템 구조](#file-system-structure)
  - [i-노드 (i-Node)](#i-node)
  - [블록 포인터](#block-pointers) 
- [파일 입출력 구현을 위한 커널 자료구조](#kernel-data-structures)
- [파일 상태 (File Status) - `stat`](#file-status)
  - [`stat` 구조체](#stat-structure)
- [파일 사용 권한 (File Permissions)](#file-permissions)
  - [`st_mode` 필드 상세](#st-mode-field)
- [실습](#practice)

---

<a id="file-basic-operations"></a>
## 파일 작업 기본

<a id="file-copy"></a>
### 파일 복사 (`copy.c`)

- **목표**: 한 파일을 다른 파일로 복사합니다.
- **핵심 로직**:
  1. 원본 파일을 읽기 전용(`O_RDONLY`)으로 `open()`.
  2. 대상 파일을 쓰기 전용(`O_WRONLY`), 생성(`O_CREAT`), 내용 삭제(`O_TRUNC`) 모드로 `open()`. 권한은 0644.
  3. `read()`로 원본 파일에서 데이터를 읽어 버퍼에 저장.
  4. `write()`로 버퍼의 데이터를 대상 파일에 씀.
  5. 모든 데이터가 복사될 때까지 3-4 반복.
  6. `close()`로 두 파일 디스크립터를 닫음.
- **주요 헤더**: `<unistd.h>`, `<fcntl.h>`
- **오류 처리**: 오류 발생 시 `perror`로 메시지를 출력하고 `exit`합니다.

<a id="file-descriptor-duplication"></a>
### 파일 디스크립터 복제 (`dup`, `dup2`)

- **`dup(int oldfd)`**: `oldfd`를 복제하여 새 파일 디스크립터 반환. 새 디스크립터는 가장 작은 가용 번호. `oldfd`와 새 디스크립터는 **같은 파일 테이블 엔트리**(파일 오프셋, 상태 플래그 공유)를 가리킴.
- **`dup2(int oldfd, int newfd)`**: `oldfd`를 `newfd`로 복제. `newfd`가 이미 열려있으면 먼저 닫힘.
- **`dup.c` 예제**:
  ```c
  fd1 = creat("myfile", 0600);
  write(fd1, "Hello! Linux", 12);
  fd2 = dup(fd1); // fd1과 fd2는 같은 파일 오프셋을 공유
  write(fd2, "Bye! Linux", 10); // "myfile"에 이어 쓰기
  // 결과: "myfile" -> "Hello! LinuxBye! Linux"
  ```

<a id="file-position-pointer"></a>
### 파일 위치 포인터 (`lseek`)

- **파일 위치 포인터**: 파일 내 다음 I/O 작업 위치.
- **`lseek(int fd, off_t offset, int whence)`**: `fd`의 파일 오프셋 변경.
  - `whence`: `SEEK_SET`(시작 기준), `SEEK_CUR`(현재 기준), `SEEK_END`(끝 기준).
- **예시**:
  - `lseek(fd, 0L, SEEK_SET);` // 파일 시작으로 (rewind)
  - `lseek(fd, n * sizeof(record), SEEK_SET);` // n+1번째 레코드 시작으로

<a id="record-based-file-io"></a>
## 레코드 기반 파일 입출력

<a id="struct-definition"></a>
### 구조체 정의 (`student.h`)

```c
struct student { 
    char name[MAX_NAME_LEN]; 
    int id; 
    int score; 
};
#define START_ID 1401001 // 학번을 이용한 오프셋 계산용
```

<a id="db-create"></a>
### 데이터베이스 생성 (`dbcreate.c`)

- **파일 열기**: `open(..., O_WRONLY | O_CREAT | O_EXCL, 0640)` (쓰기, 생성, 존재 시 오류)
- **저장 로직**:
  1. 학생 정보 입력 (`scanf`).
  2. `lseek(fd, (record.id - START_ID) * sizeof(record), SEEK_SET);` // 학번으로 위치 계산
  3. `write(fd, &record, sizeof(record));` // 레코드 쓰기

<a id="db-query"></a>
### 데이터베이스 조회 (`dbquery.c`)

- **파일 열기**: `open(..., O_RDONLY)`
- **조회 로직**:
  1. 검색할 학번 입력.
  2. `lseek(fd, (id - START_ID) * sizeof(record), SEEK_SET);`
  3. `read(fd, &record, sizeof(record));` // 레코드 읽기
  4. 정보 출력.

<a id="db-update"></a>
### 데이터베이스 업데이트 (`dbupdate.c`)

- **파일 열기**: `open(..., O_RDWR)` (읽기/쓰기)
- **업데이트 로직**:
  1. 수정할 학번 입력.
  2. `lseek(...)`로 해당 레코드 위치 이동.
  3. `read(...)`로 현재 레코드 읽기 (선택적).
  4. 새 점수 입력.
  5. `lseek(fd, (long)-sizeof(record), SEEK_CUR);` // **중요**: `read` 후 포인터가 이동했으므로, 쓰기 위해 레코드 시작으로 복귀.
  6. `write(...)`로 수정된 레코드 쓰기.

<a id="record-modification-process"></a>
### 레코드 수정 과정

1. **읽기 (READ)**: 수정할 레코드 읽기.
2. **정보 수정**: 메모리에서 레코드 정보 변경.
3. **위치 이동 및 쓰기 (LSEEK + WRITE)**: 파일 내 원래 위치로 이동 후 수정된 레코드 덮어쓰기.

<a id="file-system-structure"></a>
## 파일 시스템 구조

- **부트 블록**: 시스템 부팅 코드.
- **슈퍼 블록**: 파일 시스템 전체 정보 (블록 수, i-노드 수 등).
- **i-리스트 (Inode list)**: 모든 i-노드 저장 영역.
- **데이터 블록**: 실제 파일 내용 저장.

<a id="i-node"></a>
### i-노드 (i-Node)

- **정의**: 파일의 메타데이터 저장 (파일 이름 제외). 파일당 1개.
- **내용**: 파일 타입, 크기, 권한, 소유자/그룹, 타임스탬프, **데이터 블록 포인터**, 링크 수.

<a id="block-pointers"></a>
### 블록 포인터

- i-노드 내 데이터 블록 주소 목록.
- **직접 블록**: 데이터 블록 직접 가리킴.
- **간접 블록**: 데이터 블록 주소를 담은 블록을 가리킴 (단일, 이중, 삼중 간접).

<a id="kernel-data-structures"></a>
## 파일 입출력 구현을 위한 커널 자료구조

- **1. 파일 디스크립터 배열 (Per-process)**: 인덱스=fd, 값=열린 파일 테이블 엔트리 포인터.
- **2. 열린 파일 테이블 (System-wide)**: `open()`마다 생성 (대부분). 파일 상태 플래그, **현재 파일 오프셋**, i-노드 테이블 포인터, 참조 카운트.
- **3. 동적 i-노드 테이블 (System-wide)**: 메모리 내 i-노드 캐시. 디스크 i-노드 정보 포함.

<a id="file-status"></a>
## 파일 상태 (File Status) - `stat`

- **파일 상태**: 파일의 모든 정보. `ls -l`로 확인 가능.
- **시스템 호출**: `stat()`, `fstat()`, `lstat()` (심볼릭 링크 자체). `<sys/stat.h>`
  - `int stat(const char *filename, struct stat *buf);`

<a id="stat-structure"></a>
### `stat` 구조체

```c
struct stat {
    mode_t    st_mode;    // 파일 타입과 권한
    ino_t     st_ino;     // i-노드 번호
    // ... (st_uid, st_gid, st_size, st_atime, st_mtime, st_ctime 등)
};
```

<a id="file-permissions"></a>
## 파일 사용 권한 (File Permissions)

- **파일 권한**: 소유자, 그룹, 기타 사용자별 읽기(r), 쓰기(w), 실행(x) 권한
- **권한 비트**: 9개 비트로 표현 (rwx rwx rwx)
- **권한 검사**: 파일 접근 시 프로세스의 UID/GID와 파일의 권한 비교
- **수정 명령**: `chmod` (파일 모드 변경), `chown` (소유자 변경), `chgrp` (그룹 변경)

<a id="st-mode-field"></a>
### `st_mode` 필드 상세

- **파일 타입**: S_ISREG, S_ISDIR, S_ISCHR, S_ISBLK, S_ISFIFO, S_ISLNK, S_ISSOCK 매크로로 검사
- **접근 권한 비트**: S_IRUSR, S_IWUSR, S_IXUSR, S_IRGRP, S_IWGRP, S_IXGRP, S_IROTH, S_IWOTH, S_IXOTH
- **특수 비트**: S_ISUID(SetUID), S_ISGID(SetGID), S_ISVTX(스티키 비트)

---

<a id="practice"></a>
## 실습
  -ls –l 명령어처럼 파일의 모든 상태 정보를 프린트하는 list2.c파일을 생성

  ```c
  #include <stdio.h>
  #include <stdlib.h>
  #include <sys/stat.h>
  #include <sys/types.h>
  #include <unistd.h>
  #include <dirent.h>     // 디렉토리 처리를 위해
  #include <string.h>
  #include <pwd.h>        // 사용자 이름 출력을 위해 (getpwuid)
  #include <grp.h>        // 그룹 이름 출력을 위해 (getgrgid)
  #include <time.h>       // 시간 정보 포맷팅을 위해 (strftime, localtime)
  #include <errno.h>      // 오류 처리를 위해 (perror)
  
  // 함수 프로토타입 선언
  void printStat(const char *pathname, const char *filename, const struct stat *fileStat);
  char type(mode_t mode);
  void perm(mode_t mode, char *permStr);
  void list_directory_contents(const char *dirname);
  
  /**
   * @brief 메인 프로그램: 명령줄 인자로 주어진 파일 또는 디렉토리의 정보를 출력합니다.
   * 인자가 없으면 현재 디렉토리의 내용을 출력합니다.
   */
  int main(int argc, char *argv[]) {
      struct stat fileStat;
  
      if (argc == 1) {
          // 인자가 없으면 현재 디렉토리(".")의 내용을 나열
          list_directory_contents(".");
      } else {
          for (int i = 1; i < argc; i++) {
              if (lstat(argv[i], &fileStat) == -1) { // lstat: 심볼릭 링크 자체의 정보를 가져옴
                  perror(argv[i]); // 파일/디렉토리 접근 오류 시 메시지 출력
                  continue;
              }
  
              // 인자가 디렉토리인지 확인
              if (S_ISDIR(fileStat.st_mode)) {
                  if (argc > 2 && i > 1) { // 여러 인자가 주어지고, 이전 인자도 디렉토리였다면 줄바꿈 추가
                      printf("\n");
                  }
                  printf("%s:\n", argv[i]);
                  list_directory_contents(argv[i]);
              } else {
                  // 파일 또는 심볼릭 링크 등
                  printStat(argv[i], argv[i], &fileStat);
              }
          }
      }
      return 0;
  }
  
  /**
   * @brief 지정된 디렉토리의 내용물을 `ls -l` 형식으로 출력합니다.
   * @param dirname 디렉토리 경로
   */
  void list_directory_contents(const char *dirname) {
      DIR *dp;
      struct dirent *dentry;
      struct stat fileStat;
      char fullpath[1024]; // 경로를 포함한 파일 이름
  
      if ((dp = opendir(dirname)) == NULL) {
          perror(dirname);
          return;
      }
  
      while ((dentry = readdir(dp)) != NULL) {
          // "." 이나 ".." 은 일반적으로 ls -l 시 상세정보를 출력하지 않으므로,
          // 여기서는 모든 항목을 보여주도록 합니다. (필요시 주석 처리)
          // if (strcmp(dentry->d_name, ".") == 0 || strcmp(dentry->d_name, "..") == 0) {
          //     continue;
          // }
  
          // 전체 경로 생성
          if (strcmp(dirname, "/") == 0) { // 루트 디렉토리인 경우
               snprintf(fullpath, sizeof(fullpath), "/%s", dentry->d_name);
          } else {
              snprintf(fullpath, sizeof(fullpath), "%s/%s", dirname, dentry->d_name);
          }
  
  
          if (lstat(fullpath, &fileStat) == -1) {
              perror(fullpath);
              continue;
          }
          printStat(fullpath, dentry->d_name, &fileStat); // 파일 이름만 전달 (경로 제외)
      }
      closedir(dp);
  }
  
  
  /**
   * @brief 파일 상태 정보를 `ls -l` 형식으로 출력합니다.
   * @param pathname 파일의 전체 경로 (심볼릭 링크 대상 확인용)
   * @param filename 출력할 파일 이름
   * @param fileStat 파일 상태 정보를 담고 있는 stat 구조체 포인터
   */
  void printStat(const char *pathname, const char *filename, const struct stat *fileStat) {
      char typeChar = type(fileStat->st_mode);
      char permStr[11]; // "rwxrwxrwx\0" + S_ISUID 등 고려
      perm(fileStat->st_mode, permStr);
  
      // 사용자 이름 가져오기
      struct passwd *pw = getpwuid(fileStat->st_uid);
      const char *userName = (pw != NULL) ? pw->pw_name : "unknown";
  
      // 그룹 이름 가져오기
      struct group *gr = getgrgid(fileStat->st_gid);
      const char *groupName = (gr != NULL) ? gr->gr_name : "unknown";
  
      // 시간 정보 포맷팅
      char timeStr[80];
      time_t now = time(NULL);
      struct tm *mod_tm = localtime(&(fileStat->st_mtime));
      struct tm *now_tm = localtime(&now);
  
      // 파일 수정 시간이 현재 연도와 같으면 "월 일 시간", 다르면 "월 일 연도" 형식
      if (mod_tm->tm_year == now_tm->tm_year) {
          strftime(timeStr, sizeof(timeStr), "%b %e %H:%M", mod_tm); // 예: May  7 15:30
      } else {
          strftime(timeStr, sizeof(timeStr), "%b %e  %Y", mod_tm); // 예: Dec 25  2023
      }
  
  
      printf("%c%s %2ld %-8s %-8s %8lld %s %s",
             typeChar,
             permStr,
             fileStat->st_nlink,  // 하드 링크 수
             userName,            // 소유자 이름
             groupName,           // 그룹 이름
             (long long)fileStat->st_size, // 파일 크기 (lld로 long long 캐스팅)
             timeStr,             // 최종 수정 시간
             filename);           // 파일 이름
  
      // 심볼릭 링크인 경우, 가리키는 대상 출력
      if (typeChar == 'l') {
          char linkTarget[256];
          ssize_t len = readlink(pathname, linkTarget, sizeof(linkTarget) - 1);
          if (len != -1) {
              linkTarget[len] = '\0';
              printf(" -> %s", linkTarget);
          }
      }
      printf("\n");
  }
  
  /**
   * @brief 파일 타입을 나타내는 문자를 반환합니다.
   * @param mode 파일의 st_mode 값
   * @return 파일 타입 문자 ('-', 'd', 'l', 'c', 'b', 'p', 's')
   */
  char type(mode_t mode) {
      if (S_ISREG(mode)) return '-';  // 일반 파일
      if (S_ISDIR(mode)) return 'd';  // 디렉토리
      if (S_ISLNK(mode)) return 'l';  // 심볼릭 링크
      if (S_ISCHR(mode)) return 'c';  // 문자 장치 파일
      if (S_ISBLK(mode)) return 'b';  // 블록 장치 파일
      if (S_ISFIFO(mode)) return 'p'; // FIFO (파이프)
      if (S_ISSOCK(mode)) return 's'; // 소켓
      return '?'; // 알 수 없는 타입
  }
  
  /**
   * @brief 파일 사용 권한을 "rwxrwxrwx" 형식의 문자열로 변환합니다.
   * @param mode 파일의 st_mode 값
   * @param permStr 권한 문자열을 저장할 버퍼 (최소 10바이트 크기)
   */
  void perm(mode_t mode, char *permStr) {
      strcpy(permStr, "---------"); // 기본값으로 초기화
  
      // 사용자(Owner) 권한
      if (mode & S_IRUSR) permStr[0] = 'r';
      if (mode & S_IWUSR) permStr[1] = 'w';
      if (mode & S_IXUSR) permStr[2] = 'x';
  
      // 그룹(Group) 권한
      if (mode & S_IRGRP) permStr[3] = 'r';
      if (mode & S_IWGRP) permStr[4] = 'w';
      if (mode & S_IXGRP) permStr[5] = 'x';
  
      // 기타(Others) 권한
      if (mode & S_IROTH) permStr[6] = 'r';
      if (mode & S_IWOTH) permStr[7] = 'w';
      if (mode & S_IXOTH) permStr[8] = 'x';
  
      // 특수 권한 (SetUID, SetGID, Sticky bit)
      if (mode & S_ISUID) permStr[2] = (permStr[2] == 'x') ? 's' : 'S';
      if (mode & S_ISGID) permStr[5] = (permStr[5] == 'x') ? 's' : 'S'; // ls에서는 's' 또는 'l'로 표시되기도 함
      if (mode & S_ISVTX) permStr[8] = (permStr[8] == 'x') ? 't' : 'T';
  
      permStr[9] = '\0'; // 문자열 종료
  }

  ```



