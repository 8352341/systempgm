#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utmp.h>       // utmp 구조체 및 관련 함수
#include <unistd.h>     // getopt, ttyname
#include <time.h>       // ctime, strftime
#include <sys/stat.h>   // stat (터미널 상태 확인용)

// 함수 선언
void print_entry(struct utmp *entry, int show_term_status);
void print_quick_users(char *users[], int count);

// 메인 함수
int main(int argc, char *argv[]) {
    // 옵션 상태를 저장할 플래그 변수
    int show_all = 0, show_boot_time = 0, show_current_term = 0;
    int show_term_status = 0, show_quick = 0;

    int opt;
    // getopt를 사용하여 명령줄 옵션을 파싱. "-a -b -m -T -q"
    while ((opt = getopt(argc, argv, "abmTq")) != -1) {
        switch (opt) {
            case 'a': show_all = 1;         break;
            case 'b': show_boot_time = 1;   break;
            case 'm': show_current_term = 1;break;
            case 'T': show_term_status = 1; break;
            case 'q': show_quick = 1;       break;
            default:
                fprintf(stderr, "사용법: %s [-abmTq]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // --- 옵션별 로직 처리 ---

    // -q (quick) 옵션은 다른 출력 없이 사용자 목록과 수만 보여주고 종료
    if (show_quick) {
        struct utmp *entry;
        char *users[1024]; // 사용자 이름을 저장할 배열
        int count = 0;

        setutent(); // utmp 파일 열기
        while ((entry = getutent()) != NULL) {
            if (entry->ut_type == USER_PROCESS && entry->ut_user[0] != '\0') {
                users[count++] = strdup(entry->ut_user);
                if (count >= 1024) break; // 배열 오버플로우 방지
            }
        }
        endutent(); // utmp 파일 닫기

        print_quick_users(users, count); // 퀵 포맷으로 출력
        // 메모리 해제
        for (int i = 0; i < count; i++) {
            free(users[i]);
        }
        return 0; // 프로그램 종료
    }
    
    // -m (who am i) 옵션을 위한 현재 터미널 이름 가져오기
    char *my_tty = NULL;
    if (show_current_term) {
        my_tty = ttyname(STDIN_FILENO); // 현재 표준 입력의 터미널 장치 파일 경로
        if (my_tty) {
            my_tty += 5; // "/dev/" 부분 건너뛰기
        }
    }
    
    struct utmp *entry;
    setutent(); // utmp 파일 열기

    // utmp 파일을 순회하며 각 레코드를 처리
    while ((entry = getutent()) != NULL) {
        int record_type = entry->ut_type;

        // -a 옵션 처리
        if (show_all) { 
            // -a는 모든 유용한 정보를 출력하므로, 아래 기본 로직도 타야 함
            if (record_type == RUN_LVL) {
                printf("           run-level %c        %s", entry->ut_pid, ctime((time_t *)&entry->ut_tv.tv_sec));
            } else if (record_type == BOOT_TIME) {
                printf("           system boot      %s", ctime((time_t *)&entry->ut_tv.tv_sec));
            } else if (record_type == DEAD_PROCESS) {
                printf("           DEAD_PROCESS\n");
            }
        }
        
        // -b 옵션 처리
        if (show_boot_time) {
            if (record_type == BOOT_TIME) {
                printf("         system boot  %s", ctime((time_t *)&entry->ut_tv.tv_sec));
                endutent(); // 파일을 닫고
                return 0;   // 종료
            }
            continue; 
        }

        // -m 옵션 처리
        if (show_current_term) {
            if (my_tty && record_type == USER_PROCESS && strcmp(entry->ut_line, my_tty) == 0) {
                print_entry(entry, show_term_status);
                break; // 찾았으면 루프 종료
            }
            continue;
        }

        // 기본 동작 및 -T 옵션
        if (record_type == USER_PROCESS) {
            print_entry(entry, show_term_status);
        }
    }
    endutent(); // utmp 파일 닫기
    return 0;
}


/**
 * @brief utmp 레코드 하나를 형식에 맞춰 출력
 * @param entry 출력할 utmp 구조체 포인터
 * @param show_term_status 터미널 상태(+/-) 표시 여부
 */
void print_entry(struct utmp *entry, int show_term_status) {
    char time_buf[30];
    time_t timestamp = entry->ut_tv.tv_sec;

    // 수정된 부분: localtime(&timestamp)로 올바르게 수정
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M", localtime(&timestamp));

    // 사용자 이름이 비어있지 않은 경우에만 출력
    if (entry->ut_user[0] != '\0') {
        printf("%-8s ", entry->ut_user);
    }
    
    // 터미널 상태 출력 (+: 쓰기 가능, -: 쓰기 불가, ?: 확인 불가)
    if (show_term_status) {
        char term_path[64];
        struct stat term_stat;
        snprintf(term_path, sizeof(term_path), "/dev/%s", entry->ut_line);
        if (stat(term_path, &term_stat) == 0 && (term_stat.st_mode & S_IWGRP)) {
            printf("+ ");
        } else {
            printf("- ");
        }
    }

    printf("%-12s ", entry->ut_line);
    printf("%s ", time_buf);
    
    // 호스트 정보가 있다면 출력
    if(entry->ut_host[0] != '\0') {
        printf("(%s)", entry->ut_host);
    }
    
    printf("\n");
}


/**
 * @brief 'who -q' 형식에 맞춰 사용자 목록과 총 인원을 출력
 * @param users 사용자 이름 배열
 * @param count 사용자 수
 */
void print_quick_users(char *users[], int count) {
    for (int i = 0; i < count; i++) {
        printf("%s ", users[i]);
    }
    printf("\n# users=%d\n", count);
}