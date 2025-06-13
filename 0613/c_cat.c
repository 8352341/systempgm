#include <stdio.h>
#include <stdlib.h>     // for exit
#include <unistd.h>     // for getopt
#include <string.h>     // for strcmp, strlen

// 메인 함수: 프로그램의 시작점
int main(int argc, char *argv[]) {
    int opt;

    // 옵션 상태를 저장할 플래그 변수들
    int show_line_numbers = 0;  // -n: 줄 번호 표시 여부
    int show_ends = 0;          // -E: 줄 끝에 '$' 표시 여부
    int squeeze_blank = 0;      // -s: 연속된 빈 줄 압축 여부

    // getopt를 사용하여 명령줄 옵션을 파싱
    // "nEs"는 -n, -E, -s 옵션을 허용한다는 의미
    while ((opt = getopt(argc, argv, "nEs")) != -1) {
        switch(opt) {
            case 'n': show_line_numbers = 1; break;
            case 'E': show_ends = 1;         break;
            case 's': squeeze_blank = 1;     break;
            default: // 인식할 수 없는 옵션일 경우
                fprintf(stderr, "사용법: %s [-nEs] [파일...]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // 파일 포인터. 기본값은 표준 입력(stdin)으로, 파이프 등으로 입력을 받을 수 있다.
    FILE *fp = stdin;
    
    // optind는 getopt가 처리한 마지막 인덱스의 다음을 가리킨다.
    // 즉, 옵션이 아닌 첫 번째 파일 이름을 가리킴
    if (optind < argc) {
        // 파일 이름이 주어졌다면, 해당 파일을 읽기 모드("r")로 연다.
        fp = fopen(argv[optind], "r");
        if (!fp) {
            perror("fopen"); // 파일 열기 실패 시 에러 메시지 출력
            exit(EXIT_FAILURE);
        }
    }

    char line[1024];        // 한 줄을 읽어올 버퍼
    int line_num = 1;       // 줄 번호 카운터
    int prev_is_blank = 0;  // 이전 줄이 빈 줄이었는지를 기억하는 상태 플래그
SSS
    // fgets()를 사용하여 파일에서 한 줄씩 읽는다. 파일 끝에 도달하면 NULL을 반환.
    while (fgets(line, sizeof(line), fp) != NULL) {
        
        // --- -s 옵션 (squeeze_blank) 처리 ---
        if (squeeze_blank) {
            // 현재 줄이 빈 줄('\n'만 있는 줄)인지 확인
            if (strcmp(line, "\n") == 0) {
                // 이전 줄도 빈 줄이었다면, 이번 줄은 건너뛴다(continue).
                if (prev_is_blank) {
                    continue; 
                }
                // 이번 줄이 첫 번째 빈 줄이라면, 상태 플래그를 켠다.
                prev_is_blank = 1;
            } else {
                // 현재 줄이 빈 줄이 아니라면, 상태 플래그를 끈다.
                prev_is_blank = 0;
            }
        }

        // --- -n 옵션 (show_line_numbers) 처리 ---
        // 실제 cat은 빈 줄이 출력되지 않아도 줄 번호는 증가시킨다.
        // 따라서 여기서 출력 여부와 상관없이 번호를 붙인다.
        if (show_line_numbers) {
            printf("%6d\t", line_num++);
        }

        // --- -E 옵션 (show_ends) 처리 및 최종 출력 ---
        if (show_ends) {
            // 줄 끝의 개행 문자(\n)를 찾아서 널 문자(\0)로 바꿔 제거한다.
            size_t len = strlen(line);
            if (len > 0 && line[len - 1] == '\n') {
                line[len - 1] = '\0';
            }
            // 내용물 뒤에 '$'와 개행 문자를 붙여서 출력한다.
            printf("%s$\n", line);
        } else {
            // -E 옵션이 없으면 읽은 그대로 출력한다.
            printf("%s", line);
        }
    }

    // 표준 입력(stdin)을 사용한 게 아니라면, 열었던 파일을 닫아준다.
    if (fp != stdin) {
        fclose(fp);
    }

    return 0; // 프로그램 정상 종료
}