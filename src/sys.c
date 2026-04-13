#include "sys.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "string.h"

void sys_init(void) {
    int fl = fcntl(0, F_GETFL, 0);
    if (fl >= 0)
        fcntl(0, F_SETFL, fl | O_NONBLOCK);
}

void print(const char *s) {
    if (!s)
        return;
    size_t n = my_strlen(s);
    if (n == 0)
        return;
    (void)write(1, s, n);
}

void print_n(const char *s, size_t n) {
    if (!s || n == 0)
        return;
    (void)write(1, s, n);
}

void print_uint(unsigned long n) {
    char buf[32];
    int i = 0;
    if (n == 0) {
        print("0");
        return;
    }
    while (n > 0 && i < (int)sizeof(buf)) {
        buf[i++] = (char)('0' + (n % 10UL));
        n /= 10UL;
    }
    while (i > 0)
        (void)write(1, &buf[--i], 1);
}

void print_int(long n) {
    if (n < 0) {
        print("-");
        print_uint((unsigned long)(-(n + 1)) + 1UL);
        return;
    }
    print_uint((unsigned long)n);
}

void print_hex(unsigned long n) {
    static const char *xdigits = "0123456789abcdef";
    char buf[20];
    int i = 0;
    unsigned long v = n;
    if (v == 0) {
        print("0x0");
        return;
    }
    while (v > 0 && i < (int)sizeof(buf)) {
        buf[i++] = xdigits[v & 0xFUL];
        v >>= 4;
    }
    print("0x");
    while (i > 0)
        (void)write(1, &buf[--i], 1);
}

int read_line(char *buf, int size) {
    int pos = 0;
    if (size <= 0)
        return 0;
    while (pos < size - 1) {
        char c;
        ssize_t r = read(0, &c, 1);
        if (r == 0)
            break;
        if (r < 0) {
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN)
                break;
            break;
        }
        if (c == '\n')
            break;
        if (c == '\r')
            continue;
        buf[pos++] = c;
    }
    buf[pos] = 0;
    return pos;
}
