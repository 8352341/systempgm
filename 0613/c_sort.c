#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// --- 구조체 및 상수 정의 ---

// 프로그램 옵션을 담는 구조체. 전역 변수 남용을 피하고 코드의 명확성을 높입니다.
typedef struct {
    int sort_key;       // -k: 정렬 기준 필드 번호 (0이면 전체 라인)
    char delimiter;     // -t: 필드 구분자
    int numeric;        // -n: 숫자 기준으로 정렬
    int reverse;        // -r: 역순으로 정렬
    int unique;         // -u: 중복된 라인 제거
} SortOptions;

// C 표준 qsort는 컨텍스트 포인터를 전달할 수 없으므로,
// 비교 함수가 접근할 수 있도록 옵션을 담는 정적(static) 전역 인스턴스를 사용합니다.
// 이는 여러 개의 전역 변수를 사용하는 것보다 훨씬 깔끔하고 관리하기 좋은 절충안입니다.
static SortOptions g_opts;

// --- 함수 선언 ---
int compare_lines(const void *a, const void *b);


/**
 * @brief 라인에서 특정 필드를 추출하여 목적지 버퍼에 복사합니다.
 * @param dest 필드 내용을 저장할 버퍼
 * @param dest_size 버퍼의 크기
 * @param line 원본 라인 문자열
 * @param key 추출할 필드 번호 (1부터 시작)
 * @param delimiter 필드 구분자
 */
void get_field_from_line(char *dest, size_t dest_size, const char *line, int key, char delimiter) {
    const char *start = line;
    const char *end;
    int current_key = 1;

    // 원하는 키를 찾을 때까지 구분자를 기준으로 이동
    while (current_key < key) {
        start = strchr(start, delimiter);
        if (!start) { // 구분자를 더 찾을 수 없으면 빈 문자열 처리
            dest[0] = '\0';
            return;
        }
        start++; // 구분자 다음 문자로 이동
        current_key++;
    }

    // 필드의 끝(다음 구분자 또는 문자열의 끝)을 찾음
    end = strchr(start, delimiter);
    if (!end) {
        end = start + strlen(start);
    }
    
    // 필드 내용을 버퍼에 안전하게 복사
    size_t len = end - start;
    if (len >= dest_size) {
        len = dest_size - 1;
    }
    memcpy(dest, start, len);
    dest[len] = '\0';
}


/**
 * @brief qsort에 사용될 비교 함수. 두 라인을 g_opts에 따라 비교합니다.
 *        이 함수는 `-u` 옵션의 중복 검사에도 재사용됩니다.
 */
int compare_lines(const void *a, const void *b) {
    const char *line1 = *(const char **)a;
    const char *line2 = *(const char **)b;
    
    // 임시 버퍼를 스택에 할당하여 get_field의 static 변수 버그를 원천 차단
    char field1_buf[1024];
    char field2_buf[1024];

    const char *p1 = line1;
    const char *p2 = line2;

    if (g_opts.sort_key > 0) {
        // -k 옵션이 주어지면, 지정된 필드를 추출하여 비교 대상으로 삼음
        get_field_from_line(field1_buf, sizeof(field1_buf), line1, g_opts.sort_key, g_opts.delimiter);
        get_field_from_line(field2_buf, sizeof(field2_buf), line2, g_opts.sort_key, g_opts.delimiter);
        p1 = field1_buf;
        p2 = field2_buf;
    }

    int cmp_result;
    if (g_opts.numeric) {
        // -n: 숫자 비교
        double num1 = atof(p1);
        double num2 = atof(p2);
        if (num1 < num2) cmp_result = -1;
        else if (num1 > num2) cmp_result = 1;
        else cmp_result = 0;
    } else {
        // 기본: 문자열 비교
        cmp_result = strcmp(p1, p2);
    }

    // -r: 역순 정렬
    return g_opts.reverse ? -cmp_result : cmp_result;
}


int main(int argc, char *argv[]) {
    // 1. 옵션 파싱 (g_opts 구조체에 저장)
    // 구조체를 0으로 초기화하고 기본 구분자 설정
    memset(&g_opts, 0, sizeof(g_opts));
    g_opts.delimiter = ' '; 

    int opt;
    while ((opt = getopt(argc, argv, "rnuk:t:")) != -1) {
        switch (opt) {
            case 'r': g_opts.reverse = 1; break;
            case 'n': g_opts.numeric = 1; break;
            case 'u': g_opts.unique = 1; break;
            case 'k': g_opts.sort_key = atoi(optarg); break;
            case 't': g_opts.delimiter = optarg[0]; break;
            default:
                fprintf(stderr, "Usage: %s [-rnu] [-k field] [-t delim] [file]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // 2. 파일 처리
    FILE *fp = stdin;
    if (optind < argc) {
        fp = fopen(argv[optind], "r");
        if (!fp) {
            perror("Error opening file");
            exit(EXIT_FAILURE);
        }
    }
    
    // 3. 동적 배열을 사용하여 라인 읽기 (메모리 오버플로우 방지)
    size_t capacity = 1024; // 초기 용량
    size_t line_count = 0;
    char **lines = malloc(capacity * sizeof(char *));
    if (!lines) {
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), fp)) {
        if (line_count >= capacity) {
            // 용량이 부족하면 2배로 늘림
            capacity *= 2;
            char **new_lines = realloc(lines, capacity * sizeof(char *));
            if (!new_lines) {
                perror("Failed to reallocate memory");
                // 기존 메모리 해제 후 종료
                for (size_t i = 0; i < line_count; i++) free(lines[i]);
                free(lines);
                exit(EXIT_FAILURE);
            }
            lines = new_lines;
        }
        lines[line_count++] = strdup(buffer);
    }
    if (fp != stdin) fclose(fp);

    // 4. 정렬
    qsort(lines, line_count, sizeof(char *), compare_lines);

    // 5. 결과 출력
    for (size_t i = 0; i < line_count; i++) {
        // -u 옵션 처리: 이전 라인과 비교하여 다를 때만 출력
        // 비교 시 qsort에 사용된 것과 동일한 compare_lines 함수를 재사용하여 정확성 보장
        if (g_opts.unique && i > 0 && compare_lines(&lines[i - 1], &lines[i]) == 0) {
            continue; // 중복된 라인이면 건너뜀
        }
        printf("%s", lines[i]);
    }

    // 6. 메모리 해제
    for (size_t i = 0; i < line_count; i++) {
        free(lines[i]);
    }
    free(lines);

    return 0;
}