#include <stdio.h>
#include <sys/utsname.h> // for uname
#include <unistd.h>      // for getopt
#include <stdlib.h>      // for exit

int main(int argc, char *argv[]) {
    struct utsname uts_info;
    
    // 출력할 정보를 결정하는 플래그 변수들
    int show_s = 0; // sysname
    int show_n = 0; // nodename
    int show_r = 0; // release
    int show_v = 0; // version
    int show_m = 0; // machine

    int opt;

    // 먼저 uname 시스템 호출로 모든 정보를 가져옴
    if (uname(&uts_info) == -1) {
        perror("uname");
        return 1;
    }

    // --- getopt를 활용한 확장된 옵션 파싱 ---
    // "asnrvm"은 -a, -s, -n, -r, -v, -m 옵션을 모두 허용한다는 의미
    while ((opt = getopt(argc, argv, "asnrvm")) != -1) {
        switch (opt) {
            case 'a': // -a 옵션은 모든 플래그를 활성화
                show_s = 1;
                show_n = 1;
                show_r = 1;
                show_v = 1;
                show_m = 1;
                break;
            case 's':
                show_s = 1;
                break;
            case 'n':
                show_n = 1;
                break;
            case 'r':
                show_r = 1;
                break;
            case 'v':
                show_v = 1;
                break;
            case 'm':
                show_m = 1;
                break;
            case '?': // 인식할 수 없는 옵션 처리
                fprintf(stderr, "사용법: %s [-a | -s | -n | -r | -v | -m]...\n", argv[0]);
                exit(EXIT_FAILURE);
            default:
                exit(EXIT_FAILURE);
        }
    }

    // --- 출력 로직 ---
    // 만약 아무 옵션도 주어지지 않았다면, 기본값으로 -s를 처리
    if (argc == 1) {
        show_s = 1;
    }

    int space_needed = 0; // 출력 사이에 공백을 넣기 위한 플래그

    // 플래그가 설정된 정보를 순서대로 출력
    if (show_s) {
        printf("%s", uts_info.sysname);
        space_needed = 1;
    }
    if (show_n) {
        if (space_needed) printf(" ");
        printf("%s", uts_info.nodename);
        space_needed = 1;
    }
    if (show_r) {
        if (space_needed) printf(" ");
        printf("%s", uts_info.release);
        space_needed = 1;
    }
    if (show_v) {
        if (space_needed) printf(" ");
        printf("%s", uts_info.version);
        space_needed = 1;
    }
    if (show_m) {
        if (space_needed) printf(" ");
        printf("%s", uts_info.machine);
        space_needed = 1;
    }

    // 출력이 하나라도 있었다면 마지막에 줄바꿈 추가
    if (space_needed || argc == 1) {
        printf("\n");
    }

    return 0;
}