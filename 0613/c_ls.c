#include <stdio.h>
#include <stdlib.h>     // for qsort, exit
#include <string.h>     // for strcmp, snprintf
#include <unistd.h>     // for getopt
#include <dirent.h>     // for opendir, readdir, closedir
#include <sys/stat.h>   // for stat structure
#include <time.h>       // for localtime, strftime
#include <pwd.h>        // for getpwuid (사용자 이름 가져오기)
#include <grp.h>        // for getgrgid (그룹 이름 가져오기)

// 파일 하나의 정보를 저장하기 위한 구조체
// 정렬을 위해 파일 이름과 stat 정보를 함께 저장해야 함
typedef struct {
    char name[256];      // 파일 이름
    struct stat st;      // 파일의 모든 메타데이터 (크기, 시간, 권한 등)
} FileInfo;

// qsort를 위한 비교 함수: 수정 시간(mtime) 기준 내림차순 정렬
// 최신 파일이 먼저 오도록 정렬한다.
int compare_mtime(const void *a, const void *b) {
    FileInfo *fileA = (FileInfo *)a;
    FileInfo *fileB = (FileInfo *)b;
    // b의 시간이 더 크면(최신이면) 양수를 반환하여 b를 앞으로 보냄
    return fileB->st.st_mtime - fileA->st.st_mtime;
}

// qsort를 위한 비교 함수: 파일 크기(size) 기준 내림차순 정렬
// 크기가 큰 파일이 먼저 오도록 정렬한다.
int compare_size(const void *a, const void *b) {
    FileInfo *fileA = (FileInfo *)a;
    FileInfo *fileB = (FileInfo *)b;
    // b의 크기가 더 크면 양수를 반환하여 b를 앞으로 보냄
    return fileB->st.st_size - fileA->st.st_size;
}

// stat 구조체의 st_mode 값을 해석하여 파일 권한을 출력하는 함수
void print_permissions(mode_t mode) {
    // 1. 파일 종류 출력 (디렉토리, 일반 파일 등)
    printf(S_ISDIR(mode) ? "d" : "-");

    // 2. 소유자(User)의 권한 (읽기, 쓰기, 실행)
    printf((mode & S_IRUSR) ? "r" : "-");
    printf((mode & S_IWUSR) ? "w" : "-");
    printf((mode & S_IXUSR) ? "x" : "-");

    // 3. 그룹(Group)의 권한
    printf((mode & S_IRGRP) ? "r" : "-");
    printf((mode & S_IWGRP) ? "w" : "-");
    printf((mode & S_IXGRP) ? "x" : "-");

    // 4. 그 외 사용자(Others)의 권한
    printf((mode & S_IROTH) ? "r" : "-");
    printf((mode & S_IWOTH) ? "w" : "-");
    printf((mode & S_IXOTH) ? "x" : "-");
}

// ls -l 형식에 맞춰 파일의 상세 정보를 출력하는 함수
void print_long_format(FileInfo *f) {
    // 권한 출력
    print_permissions(f->st.st_mode);

    // 링크 수, 소유자 이름, 그룹 이름, 파일 크기 출력
    printf(" %2lu", f->st.st_nlink);
    printf(" %-8s", getpwuid(f->st.st_uid)->pw_name);
    printf(" %-8s", getgrgid(f->st.st_gid)->gr_name);
    printf(" %8ld", f->st.st_size);

    // 최종 수정 시간을 보기 좋은 형식으로 변환하여 출력
    char timebuf[20];
    strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", localtime(&f->st.st_mtime));
    printf(" %s", timebuf);

    // 파일 이름 출력
    printf(" %s\n", f->name);
}

