#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

// --- 상수 및 구조체 정의 ---
#define MAX_LINE_LENGTH 4096 // 한 줄의 최대 길이를 상수로 정의

// 프로그램 옵션을 담는 구조체. 전역 변수 대신 사용하여 코드의 명확성을 높입니다.
typedef struct {
    int ignore_case;      // -i: 대소문자 무시
    int invert_match;     // -v: 매치되지 않는 라인 선택
    int show_line_number; // -n: 라인 번호 출력
    int count_only;       // -c: 매치된 라인의 수만 출력
    const char *pattern;  // 검색할 패턴 문자열
} GrepOptions;


// --- 함수 선언 ---
// strcasestr이 시스템에 없을 경우를 대비한 사용자 정의 함수
const char *my_strcasestr(const char *haystack, const char *needle);
// 파일/스트림을 처리하는 핵심 로지
void process_stream(FILE *stream, const char *filename, const GrepOptions *opts, int *total_matches);


// --- strcasestr 구현 ---
// _GNU_SOURCE가 정의되어 있으면 string.h에 strcasestr이 포함될 가능성이 높습니다.
// 여기서는 간단하게 직접 구현한 버전을 사용합니다.
const char *my_strcasestr(const char *haystack, const char *needle) {
    if (!*needle) return haystack; // 빈 패턴은 항상 매치

    for (; *haystack; haystack++) {
        // 현재 위치에서 needle과 일치하는지 검사
        const char *h = haystack;
        const char *n = needle;
        while (*h && *n && tolower((unsigned char)*h) == tolower((unsigned char)*n)) {
            h++;
            n++;
        }
        if (*n == '\0') { // needle의 끝까지 도달했다면 매치 성공
            return haystack;
        }
    }
    return NULL; // 매치 실패
}

/**
 * @brief 주어진 라인이 패턴과 일치하는지 확인합니다.
 * @param line 검사할 텍스트 라인
 * @param opts 프로그램 옵션 구조체
 * @return 일치하면 1, 그렇지 않으면 0을 반환합니다.
 */
int line_matches(const char *line, const GrepOptions *opts) {
    const char *match_ptr;
    if (opts->ignore_case) {
        match_ptr = my_strcasestr(line, opts->pattern);
    } else {
        match_ptr = strstr(line, opts->pattern);
    }
    
    // invert_match(-v) 옵션을 고려하여 최종 결과를 반환합니다.
    // XOR(^) 연산자: (match_ptr != NULL)과 opts->invert_match가 다르면 true(1)
    // -v 미사용: 매치되면(1) 1, 안되면(0) 0 반환
    // -v 사용  : 매치되면(1) 0, 안되면(0) 1 반환
    return (match_ptr != NULL) ^ (opts->invert_match);
}

/**
 * @brief 단일 파일(또는 stdin)을 읽고 grep 로직을 수행합니다.
 * @param stream 처리할 파일 스트림 (FILE*)
 * @param filename 출력에 사용할 파일 이름 (stdin의 경우 "(standard input)")
 * @param opts 프로그램 옵션 구조체
 * @param multiple_files 여러 파일을 처리 중인지 여부 (출력 형식 결정에 사용)
 */
void process_file(FILE *stream, const char *filename, const GrepOptions *opts, int multiple_files) {
    char line[MAX_LINE_LENGTH];
    int line_number = 0;
    int match_count = 0;

    while (fgets(line, sizeof(line), stream)) {
        line_number++;
        if (line_matches(line, opts)) {
            match_count++;
            if (!opts->count_only) {
                if (multiple_files) {
                    printf("%s:", filename);
                }
                if (opts->show_line_number) {
                    printf("%d:", line_number);
                }
                fputs(line, stdout);
            }
        }
    }

    if (opts->count_only) {
        if (multiple_files) {
            printf("%s:", filename);
        }
        printf("%d\n", match_count);
    }
}

int main(int argc, char *argv[]) {
    GrepOptions options = {0}; // 옵션 구조체 0으로 초기화
    int opt;

    // 1. 옵션 파싱
    // -e 옵션이 여러 번 나올 수 있으므로, 루프 안에서 패턴을 설정합니다.
    while ((opt = getopt(argc, argv, "ivnce:")) != -1) {
        switch (opt) {
            case 'i': options.ignore_case = 1; break;
            case 'v': options.invert_match = 1; break;
            case 'n': options.show_line_number = 1; break;
            case 'c': options.count_only = 1; break;
            case 'e': options.pattern = optarg; break;
            default:
                fprintf(stderr, "Usage: %s [-ivnc] [-e pattern] [pattern] [file...]\n", argv[0]);
                return 2; // grep은 오류 시 2를 반환
        }
    }
    
    // 2. 패턴 확정
    // -e 옵션이 없었다면, 첫 번째 non-option 인자를 패턴으로 간주합니다.
    if (!options.pattern) {
        if (optind >= argc) {
            fprintf(stderr, "Usage: %s [-ivnc] [-e pattern] [pattern] [file...]\n", argv[0]);
            return 2;
        }
        options.pattern = argv[optind++];
    }

    // 3. 파일 처리
    int matches_found = 0;
    int multiple_files = (argc - optind > 1);

    if (optind == argc) {
        // 처리할 파일 인자가 없으면 표준 입력(stdin)에서 읽어옵니다.
        process_file(stdin, "(standard input)", &options, 0);
    } else {
        // 파일 인자들을 순회하며 처리합니다.
        for (int i = optind; i < argc; i++) {
            FILE *fp = fopen(argv[i], "r");
            if (!fp) {
                // grep 스타일의 오류 메시지 출력
                fprintf(stderr, "%s: %s: %s\n", argv[0], argv[i], strerror(2));
                continue; // 다음 파일로 진행
            }
            process_file(fp, argv[i], &options, multiple_files);
            fclose(fp);
        }
    }
    
    // grep의 반환 코드 규칙: 0(매치 발견), 1(매치 없음), 2(오류)
    // 이 간단한 버전에서는 단순 성공/실패만 구분합니다.
    return EXIT_SUCCESS;
}