# 운영체제 과제 최종 정리본

## 목차
- [과제 개요 및 평가](#overview)
- [7장 파일 및 레코드 잠금](#chap7)
- [8장 프로세스](#chap8)
- [9장 프로세스 제어 (프로세스 생성)](#chap9)

---

## 1. 과제 개요 및 평가 <a id="overview"></a>
- **명령어 구현**:  
  - 운영체제에서 사용하는 약 50개 명령어를 C언어와 시스템 호출을 이용해 직접 구현하고 설명  
  - 구현 시, **주석을 상세히 달아** 각 코드의 동작을 명확하게 설명할 것
- **평가 점수**:  
  - 깃허브 레포지토리: 15점  
  - 레포트: 15점
- **제출 방식**:  
  - GitHub 레포지토리에 정리 (프로젝트 설명 및 사용법은 `README.md` 파일에 기록)  
  - 발표용 PPT 제출 (5월 30일까지 준비; 최종 제출 6월 13일)
- **추가 팁**:  
  - `getopt` 및 `getopt_long`을 활용해 원하는 옵션 선택 구현  
  - GPT와 같은 도구로 기본 코드 생성은 가능하나, **코드를 완벽히 이해하고 설명**할 수 있어야 함
  - 교재 및 기존 소스 코드 활용 가능  
  - 예시 구현: `ls` 명령어, `touch` 명령어 (`touch.c`), 파일 정보 출력 예제 (`list1.c`), `chmod`, `fchmod`, `utime` 등  
  - 파일 타입 및 접근권한은 `stat` 구조체의 `st_mode`를 활용하여 구현

---

## 4. 7장 파일 및 레코드 잠금 <a id="chap7"></a>

### 파일 잠금
- **주요 함수**: `flock()`  
  - **옵션**:
    - `LOCK_SH`: 공유 잠금 (여러 프로세스가 동시에 읽기 가능)
    - `LOCK_EX`: 배타 잠금 (한 프로세스만 쓰기 가능)
    - `LOCK_UN`: 잠금 해제
    - `LOCK_NB`: 비차단 모드 (잠금 시도 시 대기 없음)
- **동작 특징**:
  - 잠금이 없으면 모든 요청 승인  
  - 이미 공유 잠금 중이면 추가 공유 잠금은 승인, 배타 요청은 거절  
  - 배타 잠금이 걸려 있으면 모든 잠금 요청이 거절됨

### 레코드 잠금
- **주요 함수**: `fcntl()`  
  - **잠금 종류**:
    - `F_RDLCK`: 읽기 잠금 (공유 가능)
    - `F_WRLCK`: 쓰기 잠금 (배타적)
    - `F_UNLCK`: 잠금 해제
  - **옵션**:
    - 잠금 검사: `F_GETLK`
    - 잠금 설정: `F_SETLK` (비차단) 또는 `F_SETLKW` (차단)

### 간단한 잠금 함수
- **함수**: `lockf()`  
  - `fcntl()`의 단순화 버전으로, 현재 파일 위치부터 지정한 길이만큼 잠금 설정

### 잠금 방식
- **권고 잠금 (Advisory Locking)**:  
  - 커널이 강제하지 않음. 프로세스가 자율적으로 잠금 규칙 준수
- **강제 잠금 (Mandatory Locking)**:  
  - 커널이 잠금 규칙을 강제함. 파일 권한 비트 변경 등 추가 설정 필요

---

## 5. 8장 프로세스 <a id="chap8"></a>

### 쉘과 프로세스
- **쉘 (Shell)**:  
  - 사용자와 OS 간 인터페이스, 명령어 해석 및 실행  
  - **전면 처리**(명령어 실행 종료까지 기다림)와 **후면 처리**(백그라운드 실행) 지원
- **프로세스**:  
  - 실행 중인 프로그램, 고유 PID를 가짐  
  - `ps`, `kill`, `sleep`, `wait` 등 다양한 명령어로 관리

### 프로그램 실행
- **실행 방식**:  
  - `exec` 계열 함수로 현재 프로세스를 새로운 프로그램 이미지로 대체  
  - `system()` 함수를 통해 쉘 명령어 실행
- **명령줄 인수 및 환경 변수**:  
  - `main(int argc, char *argv[])`로 인수 전달  
  - 환경 변수는 `getenv`, `setenv`, `putenv`, `unsetenv` 등으로 관리

### 프로그램 종료 및 종료 처리
- **종료 방식**:  
  - 정상 종료: `exit()`, `main()` 함수에서 반환 후 `atexit()`에 등록된 처리기 호출  
  - 비정상 종료: `abort()`, `_exit()`

### 프로세스 ID 및 사용자/그룹 ID
- **식별 함수**:  
  - 프로세스: `getpid()`, `getppid()`  
  - 사용자/그룹: `getuid()`, `geteuid()`, `getgid()`, `getegid()`
- **특별 권한**:  
  - SUID/SGID 비트 설정 시, 실행 중 프로세스의 효력 사용자/그룹 ID 변경 (예: `chmod 4755 file`)

### 프로세스 이미지 구성
- **메모리 분할**:  
  - 텍스트(실행 코드), 데이터(전역/정적 변수), 힙(동적 할당), 스택(함수 호출 시) 등

---

## 6. 9장 프로세스 제어 (프로세스 생성) <a id="chap9"></a>
- **프로세스 생성**:  
  - **핵심 함수**: `fork()`  
    - 부모 프로세스가 자식 프로세스를 생성  
    - **반환 값**:
      - 부모: 생성된 자식의 PID 반환  
      - 자식: 0 반환  
      - 오류 시: -1 반환
- *(그 외의 프로세스 제어나 I/O 재지정 등 세부내용은 개별 구현에 따라 달라짐)*
