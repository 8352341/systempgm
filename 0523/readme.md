# 9장 프로세스 제어 요약

## 9.1 프로세스 생성

- 부모 프로세스가 fork() 시스템 호출을 사용해 자식 프로세스를 생성합니다.  
- fork()는 부모의 복제된 형태로 자식 프로세스를 만들어내며,  
  - 부모는 자식의 프로세스 ID를 반환받고,  
  - 자식은 0을 반환받습니다.  
- 이 반환 값을 이용하여 부모와 자식이 서로 다른 작업을 수행할 수 있으며,  
  두 프로세스는 동시에 독립적으로 실행됩니다.

## 프로세스 기다리기

- 부모 프로세스는 자식 프로세스의 종료를 기다리기 위해 wait() 또는 waitpid()를 사용합니다.
- wait()는 자식 프로세스 중 하나가 종료될 때까지 대기하고, 종료된 자식의 프로세스 ID와 종료 상태를 확인합니다.
- waitpid()는 특정 자식 프로세스의 종료를 선택적으로 기다릴 수 있도록 해주며, 이를 이용해 더 세밀한 제어가 가능합니다.

# 실습 정리: 프로세스 제어 실습

---

## fork1.c: 자식 생성 후 PID 출력

**설명:**  
부모 프로세스가 `fork()`를 이용해 자식 프로세스를 생성하고, 생성된 자식 프로세스의 PID를 출력합니다.

**코드:**

```c
#include <stdio.h>
#include <unistd.h>

int main() {
    int pid;
    printf("[%d] 프로세스 시작\n", getpid());
    pid = fork();
    printf("[%d] 프로세스 : 리턴값 %d\n", getpid(), pid);
    printf("[%d] 프로세스 : 리턴값 %d\n", getpid(), pid);
    printf("[%d] 프로세스 : 리턴값 %d\n", getpid(), pid);
    return 0;
}
```

**실행 결과:**

<!-- fork1.c 실행 결과 사진을 여기에 삽입 -->

---

## fork2.c: 부모/자식이 다른 메시지를 출력

**설명:**  
부모와 자식 프로세스가 `fork()` 호출 후 각각 다른 메시지를 출력합니다.

**코드:**

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    int pid;
    pid = fork();
    if (pid == 0) {
        // 자식 프로세스
        printf("[Child] : Hello, world! pid=%d\n", getpid());
    } else {
        // 부모 프로세스
        printf("[Parent] : Hello, world! pid=%d\n", getpid());
    }
    return 0;
}
```

**실행 결과:**

<!-- fork2.c 실행 결과 사진을 여기에 삽입 -->

---

## fork3.c: 두 자식 프로세스 생성

**설명:**  
하나의 부모 프로세스에서 두 개의 자식 프로세스를 생성하고, 각 자식이 메시지를 출력합니다.

**코드:**

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main() {
    int pid1, pid2;
    
    pid1 = fork();
    if (pid1 == 0) {
        printf("[Child 1] : Hello, world! pid=%d\n", getpid());
        exit(0);
    }
    
    pid2 = fork();
    if (pid2 == 0) {
        printf("[Child 2] : Hello, world! pid=%d\n", getpid());
        exit(0);
    }
    
    printf("[Parent] : Hello, world! pid=%d\n", getpid());
    return 0;
}
```

**실행 결과:**

<!-- fork3.c 실행 결과 사진을 여기에 삽입 -->

---

## forkwait.c: 프로세스 기다리기

**설명:**  
부모 프로세스가 자식 프로세스의 종료를 `wait()` 함수를 사용하여 기다립니다.

**코드:**

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    int pid, child, status;
    printf("[%d] 부모 프로세스 시작\n", getpid());
    pid = fork();
    if (pid == 0) {
        printf("[%d] 자식 프로세스 시작\n", getpid());
        exit(1);
    }
    child = wait(&status);  // 자식 프로세스가 종료될 때까지 기다림
    printf("[%d] 자식 프로세스 %d 종료\n", getpid(), child);
    printf("종료 코드 %d\n", status >> 8);
    return 0;
}
```

**실행 결과:**

<!-- forkwait.c 실행 결과 사진을 여기에 삽입 -->

---

## waitpid.c: 특정 자식 프로세스 기다리기

**설명:**  
부모 프로세스가 특정 자식 프로세스의 종료를 `waitpid()` 함수를 사용하여 기다립니다.

**코드:**

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    int pid1, pid2, child, status;
    printf("[%d] 부모 프로세스 시작\n", getpid());
    
    pid1 = fork();
    if (pid1 == 0) {
        printf("[%d] 자식 프로세스[1] 시작\n", getpid());
        sleep(1);
        printf("[%d] 자식 프로세스[1] 종료\n", getpid());
        exit(1);
    }
    
    pid2 = fork();
    if (pid2 == 0) {
        printf("[%d] 자식 프로세스[2] 시작\n", getpid());
        sleep(2);
        printf("[%d] 자식 프로세스[2] 종료\n", getpid());
        exit(2);
    }
    
    // 자식 프로세스 #1의 종료를 기다림.
    child = waitpid(pid1, &status, 0);
    printf("[%d] 자식 프로세스 #1 %d 종료\n", getpid(), child);
    printf("종료 코드 %d\n", status >> 8);
    
    return 0;
}
```

**실행 결과:**

<!-- waitpid.c 실행 결과 사진을 여기에 삽입 -->
