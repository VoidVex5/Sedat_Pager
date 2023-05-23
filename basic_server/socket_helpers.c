#include "socket_helpers.h"

void err_n_die(const char* fmt, ...) {
	int errno_save;
	va_list ap;
	errno_save = errno;

	va_start(ap, fmt);
	vfprintf(stdout, fmt, ap);
	fprintf(stdout, "\n");
	fflush(stdout);
	
	if (errno_save != 0) {
		fprintf(stdout, "(errno = %d) : %s", errno_save, strerror(errno_save));
		fprintf(stdout, "\n");
		fflush(stdout);
	}
	va_end(ap);

	exit(1);
}
	
char* bin2hex(const char* str, size_t len) {

    char* hex_str = malloc(sizeof(char) * (len * 3) + 1);

    const char* hex_dig = "0123456789ABCDEF"; 

    int i = 0;
    int cnt = 0;
    while (i < len) {
        hex_str[cnt++] = hex_dig[(*(str + i) % 0x10)];
        hex_str[cnt++] = hex_dig[*(str + i) / 0x10];
        hex_str[cnt++] = ' ';
        i += 1;
    }
	hex_str[cnt] = '\0';
    return hex_str;                                                                                                                                     
}
