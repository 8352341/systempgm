#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // getopt, unlink, rmdir
#include <dirent.h>     // opendir, readdir, closedir
#include <sys/stat.h>   // lstat
#include <errno.h>      // errno
#include <limits.h>     // PATH_MAX

// --- 옵션 구조체 정의 ---
// 전역 변수 대신, 프로그램 옵션을 담는 구조체를 사용합니다.
// 이렇게 하면 함수에 옵션을 명시적으로 전달하여 코드의 명확성과 확장성을 높일 수 있습니다.
typedef struct {
    int force;       // -f: 오류를 무시하고 강제로 실행
    int interactive; // -i: 삭제 전 사용자에게 확인
    int recursive;   // -r, -R: 디렉터리와 그 내용물을 재귀적으로 삭제
    int verbose;     // -v: 삭제되는 파일/디렉터리 목록을 출력
} RmOptions;

// 함수 선언 (main 함수에서 사용하기 위해 미리 선언)
int remove_path(const char *path, const RmOptions *opts);

/**
 * @brief 삭제 작업을 수행하기 전 사용자에게 확인을 요청합니다.
 * @param path 삭제할 파일/디렉터리의 경로
 * @param opts 프로그램 옵션 구조체
 * @return 사용자가 'y' 또는 'Y'를 입력하면 1, 그렇지 않으면 0을 반환합니다.
 *         -i (interactive) 옵션이 꺼져 있으면 항상 1을 반환합니다.
 */
int confirm_delete(const char *path, const RmOptions *opts) {
    if (!opts->interactive) {
        return 1; // 대화형 모드가 아니면 항상 'yes'로 간주
    }

    printf("rm: remove '%s'? ", path);
    // 한 글자만 읽고, 의도치 않은 입력을 방지하기 위해 입력 버퍼를 비웁니다.
    int response = getchar();
    // 사용자가 Enter 외에 다른 문자를 입력했을 경우를 대비해 버퍼를 끝까지 비웁니다.
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF); 

    return (response == 'y' || response == 'Y');
}

/**
 * @brief 디렉터리와 그 내용물을 재귀적으로 삭제합니다.
 * @param path 삭제할 디렉터리의 경로
 * @param opts 프로그램 옵션 구조체
 * @return 성공 시 0, 실패 시 -1을 반환합니다.
 */
int handle_directory_removal(const char *path, const RmOptions *opts) {
    // -r 또는 -R 옵션이 없으면 디렉터리를 삭제할 수 없습니다.
    if (!opts->recursive) {
        fprintf(stderr, "rm: cannot remove '%s': Is a directory\n", path);
        return -1;
    }

    DIR *dir = opendir(path);
    if (!dir) {
        // -f 옵션이 켜져 있지 않을 때만 오류를 출력합니다.
        if (!opts->force) {
            fprintf(stderr, "rm: cannot open directory '%s': %s\n", path, strerror(errno));
        }
        return -1;
    }

    struct dirent *entry;
    int overall_result = 0;
    
    // 디렉터리 내의 모든 항목을 순회합니다.
    while ((entry = readdir(dir)) != NULL) {
        // 현재 디렉터리(.)와 상위 디렉터리(..)는 건너뜁니다.
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // 전체 경로를 안전하게 생성합니다. (고정 크기 대신 PATH_MAX 사용)
        char subpath[PATH_MAX];
        int len = snprintf(subpath, sizeof(subpath), "%s/%s", path, entry->d_name);
        if (len >= sizeof(subpath)) {
            fprintf(stderr, "rm: path is too long: %s/%s\n", path, entry->d_name);
            overall_result = -1;
            continue; // 다음 파일로 진행
        }
        
        // 하위 경로에 대해 재귀적으로 삭제 함수를 호출합니다.
        if (remove_path(subpath, opts) != 0) {
            overall_result = -1; // 하나라도 실패하면 최종 결과는 실패
        }
    }
    closedir(dir);

    // 하위 내용이 모두 삭제되었거나, 중간에 실패했더라도 디렉터리 자체의 삭제를 시도합니다.
    if (overall_result == 0) {
        if (confirm_delete(path, opts)) {
            if (rmdir(path) != 0) {
                if (!opts->force) {
                    fprintf(stderr, "rm: cannot remove directory '%s': %s\n", path, strerror(errno));
                }
                overall_result = -1;
            } else if (opts->verbose) {
                printf("removed directory '%s'\n", path);
            }
        }
    }
    
    return overall_result;
}

