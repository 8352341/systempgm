#include <stdio.h>
#include <unistd.h> // for gethostname
#include <limits.h> // for HOST_NAME_MAX

int main() {
    char host_name[HOST_NAME_MAX + 1];//null문자를 저장하기 위한 공간 생성

    // gethostname 시스템 호출을 사용하여 호스트 이름을 얻어옴
    if (gethostname(host_name, sizeof(host_name)) == -1) {
        perror("gethostname"); // 에러 발생 시 메시지 출력
        return 1;
    }

    printf("%s\n", host_name);
    return 0;
}