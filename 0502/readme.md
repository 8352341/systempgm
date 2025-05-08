# 리눅스 시스템 프로그래밍: 파일 (File)

## 목차
- [시스템 프로그래밍 및 컴퓨터 시스템 구조](#system-programming)
- [유닉스에서의 파일](#unix-files)
- [파일 기본 연산](#file-operations)
- [파일 디스크립터 복제](#file-descriptor-duplication)
- [파일 위치 포인터](#file-position-pointer)
- [예제 프로그램: 학생 정보 관리](#student-management)
- [핵심 개념 요약](#key-concepts)

## <a id="system-programming"></a>1. 시스템 프로그래밍 및 컴퓨터 시스템 구조

### 1.1. 유닉스 커널(Kernel)

하드웨어를 운영 관리하며 다음과 같은 서비스를 제공합니다.

- 파일 관리 (File management)
- 프로세스 관리 (Process management)
- 메모리 관리 (Memory management)
- 통신 관리 (Communication management)
- 주변장치 관리 (Device management)

구조: 응용 프로그램 ↔ (시스템 호출) ↔ 커널 ↔ (CPU, 메모리, 디스크, 주변장치)

### 1.2. 시스템 호출(System Call)

커널에 서비스 요청을 위한 프로그래밍 인터페이스입니다.

- 응용 프로그램은 시스템 호출을 통해서 커널에 서비스를 요청합니다.
- 흐름: 응용 프로그램 → C 라이브러리 함수 → 시스템 호출 → 커널 → 하드웨어

### 1.3. 시스템 호출 과정

1. 사용자 프로세스에서 C 라이브러리 함수 호출 (예: `fd = open(argv[1], O_RDONLY);`)
2. C 라이브러리 함수 내부 (예: `open(char *name, int mode)`)
3. 레지스터로 매개변수 전달
4. trap 명령어 실행, 커널 코드로 점프
5. 커널
   - 커널 내의 해당 시스템 호출 코드 (예: `open()`) 실행
   - 커널 데이터 조작
6. 사용자 코드로 리턴 (시스템 호출 결과 리턴)

### 1.4. 주요 시스템 호출 요약

| 주요 자원 | 시스템 호출 |
|----------|------------|
| 파일 | `open()`, `close()`, `read()`, `write()`, `dup()`, `lseek()` 등 |
| 프로세스 | `fork()`, `exec()`, `exit()`, `wait()`, `getpid()`, `getppid()` 등 |
| 메모리* | `malloc()`, `calloc()`, `free()` 등 (라이브러리 함수, 내부적으로 `brk`, `sbrk` 등 시스템 호출 사용) |
| 시그널 | `signal()`, `alarm()`, `kill()`, `sleep()` 등 |
| 프로세스 간 통신 | `pipe()`, `socket()` 등 |

## <a id="unix-files"></a>2. 유닉스에서의 파일

- 연속된 바이트의 나열입니다.
- 특별한 다른 포맷을 정하지 않습니다.
- 디스크 파일뿐만 아니라 외부 장치에 대한 인터페이스 역할도 합니다. (예: 0 1 2 ... n-1 [파일끝 표시])

## <a id="file-operations"></a>3. 파일 기본 연산

### 3.1. 파일 열기: open()

파일을 사용하기 위해 먼저 `open()` 시스템 호출로 파일을 열어야 합니다.

- 헤더: `<sys/types.h>`, `<sys/stat.h>`, `<fcntl.h>`
- 함수 원형: `int open(const char *path, int oflag, [ mode_t mode ]);`
- 반환 값: 성공 시 파일 디스크립터(양의 정수), 실패 시 -1.
- 파일 디스크립터: 열린 파일을 나타내는 번호.

oflag 옵션:
- `O_RDONLY`: 읽기 전용
- `O_WRONLY`: 쓰기 전용
- `O_RDWR`: 읽기/쓰기
- `O_APPEND`: 파일 끝에 추가
- `O_CREAT`: 파일이 없으면 생성 (이때 mode로 권한 지정)
- `O_TRUNC`: 파일이 있으면 내용 삭제
- `O_EXCL`: `O_CREAT`와 함께 사용, 파일이 이미 있으면 오류
- `O_NONBLOCK`: 넌블로킹 모드
- `O_SYNC`: `write()` 호출 시 물리적 디스크 쓰기 완료 후 반환

### 3.2. 파일 생성: creat()

- path가 나타내는 파일을 생성하고 쓰기 전용으로 엽니다.
- 생성된 파일의 사용 권한은 mode로 정합니다.
- 기존 파일이 있는 경우 내용을 삭제하고 엽니다.
- `open(path, WRONLY | O_CREAT | O_TRUNC, mode);` 와 동일합니다.
- 헤더: `<sys/types.h>`, `<sys/stat.h>`, `<fcntl.h>`
- 함수 원형: `int creat(const char *path, mode_t mode);`
- 반환 값: 성공 시 파일 디스크립터, 실패 시 -1.

### 3.3. 파일 닫기: close()

- fd가 나타내는 파일을 닫습니다.
- 헤더: `<unistd.h>`
- 함수 원형: `int close(int fd);`
- 반환 값: 성공 시 0, 실패 시 -1.

### 3.4. 데이터 읽기: read()

- fd가 나타내는 파일에서 nbytes 만큼의 데이터를 읽어 buf에 저장합니다.
- 헤더: `<unistd.h>`
- 함수 원형: `ssize_t read(int fd, void *buf, size_t nbytes);`
- 반환 값: 성공 시 읽은 바이트 수, 파일 끝(EOF)이면 0, 실패 시 -1.

### 3.5. 데이터 쓰기: write()

- buf에 있는 nbytes 만큼의 데이터를 fd가 나타내는 파일에 씁니다.
- 헤더: `<unistd.h>`
- 함수 원형: `ssize_t write(int fd, void *buf, size_t nbytes);`
- 반환 값: 성공 시 실제 쓰여진 바이트 수, 실패 시 -1.

## <a id="file-descriptor-duplication"></a>4. 파일 디스크립터 복제

### 4.1. dup() 및 dup2()

기존의 파일 디스크립터를 복제합니다.

- oldfd와 복제된 새로운 디스크립터는 하나의 파일을 공유합니다 (파일 위치 포인터 등 공유).
- 헤더: `<unistd.h>`
- `int dup(int oldfd);`: oldfd의 복제본인 새 파일 디스크립터 생성 후 반환. 실패 시 -1.
- `int dup2(int oldfd, int newfd);`: oldfd를 newfd에 복제. newfd가 이미 열려있으면 닫은 후 복제. 성공 시 newfd, 실패 시 -1.

예시 (dup.c): `creat()`로 파일 생성 및 "Hello! Linux" 쓰기 → `dup()`로 fd 복제 → 복제된 fd로 "Bye! Linux" 쓰기. 결과적으로 파일에는 "Hello! LinuxBye! Linux"가 쓰임 (파일 위치 포인터 공유).

## <a id="file-position-pointer"></a>5. 파일 위치 포인터 (File Position Pointer)

파일 내에 읽거나 쓸 위치인 현재 파일 위치(current file position)를 가리킵니다.

### 5.1. 파일 위치 포인터 이동: lseek()

임의의 위치로 파일 위치 포인터를 이동시킬 수 있습니다.

- 헤더: `<unistd.h>`
- 함수 원형: `off_t lseek(int fd, off_t offset, int whence);`
- 반환 값: 성공 시 파일 시작으로부터의 현재 위치, 실패 시 -1.

whence 옵션:
- `SEEK_SET`: 파일 시작(offset 바이트 위치)
- `SEEK_CUR`: 현재 위치(offset 바이트만큼 이동)
- `SEEK_END`: 파일 끝(offset 바이트만큼 이동)

예시:
- `lseek(fd, 0L, SEEK_SET);`: 파일 시작으로 이동 (rewind)
- `lseek(fd, 100L, SEEK_SET);`: 파일 시작에서 100바이트 위치로
- `lseek(fd, 0L, SEEK_END);`: 파일 끝으로 이동 (append 준비)
- `lseek(fd, n * sizeof(record), SEEK_SET);`: n+1번째 레코드 시작 위치로
- `lseek(fd, sizeof(record), SEEK_CUR);`: 다음 레코드 시작 위치로

## <a id="student-management"></a>6. 예제 프로그램: 학생 정보 관리

### 6.1. student.h

```c
#define MAX 24
#define START_ID 1401001

struct student {
    char name[MAX];
    int id;
    int score;
};
```

### 6.2. dbcreate.c (데이터베이스 생성)

학생 정보를 입력받아 데이터베이스 파일에 저장합니다.

- `open(argv[1], O_WRONLY | O_CREAT | O_EXCL, 0640)`: 파일 생성 (이미 존재하면 에러)
- 학번, 이름, 점수를 입력받습니다.
- `lseek(fd, (record.id - START_ID) * sizeof(record), SEEK_SET);`: 학번을 기준으로 파일 내 위치 계산 후 이동.
- `write(fd, (char *) &record, sizeof(record));`: 레코드 정보를 파일에 씁니다.

### 6.3. dbquery.c (데이터베이스 조회)

학번을 입력받아 해당 학생의 레코드를 파일에서 읽어 출력합니다.

- `open(argv[1], O_RDONLY)`: 파일을 읽기 전용으로 엽니다.
- 조회할 학번을 입력받습니다.
- `lseek(fd, (id - START_ID) * sizeof(record), SEEK_SET);`: 학번 기준으로 위치 이동.
- `read(fd, (char *) &record, sizeof(record))`: 레코드 정보를 읽습니다.
- 읽은 정보가 유효하면(ID 일치) 학생 정보 출력.

### 6.4. dbupdate.c (데이터베이스 수정)

학번을 입력받아 해당 학생 레코드의 점수를 수정합니다.

- `open(argv[1], O_RDWR)`: 파일을 읽기/쓰기 모드로 엽니다.
- 수정할 학생의 학번을 입력받습니다.
- `lseek(fd, (long)(id - START_ID) * sizeof(record), SEEK_SET);`: 해당 레코드 위치로 이동.
- `read()`: 기존 레코드 정보를 읽어 현재 점수 등을 보여줍니다.
- 새로운 점수를 입력받습니다.
- `lseek(fd, (long) -sizeof(record), SEEK_CUR);`: 현재 위치에서 레코드 크기만큼 뒤로 이동 (읽었던 레코드의 시작 위치로 돌아감).
- `write()`: 수정된 레코드 정보를 덮어씁니다.

### 6.5. 레코드 수정 과정

1. 파일로부터 해당 레코드를 read 한다.
2. 이 레코드(메모리상의 복사본)를 수정한 후에,
3. 수정된 레코드를 다시 파일 내의 원래 위치에 write 해야 한다. (이때 lseek으로 원래 위치로 돌아간 후 write)

## <a id="key-concepts"></a>7. 핵심 개념 요약

- 시스템 호출: 커널에 서비스를 요청하기 위한 프로그래밍 인터페이스로, 응용 프로그램은 시스템 호출을 통해서 커널에 서비스를 요청할 수 있다.
- 파일 디스크립터: 열린 파일을 나타낸다.
- `open()`: 파일을 열고 열린 파일의 파일 디스크립터를 반환한다.
- `read()`/`write()`: 지정된 파일에서 원하는 만큼의 데이터를 읽거나 쓴다.
- 파일 위치 포인터: 파일 내에 읽거나 쓸 위치인 현재 파일 위치를 가리킨다.
- `lseek()`: 지정된 파일의 현재 파일 위치를 원하는 위치로 이동시킨다.

