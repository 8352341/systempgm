#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/statvfs.h> // statvfs 구조체 및 함수
#include <mntent.h>      // getmntent, setmntent, endmntent (마운트 정보)
#include <stdint.h>      // uint64_t 등 정확한 크기의 정수 타입

/**
 * @brief 바이트 단위 크기를 사람이 읽기 쉬운 K, M, G, T 단위로 변환하여 출력
 * @param bytes 변환할 크기 (바이트 단위)
 */
void print_human_readable(uint64_t bytes) {
    // 0 바이트는 특별 처리
    if (bytes == 0) {
        printf("%5s", "0");
        return;
    }
    const char *units[] = {"B", "K", "M", "G", "T", "P"};
    int i = 0;
    double size = bytes;

    // 크기가 1024 이상이면 단위 증가
    while (size >= 1024 && i < (sizeof(units)/sizeof(char*)-1) ) {
        size /= 1024;
        i++;
    }
    // 소수점 첫째 자리까지 5칸에 맞춰 우측 정렬하여 출력
    printf("%5.1f%s", size, units[i]);
}

// 메인 함수
int main(int argc, char *argv[]) {
    // 옵션 상태를 저장할 플래그 변수
    int human_readable = 0; // -h: 사람이 읽기 쉬운 단위로 표시
    int show_type = 0;      // -T: 파일 시스템 타입 표시
    int show_all = 0;       // -a: 모든 파일 시스템 표시 (pseudo 포함)

    int opt;
    // getopt를 사용하여 명령줄 옵션을 파싱
    while ((opt = getopt(argc, argv, "hTa")) != -1) {
        switch (opt) {
            case 'h': human_readable = 1; break;
            case 'T': show_type = 1;      break;
            case 'a': show_all = 1;       break;
            default:
                fprintf(stderr, "사용법: %s [-hTa]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // /proc/mounts 파일을 열어 마운트된 파일 시스템 목록을 가져온다.
    // 이 파일은 현재 시스템에 마운트된 모든 파일 시스템의 정보를 담고 있다.
    FILE *mount_table = setmntent("/proc/mounts", "r");
    if (!mount_table) {
        perror("setmntent /proc/mounts");
        exit(EXIT_FAILURE);
    }

    struct mntent *mount_entry; // 마운트 항목 하나를 가리키는 포인터
    
    // 헤더 출력
    printf("%-20s", "Filesystem");
    if (show_type) printf(" %-10s", "Type");
    printf(" %10s %10s %10s %5s %-s\n", "Size", "Used", "Avail", "Use%", "Mounted on");

    // getmntent()를 반복 호출하여 마운트된 모든 파일 시스템을 순회
    while ((mount_entry = getmntent(mount_table)) != NULL) {
        
        // --- -a 옵션 필터링 ---
        // -a 옵션이 없고, 가상/임시 파일 시스템(pseudo-filesystem)이면 건너뛴다.
        // 보통 장치 이름이 'none'이거나, 타입이 'tmpfs', 'proc' 등인 경우가 해당된다.
        if (!show_all && (
            strcmp(mount_entry->mnt_type, "proc") == 0 ||
            strcmp(mount_entry->mnt_type, "sysfs") == 0 ||
            strcmp(mount_entry->mnt_type, "devtmpfs") == 0 ||
            strcmp(mount_entry->mnt_type, "tmpfs") == 0 ||
            strcmp(mount_entry->mnt_fsname, "none") == 0
        )) {
            continue;
        }

        struct statvfs fs_stats; // 파일 시스템 통계 정보를 담을 구조체

        // statvfs() 시스템 호출로 마운트된 디렉터리의 통계 정보를 가져온다.
        if (statvfs(mount_entry->mnt_dir, &fs_stats) != 0) {
            // 실패 시 해당 항목은 건너뛴다 (예: 접근 권한 없는 경우)
            continue;
        }

        // --- 용량 계산 ---
        // f_frsize: 기본 블록 크기
        // f_blocks: 전체 블록 수
        // f_bfree: 남은 블록 수 (전체)
        // f_bavail: 남은 블록 수 (root가 아닌 일반 사용자가 사용 가능한)
        uint64_t block_size = fs_stats.f_frsize;
        uint64_t total_size = fs_stats.f_blocks * block_size;
        uint64_t free_size = fs_stats.f_bfree * block_size;
        uint64_t avail_size = fs_stats.f_bavail * block_size;
        uint64_t used_size = total_size - free_size;
        int use_percent = (total_size == 0) ? 0 : (int)(((double)used_size / (used_size + avail_size)) * 100 + 0.5);

        // --- 최종 출력 ---
        printf("%-20s", mount_entry->mnt_fsname);
        
        if (show_type) {
            printf(" %-10s", mount_entry->mnt_type);
        }

        if (human_readable) { // -h 옵션: K, M, G 단위로 출력
            print_human_readable(total_size);
            print_human_readable(used_size);
            print_human_readable(avail_size);
        } else { // 기본: 1K 블록 단위로 출력 (df 기본 동작)
            printf(" %10lu", total_size / 1024);
            printf(" %10lu", used_size / 1024);
            printf(" %10lu", avail_size / 1024);
        }

        printf(" %4d%% %-s\n", use_percent, mount_entry->mnt_dir);
    }

    // 열었던 마운트 테이블 파일을 닫는다.
    endmntent(mount_table);
    return 0;
}