// 핵심 로직: 특정 디렉터리의 내용을 목록으로 보여주는 함수
void list_directory(const char *path, int show_all, int long_format, int recursive, int sort_time, int sort_size) {
    DIR *dir; // 디렉터리 스트림을 가리키는 포인터
    struct dirent *entry; // 디렉터리 내의 항목(파일/디렉토리)을 가리키는 포인터

    // opendir()로 지정된 경로의 디렉터리를 연다.
    if (!(dir = opendir(path))) {
        perror("opendir");
        return;
    }

    FileInfo files[1024]; // 파일 정보들을 저장할 배열 (최대 1024개로 제한)
    int count = 0; // 디렉터리 내 항목의 개수

    // readdir()을 반복 호출하여 디렉터리 내의 모든 항목을 읽는다.
    while ((entry = readdir(dir)) != NULL) {
        // -a 옵션이 없으면 '.'으로 시작하는 숨김 파일은 건너뛴다.
        if (!show_all && entry->d_name[0] == '.') {
            continue;
        }

        // 파일의 전체 경로를 생성 (예: "mydir/myfile.txt")
        char fullpath[512];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);
        
        // stat() 시스템 호출로 파일의 메타데이터를 가져와 files 배열에 저장
        if (stat(fullpath, &files[count].st) == -1) {
            perror("stat");
            continue; // stat 실패 시 해당 파일은 건너뜀
        }
        
        // 파일 이름만 구조체에 복사
        strncpy(files[count].name, entry->d_name, sizeof(files[count].name) - 1);
        files[count].name[sizeof(files[count].name) - 1] = '\0'; // 안전장치

        count++; // 파일 개수 증가
    }
    closedir(dir); // 디렉터리 스트림을 닫는다. (자원 해제)

    // 정렬 옵션 처리
    if (sort_time) { // -t 옵션: 시간순 정렬
        qsort(files, count, sizeof(FileInfo), compare_mtime);
    } 
    else if (sort_size) { // -S 옵션: 크기순 정렬
        qsort(files, count, sizeof(FileInfo), compare_size);
    }
    // (아무 옵션도 없으면 기본적으로 이름순으로 readdir이 읽은 순서대로 출력됨)

    // 실제 출력 부분
    // -R 옵션 사용 시, 어느 디렉터리에 대한 출력인지 명시해준다.
    if (recursive) {
        printf("\n%s:\n", path);
    }

    for (int i = 0; i < count; i++) {
        if (long_format) { // -l 옵션: 긴 형식으로 출력
            print_long_format(&files[i]);
        } 
        else { // 기본 출력
            printf("%s\t", files[i].name);
        }
    }
    // -l 옵션이 아닐 때만 마지막에 줄바꿈을 추가해준다.
    if (!long_format) {
        printf("\n");
    }

    // -R (재귀) 옵션 처리
    if (recursive) {
        for (int i = 0; i < count; i++) {
            // 현재 항목이 디렉터리이고, 자기 자신('.')이나 부모('..')가 아닐 경우
            if (S_ISDIR(files[i].st.st_mode) &&
                strcmp(files[i].name, ".") != 0 &&
                strcmp(files[i].name, "..") != 0) {
                
                // 해당 하위 디렉터리에 대한 전체 경로 생성
                char sub_path[512];
                snprintf(sub_path, sizeof(sub_path), "%s/%s", path, files[i].name);
                
                // 자기 자신(list_directory 함수)을 다시 호출하여 재귀적으로 탐색
                list_directory(sub_path, show_all, long_format, recursive, sort_time, sort_size);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    // 옵션 상태를 저장할 플래그 변수 초기화
    int show_all = 0, long_format = 0, recursive = 0;
    int sort_time = 0, sort_size = 0;
    int opt;

    // getopt를 사용하여 명령줄 옵션을 파싱
    // "alRtS"는 -a, -l, -R, -t, -S 옵션을 허용한다는 의미
    while ((opt = getopt(argc, argv, "alRtS")) != -1) {
        switch (opt) {
            case 'a': show_all = 1; break;
            case 'l': long_format = 1; break;
            case 'R': recursive = 1; break;
            case 't': sort_time = 1; sort_size = 0; break; // t와 S는 동시에 적용될 수 없으므로 다른 쪽을 해제
            case 'S': sort_size = 1; sort_time = 0; break;
            default: // 인식할 수 없는 옵션일 경우
                fprintf(stderr, "사용법: %s [-a] [-l] [-R] [-t] [-S] [파일 또는 디렉터리]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // 기본 대상은 현재 디렉터리(".")
    const char *target_path = ".";
    // 옵션 파싱 후 남은 인자가 있다면 그것을 대상 경로로 사용
    if (optind < argc) {
        target_path = argv[optind];
    }

    // 핵심 로직 함수 호출
    list_directory(target_path, show_all, long_format, recursive, sort_time, sort_size);
    
    return 0;
}