/**
 * @brief 단일 파일(또는 심볼릭 링크 등)을 삭제합니다.
 * @param path 삭제할 파일의 경로
 * @param opts 프로그램 옵션 구조체
 * @return 성공 시 0, 실패 시 -1을 반환합니다.
 */
int handle_file_removal(const char *path, const RmOptions *opts) {
    if (confirm_delete(path, opts)) {
        if (unlink(path) != 0) {
            // 파일이 존재하지 않는 오류(ENOENT)는 -f 옵션이 있을 때 무시합니다.
            if (!opts->force || errno != ENOENT) {
                fprintf(stderr, "rm: cannot remove '%s': %s\n", path, strerror(errno));
            }
            // -f 옵션이 있어도 파일이 없는 경우가 아니면 실패로 간주합니다.
            if (errno != ENOENT) return -1;
        } else if (opts->verbose) {
            printf("removed '%s'\n", path);
        }
    }
    return 0; // 사용자가 'no'를 선택한 경우도 실패는 아님
}

/**
 * @brief 주어진 경로의 종류(파일, 디렉터리)를 파악하고 적절한 삭제 함수를 호출하는 분배자(dispatcher) 역할
 * @param path 삭제할 대상의 경로
 * @param opts 프로그램 옵션 구조체
 * @return 성공 시 0, 실패 시 -1을 반환합니다.
 */
int remove_path(const char *path, const RmOptions *opts) {
    struct stat st;
    // stat 대신 lstat을 사용하여 심볼릭 링크 자체의 정보를 가져옵니다.
    if (lstat(path, &st) != 0) {
        // 파일/디렉터리가 존재하지 않는 경우
        if (errno == ENOENT) {
            // -f 옵션이 켜져 있으면 오류가 아니므로 조용히 성공 처리합니다.
            if (opts->force) return 0;
            // -f 옵션이 없으면 오류 메시지를 출력합니다.
            fprintf(stderr, "rm: cannot remove '%s': No such file or directory\n", path);
        } else {
            // 그 외 lstat 오류 (예: 권한 문제)
            fprintf(stderr, "rm: cannot access '%s': %s\n", path, strerror(errno));
        }
        return -1;
    }

    // 경로가 디렉터리인지 확인하고, 그에 맞는 핸들러를 호출합니다.
    if (S_ISDIR(st.st_mode)) {
        return handle_directory_removal(path, opts);
    } else {
        return handle_file_removal(path, opts);
    }
}

int main(int argc, char *argv[]) {
    // 옵션 구조체를 0으로 초기화 (모든 플래그를 false로 설정)
    RmOptions options = {0};
    int opt;

    // POSIX 표준인 getopt를 사용하여 커맨드 라인 옵션을 파싱합니다.
    while ((opt = getopt(argc, argv, "firRv")) != -1) {
        switch (opt) {
            case 'f':
                options.force = 1;
                options.interactive = 0; // -f는 -i를 무시합니다.
                break;
            case 'i':
                options.interactive = 1;
                break;
            case 'r':
            case 'R':
                options.recursive = 1;
                break;
            case 'v':
                options.verbose = 1;
                break;
            default: // 알 수 없는 옵션
                fprintf(stderr, "Usage: %s [-firRv] file...\n", argv[0]);
                return EXIT_FAILURE;
        }
    }

    // 옵션 파싱 후, 파일 인자가 하나도 없는 경우
    if (optind >= argc) {
        // -f 옵션이 있을 때는 실제 rm처럼 아무 메시지도 출력하지 않습니다.
        if (!options.force) {
            fprintf(stderr, "rm: missing operand\n");
            fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
        }
        return EXIT_FAILURE;
    }

    int exit_status = EXIT_SUCCESS;
    // 옵션이 아닌, 실제 파일/디렉터리 인자들을 순회합니다.
    for (int i = optind; i < argc; i++) {
        if (remove_path(argv[i], &options) != 0) {
            exit_status = EXIT_FAILURE; // 한 번이라도 실패하면, 프로그램은 실패 코드로 종료
        }
    }

    return exit_status;
}