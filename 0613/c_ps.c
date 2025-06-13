#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>     // opendir, readdir, closedir
#include <ctype.h>      // isdigit
#include <unistd.h>     // getuid, getopt
#include <pwd.h>        // getpwuid

// /proc 파일 시스템 내의 파일 경로를 정의하는 매크로
// 각 프로세스(PID)의 정보는 /proc/<PID>/ 디렉터리 아래에 파일로 저장된다.
#define PROC_DIR "/proc"
#define STAT_PATH "/proc/%s/stat"       // 프로세스 상태 정보 (CPU, 메모리 등)
#define STATUS_PATH "/proc/%s/status"   // 사람이 읽기 쉬운 상태 정보 (UID, GID 등)
#define CMDLINE_PATH "/proc/%s/cmdline" // 프로세스 실행 시 사용된 전체 명령어

// 프로세스 정보를 담기 위한 구조체
typedef struct {
    char pid[10];
    char ppid[10];
    char uid_str[10];
    char tty_nr[10];  // TTY 정보 (여기서는 간단히 처리)
    char cmd[512];
    char user[32];
} ProcessInfo;

// 함수 선언
int is_pid_dir(const struct dirent *entry);
int get_process_info(ProcessInfo *p_info, const char *pid);
void get_username_from_status(const char *pid, char *username_buf, size_t buf_size);

