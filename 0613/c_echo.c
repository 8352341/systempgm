#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h> // isdigit, isxdigit

// --- 옵션 구조체 정의 ---
// 프로그램의 동작을 제어하는 옵션들을 구조체로 묶어 관리합니다.
// 전역 변수 대신 이 구조체를 함수에 전달하여 코드의 명확성을 높입니다.
typedef struct {
    int no_newline;     // -n: 마지막에 개행 문자를 출력하지 않음
    int enable_escapes; // -e: 백슬래시 이스케이프 시퀀스를 해석
} EchoOptions;

/**
 * @brief 주어진 문자열을 옵션에 따라 해석하고 출력합니다.
 *        백슬래시 이스케이프 처리를 담당합니다.
 * @param str 출력할 문자열
 * @param opts 프로그램 옵션 구조체
 * @return \c 시퀀스가 발견되면 1을 반환하여 이후 출력을 중단하도록 신호하고, 그렇지 않으면 0을 반환합니다.
 */
int process_and_print(const char *str, const EchoOptions *opts) {
    if (!opts->enable_escapes) {
        // 이스케이프 비활성화(-E) 모드에서는 문자열을 그대로 출력합니다.
        fputs(str, stdout);
        return 0;
    }

    // 이스케이프 활성화(-e) 모드
    while (*str) {
        if (*str == '\\' && *(str + 1) != '\0') {
            str++; // 백슬래시 다음 문자로 이동
            switch (*str) {
                case 'a': putchar('\a'); break; // 경고음 (bell)
                case 'b': putchar('\b'); break; // 백스페이스
                case 'c': return 1;             // 이후 모든 출력을 중단 (개행 포함)
                case 'e': putchar('\x1B'); break; // ESC (escape) 문자
                case 'f': putchar('\f'); break; // 폼 피드
                case 'n': putchar('\n'); break; // 개행
                case 'r': putchar('\r'); break; // 캐리지 리턴
                case 't': putchar('\t'); break; // 수평 탭
                case 'v': putchar('\v'); break; // 수직 탭
                case '\\': putchar('\\'); break; // 백슬래시 자체
                // case '"': putchar('\"'); break; // echo는 따옴표를 이스케이프하지 않음 (쉘이 처리)
                
                // 8진수 처리 (예: \033) - 1~3자리
                case '0': {
                    char octal_str[4] = {0};
                    for (int i = 0; i < 3 && str[i+1] >= '0' && str[i+1] <= '7'; ++i) {
                        octal_str[i] = *(++str);
                    }
                    putchar(strtol(octal_str, NULL, 8));
                    break;
                }
                
                default:
                    // 인식되지 않는 이스케이프 시퀀스는 백슬래시와 문자를 그대로 출력
                    putchar('\\');
                    putchar(*str);
                    break;
            }
        } else {
            // 일반 문자는 그대로 출력
            putchar(*str);
        }
        str++;
    }
    return 0; // 정상적으로 문자열 끝까지 처리
}

int main(int argc, char *argv[]) {
    // 옵션 구조체를 기본값으로 초기화
    // 기본 동작은 -E (이스케이프 비활성화), -n 없음.
    EchoOptions options = { .no_newline = 0, .enable_escapes = 0 };
    int opt;

    // 옵션 파싱: getopt는 옵션이 아닌 첫 인자를 만나면 중단됨
    // 쉘 기본 echo는 옵션 파싱 순서가 중요하므로, POSIX 표준을 따릅니다.
    while ((opt = getopt(argc, argv, "neE")) != -1) {
        switch (opt) {
            case 'n':
                options.no_newline = 1;
                break;
            case 'e':
                options.enable_escapes = 1;
                break;
            case 'E':
                options.enable_escapes = 0;
                break;
            default: // 알 수 없는 옵션
                fprintf(stderr, "Usage: %s [-n] [-e|-E] [string ...]\n", argv[0]);
                return 1;
        }
    }

    // 출력 중단 플래그 (\c를 만났을 때 true로 설정)
    int output_halted = 0; 
    
    // 옵션 다음의 인자들을 순회하며 출력
    for (int i = optind; i < argc; i++) {
        if (process_and_print(argv[i], &options)) {
            output_halted = 1;
            break; // \c를 만났으므로 루프 즉시 탈출
        }
        
        // 마지막 인자가 아니라면 공백으로 구분
        if (i < argc - 1) {
            putchar(' ');
        }
    }

    // -n 옵션이 없고, \c 시퀀스가 등장하지 않았을 때만 마지막에 개행 문자를 출력
    if (!options.no_newline && !output_halted) {
        putchar('\n');
    }

    return 0;
}