// 메인 함수
int main(int argc, char *argv[]) {
    // 옵션 상태를 저장할 플래그 변수
    int show_all_users = 0; // -e: 모든 사용자의 프로세스 표시
    int full_format = 0;    // -f: 상세 포맷 (UID, PPID 등) 표시
    
    // 현재 프로그램 실행자의 UID. -u 옵션의 기본값으로 사용된다.
    uid_t my_uid = getuid();
    
    int opt;
    // getopt를 사용하여 명령줄 옵션을 파싱
    while ((opt = getopt(argc, argv, "ef")) != -1) {
        switch (opt) {
            case 'e':
                show_all_users = 1;
                break;
            case 'f':
                full_format = 1;
                break;
            default:
                fprintf(stderr, "사용법: %s [-e] [-f]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    
    DIR *proc_dir = opendir(PROC_DIR);
    if (!proc_dir) {
        perror("opendir /proc");
        exit(EXIT_FAILURE);
    }
    
    struct dirent *entry;

    // 헤더 출력: -f 옵션 여부에 따라 다르게 출력
    if (full_format) {
        printf("%-8s %5s %5s %-20s\n", "UID", "PID", "PPID", "CMD");
    } else {
        printf("%5s %-10s %s\n", "PID", "TTY", "CMD");
    }
    
    // /proc 디렉터리를 순회하며 각 항목을 읽음
    while ((entry = readdir(proc_dir)) != NULL) {
        // 현재 항목이 숫자로만 이루어진 디렉터리(즉, PID)인지 확인
        if (!is_pid_dir(entry)) {
            continue;
        }

        ProcessInfo p_info = {0}; // 구조체 초기화
        // 프로세스 정보를 가져온다. 실패 시 건너뜀.
        if (!get_process_info(&p_info, entry->d_name)) {
            continue;
        }
        
        // --- 옵션에 따른 필터링 ---
        // -e 옵션이 없으면, 현재 사용자의 프로세스만 보여준다.
        if (!show_all_users) {
            uid_t process_uid = (uid_t)atoi(p_info.uid_str);
            if (process_uid != my_uid) {
                continue;
            }
        }
        
        // --- 최종 출력 ---
        if (full_format) { // -f 옵션: UID, PID, PPID, CMD
            printf("%-8s %5s %5s %-20s\n", p_info.user, p_info.pid, p_info.ppid, p_info.cmd);
        } else { // 기본: PID, TTY, CMD
            printf("%5s %-10s %s\n", p_info.pid, p_info.tty_nr, p_info.cmd);
        }
    }
    
    closedir(proc_dir);
    return 0;
}

/**
 * @brief direntry가 숫자로만 이루어진 디렉토리 이름(PID)인지 확인
 * @param entry readdir()로 읽은 dirent 구조체 포인터
 * @return PID 디렉터리면 1, 아니면 0
 */
int is_pid_dir(const struct dirent *entry) {
    for (const char *p = entry->d_name; *p; p++) {
        if (!isdigit(*p)) {
            return 0;
        }
    }
    return 1;
}

/**
 * @brief 특정 PID에 대한 주요 정보를 수집하여 ProcessInfo 구조체에 채운다.
 * @param p_info 정보를 채울 ProcessInfo 구조체 포인터
 * @param pid 정보를 가져올 프로세스의 ID (문자열)
 * @return 성공 시 1, 실패 시 0
 */
int get_process_info(ProcessInfo *p_info, const char *pid) {
    char path[256];
    FILE *fp;

    // --- 1. /proc/<pid>/stat 파일에서 PID, PPID, TTY 정보 가져오기 ---
    snprintf(path, sizeof(path), STAT_PATH, pid);
    fp = fopen(path, "r");
    if (!fp) return 0;
    
    // stat 파일은 공백으로 구분된 여러 값을 포함한다. 필요한 값만 파싱한다.
    // %*s 는 해당 필드를 읽고 무시하라는 의미.
    fscanf(fp, "%s %*s %*c %s %*d %*d %s",
           p_info->pid, p_info->ppid, p_info->tty_nr);
    fclose(fp);

    // --- 2. /proc/<pid>/cmdline 파일에서 명령어 정보 가져오기 ---
    snprintf(path, sizeof(path), CMDLINE_PATH, pid);
    fp = fopen(path, "r");
    if (fp) {
        size_t len = fread(p_info->cmd, 1, sizeof(p_info->cmd) - 1, fp);
        p_info->cmd[len] = '\0'; // Null-terminate
        // cmdline은 인자가 NULL 문자로 구분되어 있으므로 첫 번째 NULL을 공백으로 바꿔준다.
        for (int i = 0; i < len; i++) {
            if (p_info->cmd[i] == '\0') p_info->cmd[i] = ' ';
        }
        fclose(fp);
    } else { // cmdline 파일이 없는 경우 (커널 스레드 등)
        strncpy(p_info->cmd, "[kernel thread]", sizeof(p_info->cmd));
    }


    // --- 3. /proc/<pid>/status 파일에서 UID를 가져와 사용자 이름으로 변환 ---
    get_username_from_status(pid, p_info->user, sizeof(p_info->user));
    
    // TTY 번호를 "pts/0" 같은 이름으로 변환하는 로직은 복잡하여 '?'로 대체
    if (strcmp(p_info->tty_nr, "0") == 0) {
        strcpy(p_info->tty_nr, "?");
    } else {
        // 실제로는 dev_t major/minor 번호를 tty 이름으로 매핑해야 함
        strcpy(p_info->tty_nr, "pts/x"); 
    }
    
    return 1;
}


/**
 * @brief /proc/<pid>/status 파일에서 UID를 읽고, 사용자 이름으로 변환한다.
 * @param pid 프로세스 ID (문자열)
 * @param username_buf 사용자 이름을 저장할 버퍼
 * @param buf_size 버퍼의 크기
 */
void get_username_from_status(const char *pid, char *username_buf, size_t buf_size) {
    char path[256], line[256];
    uid_t uid = -1;

    snprintf(path, sizeof(path), STATUS_PATH, pid);
    FILE *fp = fopen(path, "r");
    if (!fp) {
        strncpy(username_buf, "?", buf_size);
        return;
    }
    
    // "Uid:"로 시작하는 라인을 찾아 UID 값을 파싱한다.
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "Uid:", 4) == 0) {
            sscanf(line, "Uid:\t%u", &uid);
            break;
        }
    }
    fclose(fp);

    // UID를 사용자 이름으로 변환한다.
    struct passwd *pw = getpwuid(uid);
    if (pw) {
        strncpy(username_buf, pw->pw_name, buf_size);
    } else { // 해당 UID의 사용자가 없을 경우
        snprintf(username_buf, buf_size, "%u", uid); // 그냥 UID 숫자를 쓴다.
    }
}