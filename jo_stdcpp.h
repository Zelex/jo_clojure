// Written by Jon Olick
// This is free and unencumbered software released into the public domain.
//
// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non-commercial, and by any
// means.
//
// In jurisdictions that recognize copyright laws, the author or authors
// of this software dedicate any and all copyright interest in the
// software to the public domain. We make this dedication for the benefit
// of the public at large and to the detriment of our heirs and
// successors. We intend this dedication to be an overt act of
// relinquishment in perpetuity of all present and future rights to this
// software under copyright law.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
// For more information, please refer to <http://unlicense.org/>

#ifndef JO_STDCPP
#define JO_STDCPP
#pragma once

// Not in any way intended to be fastest implementation, just simple bare minimum we need to compile tinyexr

#include <string.h>
//#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <atomic>

#ifdef _WIN32
#include <conio.h>
#include <direct.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#define jo_strdup _strdup
#define jo_chdir _chdir
#pragma warning(push)
#pragma warning(disable : 4345)
#elif defined(__APPLE__)
#include <mach/mach_time.h>
#include <unistd.h>
#include <termios.h>
#define jo_strdup strdup
#define jo_chdir chdir
#else
#include <unistd.h>
#include <termios.h>
#define jo_strdup strdup
#define jo_chdir chdir
#endif

#ifdef _WIN32
#include <mutex>
#define jo_mutex std::mutex
#else
#include <pthread.h>
class jo_mutex {
    pthread_mutex_t mutex;
public:
    jo_mutex() { pthread_mutex_init(&mutex, nullptr); }
    ~jo_mutex() { pthread_mutex_destroy(&mutex); }
    void lock() { pthread_mutex_lock(&mutex); }
    void unlock() { pthread_mutex_unlock(&mutex); }
};
#endif

class jo_lock_guard {
    jo_mutex& mutex;
public:
    jo_lock_guard(jo_mutex& mutex) : mutex(mutex) { mutex.lock(); }
    ~jo_lock_guard() { mutex.unlock(); }
};

#ifdef _WIN32
static int jo_setenv(const char *name, const char *value, int overwrite)
{
    int errcode = 0;
    if(!overwrite) {
        size_t envsize = 0;
        errcode = getenv_s(&envsize, NULL, 0, name);
        if(errcode || envsize) return errcode;
    }
    return _putenv_s(name, value);
}
#else
#define jo_setenv setenv
#endif

static bool jo_file_exists(const char *path)
{
    FILE *f = fopen(path, "r");
    if(f) {
        fclose(f);
        return true;
    }
    return false;
}

static size_t jo_file_size(const char *path)
{
    FILE *f = fopen(path, "r");
    if(!f) return 0;
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fclose(f);
    return size;
}

static bool jo_file_readable(const char *path)
{
    FILE *f = fopen(path, "r");
    if(!f) return false;
    fclose(f);
    return true;
}

// tests to see if it can write to a file without truncating it
static bool jo_file_writable(const char *path)
{
    FILE *f = fopen(path, "r+");
    if(!f) return false;
    fclose(f);
    return true;
}

// checks to see if file can be executed (cross-platform)
static bool jo_file_executable(const char *path)
{
#ifdef _WIN32
    // check stat to see if it has executable bit set
    struct _stat s;
    if(_stat(path, &s) != 0) return false;
    return s.st_mode & _S_IEXEC;
#else
    // check access to see if it can be executed
    return access(path, X_OK) == 0;
#endif
}

static bool jo_file_empty(const char *path)
{
    FILE *f = fopen(path, "r");
    if(!f) return true;
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fclose(f);
    return size == 0;
}

// copy file 16k at a time
static bool jo_file_copy(const char *src, const char *dst)
{
    FILE *fsrc = fopen(src, "rb");
    if(!fsrc) return false;
    FILE *fdst = fopen(dst, "wb");
    if(!fdst) {
        fclose(fsrc);
        return false;
    }
    char buf[16384];
    size_t nread = 0;
    while((nread = fread(buf, 1, sizeof(buf), fsrc))) {
        fwrite(buf, 1, nread, fdst);
    }
    fclose(fsrc);
    fclose(fdst);
    return true;
}

static int jo_kbhit()
{
#ifdef _WIN32
    return _kbhit();
#else
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(0, &fds); //STDIN_FILENO is 0
    select(1, &fds, NULL, NULL, &tv);
    return FD_ISSET(0, &fds);
#endif
}

static int jo_getch()
{
#ifdef _WIN32
    return _getch();
#else
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
#endif
}

#ifndef _WIN32
#include <dirent.h>
static bool jo_dir_exists(const char *path)
{
    DIR *d = opendir(path);
    if(d) {
        closedir(d);
        return true;
    }
    return false;
}
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static bool jo_dir_exists(const char *path)
{
    DWORD attrib = GetFileAttributes(path);
    return (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY));
}
#undef min
#undef max
#endif

// always adds a 0 terminator
static void *jo_slurp_file(const char *path, size_t *size)
{
    FILE *f = fopen(path, "rb");
    if(!f) return NULL;
    fseek(f, 0, SEEK_END);
    size_t fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    void *data = malloc(fsize + 1);
    if(!data) {
        fclose(f);
        return NULL;
    }
    size_t read = fread(data, 1, fsize, f);
    fclose(f);
    if(read != fsize) {
        free(data);
        return NULL;
    }
    ((char *)data)[fsize] = 0;
    if(size) *size = fsize;
    return data;
}

static char *jo_slurp_file(const char *path)
{
    size_t size = 0;
    void *data = jo_slurp_file(path, &size);
    if(!data) return NULL;
    char *str = (char *)data;
    str[size] = 0;
    return str;
}

static int jo_spit_file(const char *path, const void *data, size_t size)
{
    FILE *f = fopen(path, "wb");
    if(!f) return 1;
    size_t written = fwrite(data, 1, size, f);
    fclose(f);
    return written != size;
}

static int jo_spit_file(const char *path, const char *data)
{
    return jo_spit_file(path, data, strlen(data));
}

static int jo_tolower(int c)
{
    if(c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

static int jo_toupper(int c)
{
    if(c >= 'a' && c <= 'z') return c - 32;
    return c;
}

static int jo_isspace(int c)
{
    return (c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r');
}

static int jo_isletter(int c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

// returns a pointer to the last occurrence of needle in haystack
// or NULL if needle is not found
static const char *jo_strrstr(const char *haystack, const char *needle)
{
    const char *h = haystack + strlen(haystack);
    const char *n = needle + strlen(needle);
    while(h > haystack) {
        const char *h2 = h - 1;
        const char *n2 = n - 1;
        while(n2 > needle && *h2 == *n2) h2--, n2--;
        if(n2 == needle) return h2;
        h--;
    }
    return NULL;
}

void jo_sleep(float seconds) {
#ifdef _WIN32
    Sleep((int)(seconds * 1000));
#else
    usleep((int)(seconds * 1000000));
#endif
}

// yield
static void jo_yield()
{
#ifdef _WIN32
    Sleep(0);
#else
    sched_yield();
#endif
}

FILE *jo_fmemopen(void *buf, size_t size, const char *mode)
{
    if (!size) {
        return 0;
    }

#ifdef _WIN32
    int fd;
    FILE *fp;
    char tp[MAX_PATH - 13];
    char fn[MAX_PATH + 1];
    int *pfd = &fd;
    int retner = -1;
    char tfname[] = "MemTF_";
    if (!GetTempPathA(sizeof(tp), tp) || !GetTempFileNameA(tp, tfname, 0, fn)) {
        return NULL;
    }
    retner = _sopen_s(pfd, fn, _O_CREAT | _O_SHORT_LIVED | _O_TEMPORARY | _O_RDWR | _O_BINARY | _O_NOINHERIT, _SH_DENYRW, _S_IREAD | _S_IWRITE);
    if (retner != 0 || fd == -1) {
        return NULL;
    }
    fp = _fdopen(fd, "wb+");
    if (!fp) {
        _close(fd);
        return NULL;
    }
    /*File descriptors passed into _fdopen are owned by the returned FILE * stream.
      If _fdopen is successful, do not call _close on the file descriptor.
      Calling fclose on the returned FILE * also closes the file descriptor.
    */
    fwrite(buf, size, 1, fp);
    rewind(fp);
    return fp;
#else
    return fmemopen(buf, size, mode);
#endif
}


// 
// Simple C++std replacements...
//

#define JO_M_PI 3.14159265358979323846
#define JO_M_PI_2 1.57079632679489661923
#define JO_M_PI_4 0.785398163397448309616
#define JO_M_1_PI 0.318309886183790671538
#define JO_M_2_PI 0.636619772367581343076
#define JO_M_2_SQRTPI 1.12837916709551257390
#define JO_M_SQRT2 1.41421356237309504880
#define JO_M_SQRT1_2 0.707106781186547524401
#define JO_M_LOG2E 1.44269504088896340736
#define JO_M_LOG10E 0.434294481903251827651
#define JO_M_LN2 0.693147180559945309417
#define JO_M_LN10 2.30258509299404568402
#define JO_M_E 2.7182818284590452354


template<typename T> struct jo_numeric_limits;

template<> struct jo_numeric_limits<int> {
    static int max() { return INT_MAX; }
    static int min() { return INT_MIN; }
};

#define jo_endl ("\n")
#define jo_npos ((size_t)(-1))

template <class F> struct lambda_traits : lambda_traits<decltype(&F::operator())> { };
template <typename F, typename R, typename... Args>
struct lambda_traits<R(F::*)(Args...)> : lambda_traits<R(F::*)(Args...) const> { };
template <class F, class R, class... Args>
struct lambda_traits<R(F::*)(Args...) const> {
    using pointer = typename std::add_pointer<R(Args...)>::type;
    static pointer jo_cify(F&& f) {
        static F fn = std::forward<F>(f);
        return [](Args... args) {
            return fn(std::forward<Args>(args)...);
        };
    }
};

template <class F>
inline typename lambda_traits<F>::pointer jo_cify(F&& f) {
    return lambda_traits<F>::jo_cify(std::forward<F>(f));
}

// jo_pair is a simple pair of values
template<typename T1, typename T2>
struct jo_pair {
    T1 first;
    T2 second;

    jo_pair() : first(), second() {}
    jo_pair(const T1 &a, const T2 &b) : first(a), second(b) {}

    bool operator==(const jo_pair<T1, T2> &other) const {
        return first == other.first && second == other.second;
    }
    bool operator!=(const jo_pair<T1, T2> &other) const {
        return !(*this == other);
    }
    bool operator<(const jo_pair<T1, T2> &other) const {
        if(first < other.first) return true;
        if(first > other.first) return false;
        return second < other.second;
    }
};

// jo_triple
template<typename T1, typename T2, typename T3>
struct jo_triple {
    T1 first;
    T2 second;
    T3 third;

    jo_triple() : first(), second(), third() {}
    jo_triple(const T1 &a, const T2 &b, const T3 &c) : first(a), second(b), third(c) {}

    bool operator==(const jo_triple<T1, T2, T3> &other) const {
        return first == other.first && second == other.second && third == other.third;
    }
    bool operator!=(const jo_triple<T1, T2, T3> &other) const {
        return !(*this == other);
    }
    bool operator<(const jo_triple<T1, T2, T3> &other) const {
        if(first < other.first) return true;
        if(first > other.first) return false;
        if(second < other.second) return true;
        if(second > other.second) return false;
        return third < other.third;
    }

};

// jo_swap, std::swap alternative
template<typename T>
inline void jo_swap(T &a, T &b) {
    T tmp = a;
    a = b;
    b = tmp;
}

// jo_random_int
inline int jo_random_int(int min, int max) {
    return min + (rand() % (max - min + 1));
}

inline int jo_random_int(int max) {
    return jo_random_int(0, max);
}

inline int jo_random_int() {
    return jo_random_int(0, RAND_MAX);
}

inline float jo_random_float() {
    return (float)rand() / (float)RAND_MAX;
}

// jo_random_shuffle
template<typename T>
void jo_random_shuffle(T *begin, T *end) {
    for(T *i = begin; i != end; ++i) {
        T *j = begin + jo_random_int(end - begin - 1);
        jo_swap(*i, *j);
    }
}

// count leading zeros
inline int jo_clz32(int x) {
#ifdef _WIN32
    unsigned long r = 0;
    _BitScanReverse(&r, x);
    return 31 - r;
#else
    return __builtin_clz(x);
#endif
}

inline long long jo_clz64(long long x) {
#ifdef _WIN32
    unsigned long r = 0;
    _BitScanReverse64(&r, x);
    return 63 - r;
#else
    return __builtin_clzll(x);
#endif
}

struct jo_string {
    char *str;
    
    jo_string() { str = jo_strdup(""); }
    jo_string(const char *ss) { str = jo_strdup(ss); }
    jo_string(char c) { str = jo_strdup(" "); str[0] = c; }
    jo_string(const jo_string *other) { str = jo_strdup(other->str); }
    jo_string(const jo_string &other) { str = jo_strdup(other.str); }
    jo_string(jo_string &&other) {
        str = other.str;
        other.str = NULL;
    }
    jo_string(const char *a, size_t size) {
        str = (char*)malloc(size+1);
        memcpy(str, a, size);
        str[size] = 0;
    }
    jo_string(const char *a, const char *b) {
        ptrdiff_t size = b - a;
        str = (char*)malloc(size+1);
        memcpy(str, a, size);
        str[size] = 0;
    }

    ~jo_string() {
        free(str);
        str = 0;
    }

    const char *c_str() const { return str; };
    int compare(const jo_string &other) { return strcmp(str, other.str); }
    size_t size() const { return strlen(str); }
    size_t length() const { return strlen(str); }

    jo_string &operator=(const char *s) {
        char *tmp = jo_strdup(s);
        free(str);
        str = tmp;
        return *this;
    }

    jo_string &operator=(const jo_string &s) {
        char *tmp = jo_strdup(s.str);
        free(str);
        str = tmp;
        return *this;
    }

    jo_string &operator+=(const char *s) {
        size_t l0 = strlen(str);
        size_t l1 = strlen(s);
        char *new_str = (char*)realloc(str, l0 + l1 + 1);
        if(!new_str) {
            // malloc failed!
            return *this;
        }
        str = new_str;
        memcpy(str+l0, s, l1);
        str[l0+l1] = 0;
        return *this;
    }
    jo_string &operator+=(const jo_string &s) { *this += s.c_str(); return *this; }

    jo_string &operator+=(char c) {
        size_t l0 = strlen(str);
        char *new_str = (char*)realloc(str, l0 + 2);
        if(!new_str) {
            // malloc failed!
            return *this;
        }
        str = new_str;
        str[l0] = c;
        str[l0+1] = 0;
        return *this;
    }

    size_t find_last_of(char c) const {
        char *tmp = strrchr(str, c);
        if(!tmp) return jo_npos;
        return (size_t)(tmp - str);
    }

    size_t find_last_of(const char *s) const {
        const char *tmp = jo_strrstr(str, s);
        if(!tmp) return jo_npos;
        return (size_t)(tmp - str);
    }

    jo_string &erase(size_t n) {
        str[n] = 0;
        return *this;
    }

    jo_string &erase(size_t n, size_t m) {
        size_t l = strlen(str);
        if(n >= l) return *this;
        if(m > l) m = l;
        if(m <= n) {
            str[n] = 0;
            return *this;
        }
        memmove(str+n, str+m, l-m+1);
        return *this;
    }

    jo_string &insert(size_t n, const char *s) {
        size_t l0 = strlen(str);
        size_t l1 = strlen(s);
        char *new_str = (char*)realloc(str, l0 + l1 + 1);
        if(!new_str) {
            // malloc failed!
            return *this;
        }
        str = new_str;
        memmove(str+n+l1, str+n, l0-n+1);
        memcpy(str+n, s, l1);
        return *this;
    }

    jo_string substr(size_t pos = 0, size_t len = jo_npos) const {
        if(len == jo_npos) {
            len = size() - pos;
        }
        if(pos >= size()) return jo_string();
        return jo_string(str + pos, len);
    }

    size_t find(char c, size_t pos = 0) const {
        const char *tmp = strchr(str+pos, c);
        if(!tmp) return jo_npos;
        return (size_t)(tmp - str);
    }
    size_t find(const char *s, size_t pos = 0) const {
        const char *tmp = strstr(str+pos, s);
        if(!tmp) return jo_npos;
        return (size_t)(tmp - str);
    }
    size_t find(const jo_string &s, size_t pos = 0) const { return find(s.c_str(), pos); }

    static jo_string format(const char *fmt, ...) {
        va_list args;
        va_start(args, fmt);
        int len = vsnprintf(0, 0, fmt, args);
        va_end(args);
        if(len < 0) {
            // error
            return jo_string();
        }
        char *tmp = (char*)malloc(len+1);
        if(!tmp) {
            // malloc failed!
            return jo_string();
        }
        va_start(args, fmt);
        vsnprintf(tmp, len+1, fmt, args);
        va_end(args);
        return jo_string(tmp);
    }

    int compare(const char *s) const { return strcmp(str, s); }

    jo_string &lower() {
        for(size_t i = 0; i < size(); i++) {
            str[i] = jo_tolower(str[i]);
        }
        return *this;
    }

    jo_string &upper() {
        for(size_t i = 0; i < size(); i++) {
            str[i] = jo_toupper(str[i]);
        }
        return *this;
    }

    jo_string &reverse() {
        char *tmp = str;
        char *end = str + size();
        while(tmp < end) {
            char c = *tmp;
            *tmp = *(end-1);
            *(end-1) = c;
            tmp++;
            end--;
        }
        return *this;
    }

    // True if s empty or contains only whitespace.
    bool empty() const {
        for(size_t i = 0; i < size(); i++) {
            if(!jo_isspace(str[i])) return false;
        }
        return true;
    }

    // Converts first character of the string to upper-case, all other characters to lower-case.
    jo_string &capitalize() {
        if(size() == 0) return *this;
        str[0] = jo_toupper(str[0]);
        for(size_t i = 1; i < size(); i++) {
            str[i] = jo_tolower(str[i]);
        }
        return *this;
    }

    bool ends_with(const char *s) const {
        size_t l1 = strlen(s);
        size_t l2 = size();
        if(l1 > l2) return false;
        return strcmp(str+l2-l1, s) == 0;
    }

    bool starts_with(const char *s) const {
        size_t l1 = strlen(s);
        size_t l2 = size();
        if(l1 > l2) return false;
        return strncmp(str, s, l1) == 0;
    }

    bool includes(const char *s) const {
        return find(s) != jo_npos;
    }

    int index_of(char c) const {
        for(size_t i = 0; i < size(); i++) {
            if(str[i] == c) return (int)i;
        }
        return -1;
    }

    int index_of(const char *s) const {
        size_t pos = find(s);
        if(pos == jo_npos) return -1;
        return (int)pos;
    }

    int last_index_of(char c) const {
        for(int i = (int)size()-1; i >= 0; i--) {
            if(str[i] == c) return i;
        }
        return -1;
    }

    int last_index_of(const char *s) const {
        size_t pos = find_last_of(s);
        if(pos == jo_npos) return -1;
        return (int)pos;
    }

    jo_string &trim() {
        size_t start = 0;
        while(start < size() && jo_isspace(str[start])) {
            start++;
        }
        size_t end = size()-1;
        while(end > start && jo_isspace(str[end])) {
            end--;
        }
        if(start > 0 || end < size()-1) {
            char *tmp = (char*)malloc(end-start+2);
            if(!tmp) {
                // malloc failed!
                return *this;
            }
            memcpy(tmp, str+start, end-start+1);
            tmp[end-start+1] = 0;
            free(str);
            str = tmp;
        }
        return *this;
    }

    jo_string &ltrim() {
        size_t start = 0;
        while(start < size() && jo_isspace(str[start])) {
            start++;
        }
        if(start > 0) {
            char *tmp = (char*)malloc(size()-start+1);
            if(!tmp) {
                // malloc failed!
                return *this;
            }
            memcpy(tmp, str+start, size()-start);
            tmp[size()-start] = 0;
            free(str);
            str = tmp;
        }
        return *this;
    }

    jo_string &rtrim() {
        size_t end = size()-1;
        while(end > 0 && jo_isspace(str[end])) {
            end--;
        }
        if(end < size()-1) {
            char *tmp = (char*)malloc(end+2);
            if(!tmp) {
                // malloc failed!
                return *this;
            }
            memcpy(tmp, str, end+1);
            tmp[end+1] = 0;
            free(str);
            str = tmp;
        }
        return *this;
    }

    jo_string &chomp() {
        if(size() == 0) return *this;
        if(str[size()-1] == '\n') {
            str[size()-1] = 0;
            if(size() > 0 && str[size()-1] == '\r') {
                str[size()-1] = 0;
            }
        }
        return *this;
    }

    jo_string &replace(const char *s, const char *r) {
        size_t pos = 0;
        while(pos < size()) {
            size_t pos2 = find(s, pos);
            if(pos2 == jo_npos) {
                break;
            }
            erase(pos2, strlen(s));
            insert(pos2, r);
            pos = pos2 + strlen(r);
        }
        return *this;
    }

    jo_string &replace_first(const char *s, const char *r) {
        size_t pos = find(s);
        if(pos == jo_npos) {
            return *this;
        }
        erase(pos, strlen(s));
        insert(pos, r);
        return *this;
    }

    jo_string &take(size_t n) {
        size_t l = size();
        if(n > l) {
            n = l;
        }
        char *tmp = (char*)malloc(n+1);
        if(!tmp) {
            // malloc failed!
            return *this;
        }
        memcpy(tmp, str, n);
        tmp[n] = 0;
        free(str);
        str = tmp;
        return *this;
    }

    jo_string &drop(size_t n) {
        size_t l = size();
        if(n > l) {
            n = l;
        }
        char *tmp = (char*)malloc(l-n+1);
        if(!tmp) {
            // malloc failed!
            return *this;
        }
        memcpy(tmp, str+n, l-n);
        tmp[l-n] = 0;
        free(str);
        str = tmp;
        return *this;
    }

    // iterator
    class iterator {
    public:
        iterator(const char *s) : str(s) {
        }
        char operator*() const {
            return *str;
        }
        iterator &operator++() {
            str++;
            return *this;
        }
        iterator operator++(int) {
            iterator tmp = *this;
            str++;
            return tmp;
        }
        bool operator==(const iterator &other) const {
            return str == other.str;
        }
        bool operator!=(const iterator &other) const {
            return str != other.str;
        }
    private:
        const char *str;
    };

    iterator begin() const {
        return iterator(str);
    }

    iterator end() const {
        return iterator(str+size());
    }




};

static inline jo_string operator+(const jo_string &lhs, const jo_string &rhs) { jo_string ret(lhs); ret += rhs; return ret; }
static inline jo_string operator+(const jo_string &lhs, const char *rhs) { jo_string ret(lhs); ret += rhs; return ret; }
static inline jo_string operator+(const char *lhs, const jo_string &rhs) { jo_string ret(lhs); ret += rhs; return ret; }
static inline jo_string operator+(const jo_string &lhs, char rhs) { jo_string ret(lhs); ret += rhs; return ret; }
static inline jo_string operator+(char lhs, const jo_string &rhs) { jo_string ret(lhs); ret += rhs; return ret; }
static inline bool operator==(const jo_string &lhs, const jo_string &rhs) { return !strcmp(lhs.c_str(), rhs.c_str()); }
static inline bool operator==(const char *lhs, const jo_string &rhs) { return !strcmp(lhs, rhs.c_str()); }
static inline bool operator==(const jo_string &lhs, const char *rhs) { return !strcmp(lhs.c_str(), rhs); }
static inline bool operator!=(const jo_string &lhs, const jo_string &rhs) { return !!strcmp(lhs.c_str(), rhs.c_str()); }
static inline bool operator!=(const char *lhs, const jo_string &rhs) { return !!strcmp(lhs, rhs.c_str()); }
static inline bool operator!=(const jo_string &lhs, const char *rhs) { return !!strcmp(lhs.c_str(), rhs); }
static inline bool operator<(const jo_string &lhs, const jo_string &rhs) { return strcmp(lhs.c_str(), rhs.c_str()) < 0; }
static inline bool operator<(const char *lhs, const jo_string &rhs) { return strcmp(lhs, rhs.c_str()) < 0; }
static inline bool operator<(const jo_string &lhs, const char *rhs) { return strcmp(lhs.c_str(), rhs) < 0; }
static inline bool operator<=(const jo_string &lhs, const jo_string &rhs) { return strcmp(lhs.c_str(), rhs.c_str()) <= 0; }
static inline bool operator<=(const char *lhs, const jo_string &rhs) { return strcmp(lhs, rhs.c_str()) <= 0; }
static inline bool operator<=(const jo_string &lhs, const char *rhs) { return strcmp(lhs.c_str(), rhs) <= 0; }
static inline bool operator>(const jo_string &lhs, const jo_string &rhs) { return strcmp(lhs.c_str(), rhs.c_str()) > 0; }
static inline bool operator>(const char *lhs, const jo_string &rhs) { return strcmp(lhs, rhs.c_str()) > 0; }
static inline bool operator>(const jo_string &lhs, const char *rhs) { return strcmp(lhs.c_str(), rhs) > 0; }
static inline bool operator>=(const jo_string &lhs, const jo_string &rhs) { return strcmp(lhs.c_str(), rhs.c_str()) >= 0; }
static inline bool operator>=(const char *lhs, const jo_string &rhs) { return strcmp(lhs, rhs.c_str()) >= 0; }
static inline bool operator>=(const jo_string &lhs, const char *rhs) { return strcmp(lhs.c_str(), rhs) >= 0; }

struct jo_stringstream {
    jo_string s;

    jo_string &str() { return s; }
    const jo_string &str() const { return s; }

    jo_stringstream &operator<<(int val) {
        char tmp[33];
#ifdef _WIN32
        sprintf_s(tmp, "%i", val);
#else
        sprintf(tmp, "%i", val);
#endif
        s += tmp;
        return *this;
    }

    jo_stringstream &operator<<(const char *val) {
        s += val;
        return *this;
    }
};

template<typename T> static inline T jo_min(T a, T b) { return a < b ? a : b; }
template<typename T> static inline T jo_max(T a, T b) { return a > b ? a : b; }

#if !defined(__PLACEMENT_NEW_INLINE) && !defined(_WIN32)
//inline void *operator new(size_t, void *p) { return p; }
#endif

template<typename T>
struct jo_vector {
    T *ptr;
    size_t ptr_size;
    size_t ptr_capacity;

    jo_vector() {
        ptr = 0;
        ptr_size = 0;
        ptr_capacity = 0;
    }

    jo_vector(size_t n) {
        ptr = 0;
        ptr_size = 0;
        ptr_capacity = 0;
        
        resize(n);
    }

    ~jo_vector() {
        resize(0);
    }

    size_t size() const { return ptr_size; }

    T *data() { return ptr; }
    const T *data() const { return ptr; }

    T *begin() { return ptr; }
    const T *begin() const { return ptr; }

    T *end() { return ptr + ptr_size; }
    const T *end() const { return ptr + ptr_size; }

    T *rbegin() { return ptr + ptr_size - 1; }
    const T *rbegin() const { return ptr + ptr_size - 1; }

    T *rend() { return ptr - 1; }
    const T *rend() const { return ptr - 1; }

    T &at(size_t i) { return ptr[i]; }
    const T &at(size_t i) const { return ptr[i]; }

    T &operator[](size_t i) { return ptr[i]; }
    const T &operator[](size_t i) const { return ptr[i]; }

    void resize(size_t n) {
        if(n < ptr_size) {
            // call dtors on stuff your destructing before moving memory...
            for(size_t i = n; i < ptr_size; ++i) {
                ptr[i].~T();
            }
        }

        if(n > ptr_capacity) {
            T *newptr = (T*)malloc(n*sizeof(T));
            if(!newptr) {
                // malloc failed!
                return;
            }
            if(ptr) {
                memcpy(newptr, ptr, ptr_size*sizeof(T));
                //memset(ptr, 0xFE, ptr_size*sizeof(T));
                free(ptr);
            }
            ptr = newptr;
            ptr_capacity = n;
        }

        if(n > ptr_size) {
            // in-place new on new data after moving memory to new location
            for(size_t i = ptr_size; i < n; ++i) {
                new(ptr+i) T(); // default c-tor
            }
        }
        
        if ( n == 0 )
        {
          if ( ptr ) free( ptr );
          ptr = 0;
          ptr_size = 0;
          ptr_capacity = 0;
        }

        ptr_size = n;
    }
    void clear() { resize(0); }

    void insert(const T *where, const T *what, size_t how_many) {
        if(how_many == 0) {
            return;
        }

        size_t n = ptr_size + how_many;
        ptrdiff_t where_at = where - ptr;

        // resize if necessary
        if(n > ptr_capacity) {
            size_t new_capacity = n + n/2; // grow by 50%
            T *newptr = (T*)malloc(new_capacity*sizeof(T));
            if(!newptr) {
                // malloc failed!
                return;
            }
            if(ptr) {
                memcpy(newptr, ptr, ptr_size*sizeof(T));
                //memset(ptr, 0xFE, ptr_size*sizeof(T));
                free(ptr);
            }
            ptr = newptr;
            ptr_capacity = new_capacity;
        }

        // simple case... add to end of array.
        if(where == ptr+ptr_size || where == 0) {
            for(size_t i = ptr_size; i < n; ++i) {
                new(ptr+i) T(what[i - ptr_size]);
            }
            ptr_size = n;
            return;
        }

        // insert begin/middle means we need to move the data past where to the right, and insert how_many there...
        memmove(ptr + where_at + how_many, ptr + where_at, sizeof(T)*(ptr_size - where_at));
        for(size_t i = where_at; i < where_at + how_many; ++i) {
            new(ptr+i) T(what[i - where_at]);
        }
        ptr_size = n;
    }

    void insert(const T *where, const T *what_begin, const T *what_end) {
        insert(where, what_begin, (size_t)(what_end - what_begin));
    }

    void push_back(const T& val) { insert(end(), &val, 1); }
    void push_front(const T& val) { insert(begin(), &val, 1); }
    T pop_back() { T ret = ptr[ptr_size-1]; resize(ptr_size-1); return ret; }
    T &back() { return ptr[ptr_size-1]; }
    const T &back() const { return ptr[ptr_size-1]; }

    T &front() { return ptr[0]; }
    const T &front() const { return ptr[0]; }

    void shrink_to_fit() {
        if(ptr_capacity == ptr_size) {
            return;
        }
        T *newptr = (T*)malloc(ptr_size*sizeof(T));
        if(!newptr) {
            // malloc failed!
            return;
        }
        memcpy(newptr, ptr, ptr_size*sizeof(T));
        //memset(ptr, 0xFE, ptr_capacity*sizeof(T)); // DEBUG
        free(ptr);
        ptr = newptr;
        ptr_capacity = ptr_size;
    }

    // reserve
    void reserve(size_t n) {
        if(n > ptr_capacity) {
            T *newptr = (T*)malloc(n*sizeof(T));
            if(!newptr) {
                // malloc failed!
                return;
            }

            if(ptr) {
                memcpy(newptr, ptr, ptr_size*sizeof(T));
                //memset(ptr, 0xFE, ptr_size*sizeof(T));
                free(ptr);
            }
            ptr = newptr;
            ptr_capacity = n;
        }
    }
    
};

// jo_pinned_vector
// has 64 exponentially pow2 sized buckets and a split of top = jo_clz64(index), bottom = index & (~0ull >> top)
// this is different than a jo_vector in that the elements never move and pointers can thus be relied upon as stable.
// In practice, bias index by (1<<k) to make the smallest alloc have that many elements.
// if when push_back we hit an empty bucket, we allocte it and add to it.
template<typename T, int k=5>
struct jo_pinned_vector {
    T *buckets[64-k+1];
    std::atomic<size_t> num_elements;
    jo_mutex grow_mutex;

    jo_pinned_vector() : buckets(), num_elements() {}

    ~jo_pinned_vector() {
        jo_lock_guard guard(grow_mutex);
        for(size_t i = 0; i < 64-k; ++i) {
            if(buckets[i]) {
                free(buckets[i]);
            }
        }
    }

    inline size_t bucket_size(size_t b) const { return 1ull << (64 - b); }
    inline int index_top(size_t i) const { return jo_clz64(i + (1<<k)) + 1; }
    inline size_t index_bottom(size_t i, int top) const { return i & (~0ull >> top); }

    void push_back(const T& val) {
        size_t this_elem = num_elements++;
        int top = index_top(this_elem);
        size_t bottom = index_bottom(this_elem, top);
        if(buckets[top] == 0) {
            jo_lock_guard guard(grow_mutex);
            if(buckets[top] == 0) {
                buckets[top] = (T*)malloc(sizeof(T)*bucket_size(top));
            }
        }
        if(std::is_pod<T>::value) {
            memcpy(buckets[top] + bottom, &val, sizeof(T));
        } else {
            new(buckets[top] + bottom) T(val);
        }
    }

    void pop_back() {
        num_elements--;
    }

    T &operator[](size_t i) {
        int top = index_top(i);
        return buckets[top][index_bottom(i, top)];
    }

    const T &operator[](size_t i) const {
        int top = index_top(i);
        return buckets[top][index_bottom(i, top)];
    }

    size_t size() const {
        return num_elements;
    }

    void resize(size_t n) {
        jo_lock_guard guard(grow_mutex);
        if(n == num_elements) {
            return;
        }
        if(n > num_elements) {
            // grow
            for(size_t i = num_elements; i < n; ++i) {
                size_t top = index_top(i);
                if(buckets[top] == 0) {
                    buckets[top] = (T*)malloc(sizeof(T)*bucket_size(top));
                }
                new(buckets[top] + index_bottom(i, top)) T();
            }
            num_elements = n;
        } else {
            // shrink
            if(!std::is_pod<T>::value) {
                for(size_t i = n; i < num_elements; ++i) {
                    (*this)[i].~T();
                }
            }
            num_elements = n;
        }
    }

    void clear() {
        resize(0);
    }

    void shrink_to_fit() {
        jo_lock_guard guard(grow_mutex);
        int top = index_top(num_elements);
        for(int i = top-1; i >= 0; --i) {
            if(buckets[i]) {
                jo_lock_guard guard(grow_mutex);
                if(buckets[i]) {
                    free(buckets[i]);
                    buckets[i] = 0;
                }
            }
        }
    }

    // iterator
    struct iterator {
        jo_pinned_vector<T, k> *vec;
        size_t index;
        iterator(jo_pinned_vector<T, k> *v, size_t i) : vec(v), index(i) {}
        iterator& operator++() {
            index++;
            return *this;
        }
        iterator operator++(int) {
            iterator ret = *this;
            index++;
            return ret;
        }
        bool operator==(const iterator& other) const {
            return index == other.index;
        }
        bool operator!=(const iterator& other) const {
            return index != other.index;
        }
        T& operator*() {
            return (*vec)[index];
        }
        const T& operator*() const {
            return (*vec)[index];
        }
    };

    iterator begin() { return iterator(this, 0); }
    iterator end() { return iterator(this, num_elements); }
    const iterator begin() const { return iterator(this, 0); }
    const iterator end() const { return iterator(this, num_elements); }

    T& back() { return (*this)[num_elements-1]; }
    const T& back() const { return (*this)[num_elements-1]; }
};

template<typename T, typename TT>
void jo_sift_down(T *begin, T *end, size_t root, TT cmp) {
    ptrdiff_t n = end - begin;
    ptrdiff_t parent = root;
    ptrdiff_t child = 2 * parent + 1;
    while (child < n) {
        if (child + 1 < n && cmp(begin[child], begin[child + 1])) {
            child++;
        }
        if (!cmp(begin[child], begin[parent])) {
            T tmp = begin[child];
            begin[child] = begin[parent];
            begin[parent] = tmp;

            parent = child;
            child = 2 * parent + 1;
        } else {
            break;
        }
    }
}

template<typename T, typename TT>
void jo_sift_up(T *begin, ptrdiff_t child, TT cmp) {
   	ptrdiff_t parent = (child - 1) >> 1;
	while (child > 0) {
		if(!cmp(begin[child], begin[parent])) {
			T tmp = begin[child];
			begin[child] = begin[parent];
			begin[parent] = tmp;

			child = parent;
			parent = (child - 1) >> 1;
		} else {
			break;
		}
	}
}

template<typename T, typename TT>
void jo_make_heap(T *begin, T *end, TT cmp) {
    if(begin >= end) {
        return;
    }
    ptrdiff_t n = end - begin;
    ptrdiff_t root = (n - 2) >> 1;
    for(; root >= 0; --root) {
        jo_sift_down(begin, end, root, cmp);
    }
}

template<typename T, typename TT>
void jo_pop_heap(T *begin, T *end, TT cmp) {
    if(begin >= end) {
        return;
    }
    T tmp = begin[0];
    begin[0] = end[-1];
    end[-1] = tmp;
    jo_sift_down(begin, end-1, 0, cmp);
}

template<typename T, typename TT>
void jo_push_heap(T *begin, T *end, TT cmp) {
    ptrdiff_t n = end - begin;
    if(n <= 1) {
        return; // nothing to do...
    }
    jo_sift_up(begin, n - 1, cmp);
}

template<typename T, typename TT>
void jo_sort_heap(T *begin, T *end, TT cmp) {
    jo_make_heap(begin, end, cmp);
    ptrdiff_t n = end - begin;
    for (ptrdiff_t i = n; i > 0; --i) {
        begin[0] = begin[i-1];
        jo_sift_down(begin, begin + i - 1, 0, cmp);
    }
}

template<typename T>
const T *jo_find(const T *begin, const T *end, const T &needle) {
    for(const T *ptr = begin; ptr != end; ++ptr) {
        if(*ptr == needle) return ptr;
    }
    return end;
}

template<typename T>
T *jo_find(T *begin, T *end, const T &needle) {
    for(T *ptr = begin; ptr != end; ++ptr) {
        if(*ptr == needle) return ptr;
    }
    return end;
}

template<typename T>
const T *jo_find_if(const T *begin, const T *end, bool (*pred)(const T&)) {
    for(const T *ptr = begin; ptr != end; ++ptr) {
        if(pred(*ptr)) return ptr;
    }
    return end;
}

template<typename T>
T *jo_find_if(T *begin, T *end, bool (*pred)(const T&)) {
    for(T *ptr = begin; ptr != end; ++ptr) {
        if(pred(*ptr)) return ptr;
    }
    return end;
}

template<typename T>
T *jo_lower_bound(T *begin, T *end, T &needle) {
    ptrdiff_t n = end - begin;
    ptrdiff_t first = 0;
    ptrdiff_t last = n;
    while (first < last) {
        ptrdiff_t mid = (first + last) / 2;
        if (needle < begin[mid]) {
            last = mid;
        } else {
            first = mid + 1;
        }
    }
    return begin + first;
}

template<typename T>
T *jo_upper_bound(T *begin, T *end, T &needle) {
    ptrdiff_t n = end - begin;
    ptrdiff_t first = 0;
    ptrdiff_t last = n;
    while (first < last) {
        ptrdiff_t mid = (first + last) / 2;
        if (needle <= begin[mid]) {
            last = mid;
        } else {
            first = mid + 1;
        }
    }
    return begin + first;
}

static inline double jo_time() {
#if defined(__APPLE__)
    static mach_timebase_info_data_t sTimebaseInfo;
    if (sTimebaseInfo.denom == 0) {
        (void) mach_timebase_info(&sTimebaseInfo);
    }
    uint64_t time = mach_absolute_time();
    return (double)time * sTimebaseInfo.numer / sTimebaseInfo.denom / 1000000000.0;
#elif defined(_WIN32)
    static LARGE_INTEGER sFrequency;
    static BOOL sInitialized = FALSE;
    if (!sInitialized) {
        sInitialized = QueryPerformanceFrequency(&sFrequency);
    }
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);
    return (double)time.QuadPart / sFrequency.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
#endif
}

// Maintains a linked list of blocks of objects which can be allocated from
// and deallocated to.
// Allocates single objects from blocks of 32 objects. Only frees memory when an entire block is free.
template<typename T>
struct jo_pool_alloc {
    struct block_item {
        // This points to the current block, can be looked up on deallocate
        struct block *b;
        T t;
    };
    struct block {
        block *next;
        block *prev;
        block_item items[32];
        int count;
    };

    block *first;
    block *last;
    size_t count;
    size_t capacity;
    jo_mutex mutex;

    jo_pool_alloc() : first(0), last(0), count(0), capacity(0), mutex() {}

    ~jo_pool_alloc() {
        jo_lock_guard lock(mutex);
        block *ptr = first;
        while(ptr) {
            block *next = ptr->next;
            free(ptr);
            ptr = next;
        }
    }

    T *allocate() {
        jo_lock_guard lock(mutex);
        if(!first) {
            first = (block *)malloc(sizeof(block));
            first->next = 0;
            first->prev = 0;
            first->count = 0;
            last = first;
            capacity = 32;
        }
        if(last->count == 32) {
            last->next = (block *)malloc(sizeof(block));
            last->next->prev = last;
            last->next->next = 0;
            last->next->count = 0;
            last = last->next;
            capacity += 32;
        }
        last->items[last->count].b = last;
        T *ptr = &last->items[last->count].t;
        last->count++;
        count++;
        return ptr;
    }

    void deallocate(T *ptr) {
        jo_lock_guard lock(mutex);
        block_item *item = (block_item *)(ptr - offsetof(block_item, t));
        block *b = item->b;
        b->count--;
        count--;
        if(b->count == 0) {
            if(b->prev) {
                b->prev->next = b->next;
            } else {
                first = b->next;
            }
            if(b->next) {
                b->next->prev = b->prev;
            } else {
                last = b->prev;
            }
            free(b);
            capacity -= 32;
        }
    }

    size_t size() {
        return count;
    }

    void clear() {
        jo_lock_guard lock(mutex);
        block *ptr = first;
        while(ptr) {
            block *next = ptr->next;
            free(ptr);
            ptr = next;
        }
        first = 0;
        last = 0;
        count = 0;
        capacity = 0;
    }
};

template<typename T>
class jo_set {
    T *ptr;
    size_t ptr_size;
    size_t ptr_capacity;
    bool (*cmp)(const T&, const T&);
public:
    jo_set(bool (*cmp)(const T&, const T&)) : ptr(0), ptr_size(0), ptr_capacity(0), cmp(cmp) {}
    jo_set(const jo_set<T> &other) : ptr(0), ptr_size(0), ptr_capacity(0), cmp(other.cmp) {
        *this = other;
    }
    jo_set<T> &operator=(const jo_set<T> &other) {
        if(this == &other) return *this;
        if(ptr_capacity < other.ptr_size) {
            ptr_capacity = other.ptr_size;
            ptr = (T*)realloc(ptr, sizeof(T)*ptr_capacity);
        }
        ptr_size = other.ptr_size;
        memcpy(ptr, other.ptr, sizeof(T)*ptr_size);
        return *this;
    }
    ~jo_set() {
        if(ptr) free(ptr);
    }

    void insert(const T &val) {
        if(ptr_size == ptr_capacity) {
            ptr_capacity = ptr_capacity ? ptr_capacity*2 : 4;
            ptr = (T*)realloc(ptr, sizeof(T)*ptr_capacity);
        }
        ptr[ptr_size++] = val;
        jo_sift_up(ptr, ptr_size-1, cmp);
    }

    void emplace(T &&val) {
        if(ptr_size == ptr_capacity) {
            ptr_capacity = ptr_capacity ? ptr_capacity*2 : 4;
            ptr = (T*)realloc(ptr, sizeof(T)*ptr_capacity);
        }
        ptr[ptr_size++] = val;
        jo_sift_up(ptr, ptr_size-1, cmp);
    }

    void erase(const T &val) {
        T *ptr = jo_find(this->ptr, this->ptr + ptr_size, val);
        if(ptr == this->ptr + ptr_size) return;
        ptr_size--;
        if(ptr != this->ptr + ptr_size) {
            T tmp = *ptr;
            *ptr = *(this->ptr + ptr_size);
            *(this->ptr + ptr_size) = tmp;
            jo_sift_down(this->ptr, this->ptr + ptr_size, (size_t)(ptr - this->ptr), cmp);
            jo_sift_up(this->ptr, ptr_size, cmp);
        }
    }

    void erase_if(bool (*pred)(const T&)) {
        T *ptr = jo_find_if(this->ptr, this->ptr + ptr_size, pred);
        if(ptr == this->ptr + ptr_size) return;
        ptr_size--;
        if(ptr != this->ptr + ptr_size) {
            T tmp = *ptr;
            *ptr = *(this->ptr + ptr_size);
            *(this->ptr + ptr_size) = tmp;
            jo_sift_down(this->ptr, this->ptr + ptr_size, (size_t)(ptr - this->ptr), cmp);
            jo_sift_up(this->ptr, ptr_size, cmp);
        }
    }

    void clear() { ptr_size = 0; }
    size_t size() const { return ptr_size; }
    size_t max_size() const { return ptr_capacity; }

    const T &operator[](size_t i) const { return ptr[i]; }
    T &operator[](size_t i) { return ptr[i]; }
    const T &at(size_t i) const { return ptr[i]; }
    T &at(size_t i) { return ptr[i]; }

    const T &front() const { return ptr[0]; }
    T &front() { return ptr[0]; }
    const T &back() const { return ptr[ptr_size-1]; }
    T &back() { return ptr[ptr_size-1]; }

    void sort() { jo_sort_heap(ptr, ptr + ptr_size, cmp); }

    void push(const T &val) { insert(val); }

    void pop_back() { erase(ptr[--ptr_size]); }
    void pop_front() { erase(ptr[0]); }

    void swap(jo_set<T> &other) {
        jo_swap(ptr, other.ptr);
        jo_swap(ptr_size, other.ptr_size);
        jo_swap(ptr_capacity, other.ptr_capacity);
        jo_swap(cmp, other.cmp);
    }

    void reserve(size_t n) {
        if(ptr_capacity < n) {
            ptr_capacity = n;
            ptr = (T*)realloc(ptr, sizeof(T)*ptr_capacity);
        }
    }

    void shrink_to_fit() {
        if(ptr_capacity > ptr_size) {
            ptr_capacity = ptr_size;
            ptr = (T*)realloc(ptr, sizeof(T)*ptr_capacity);
        }
    }

    void resize(size_t n) {
        if(ptr_capacity < n) {
            ptr_capacity = n;
            ptr = (T*)realloc(ptr, sizeof(T)*ptr_capacity);
        }
        ptr_size = n;
    }

    void resize(size_t n, const T &val) {
        if(ptr_capacity < n) {
            ptr_capacity = n;
            ptr = (T*)realloc(ptr, sizeof(T)*ptr_capacity);
        }
        if(ptr_size < n) {
            memset(ptr + ptr_size, 0, sizeof(T)*(n - ptr_size));
        }
        ptr_size = n;
    }

    void resize(size_t n, const T &val, bool (*cmp)(const T&, const T&)) {
        if(ptr_capacity < n) {
            ptr_capacity = n;
            ptr = (T*)realloc(ptr, sizeof(T)*ptr_capacity);
        }
        if(ptr_size < n) {
            memset(ptr + ptr_size, 0, sizeof(T)*(n - ptr_size));
        }
        ptr_size = n;
        jo_sort_heap(ptr, ptr + ptr_size, cmp);
    }

    T *lower_bound(const T &val) { return jo_lower_bound(ptr, ptr + ptr_size, val, cmp);}
    const T *lower_bound(const T &val) const { return jo_lower_bound(ptr, ptr + ptr_size, val, cmp);}

    T *upper_bound(const T &val) { return jo_upper_bound(ptr, ptr + ptr_size, val, cmp); }
    const T *upper_bound(const T &val) const { return jo_upper_bound(ptr, ptr + ptr_size, val, cmp); }
};

// jo_exception class
class jo_exception {
public:
    const char *msg;
    jo_exception(const char *msg) : msg(msg) {}
};

// std map like implementation
template<typename Key, typename Value> 
class jo_map {
    struct Node {
        Key key;
        Value value;
        Node *left, *right;
        Node(const Key &key, const Value &value) : key(key), value(value), left(nullptr), right(nullptr) {}
    };
    Node *root;
    size_t root_size;
    bool (*cmp)(const Key&, const Key&);
    void insert(Node *&root, const Key &key, const Value &value) {
        if(!root) {
            root = new Node(key, value);
            root_size++;
            return;
        }
        if(cmp(key, root->key)) {
            insert(root->left, key, value);
        } else if(cmp(root->key, key)) {
            insert(root->right, key, value);
        } else {
            root->value = value;
        }
    }

    void erase(Node *&root, const Key &key) {
        if(!root) return;
        if(cmp(key, root->key)) {
            erase(root->left, key);
        } else if(cmp(root->key, key)) {
            erase(root->right, key);
        } else {
            if(!root->left) {
                Node *tmp = root;
                root = root->right;
                delete tmp;
            } else if(!root->right) {
                Node *tmp = root;
                root = root->left;
                delete tmp;
            } else {
                Node *tmp = jo_min(root->right);
                root->key = tmp->key;
                root->value = tmp->value;
                erase(root->right, tmp->key);
            }
            root_size--;
        }
    }

    Node *find(Node *root, const Key &key) const {
        if(!root) return nullptr;
        if(cmp(key, root->key)) {
            return find(root->left, key);
        } else if(cmp(root->key, key)) {
            return find(root->right, key);
        } else {
            return root;
        }
    }

    void clear(Node *root) {
        if(!root) return;
        clear(root->left);
        clear(root->right);
        delete root;
    }

    void copy(Node *&root, Node *other) {
        if(!other) {
            root = nullptr;
            return;
        }
        root = new Node(other->key, other->value);
        copy(root->left, other->left);
        copy(root->right, other->right);
    }

    void swap(Node *&root, Node *&other) {
        Node *tmp = root;
        root = other;
        other = tmp;
    }
public:
    jo_map() : root(nullptr), root_size(0), cmp(nullptr) {}
    jo_map(const jo_map<Key, Value> &other) : root(nullptr), root_size(0), cmp(nullptr) {
        copy(root, other.root);
    }
    jo_map(jo_map<Key, Value> &&other) : root(nullptr), root_size(0), cmp(nullptr) {
        swap(root, other.root);
    }
    jo_map<Key, Value> &operator=(const jo_map<Key, Value> &other) {
        if(this == &other) return *this;
        clear(root);
        copy(root, other.root);
        return *this;
    }
    jo_map<Key, Value> &operator=(jo_map<Key, Value> &&other) {
        if(this == &other) return *this;
        clear(root);
        swap(root, other.root);
        return *this;
    }
    ~jo_map() {
        clear(root);
    }

    size_t size() const { return root_size; }
    bool empty() const { return !root_size; }

    void insert(const Key &key, const Value &value) {
        insert(root, key, value);
    }
    void erase(const Key &key) {
        erase(root, key);
    }
    Value &operator[](const Key &key) {
        Node *node = find(root, key);
        if(!node) {
            insert(key, Value());
            node = find(root, key);
        }
        return node->value;
    }
    const Value &operator[](const Key &key) const {
        Node *node = find(root, key);
        if(!node) {
            throw jo_exception("Key not found");
        }
        return node->value;
    }
    bool contains(const Key &key) const {
        return find(root, key) != nullptr;
    }
    Value &at(const Key &key) {
        Node *node = find(root, key);
        if(!node) {
            throw jo_exception("Key not found");
        }
        return node->value;
    }

    void clear() {
        clear(root);
        root = nullptr;
        root_size = 0;
    }

    void swap(jo_map<Key, Value> &other) {
        swap(root, other.root);
        swap(root_size, other.root_size);
        swap(cmp, other.cmp);
    }

    struct iterator {
        Node *node;
        jo_map<Key, Value> *map;
        iterator(Node *node, jo_map<Key, Value> *map) : node(node), map(map) {}
        iterator() : node(nullptr), map(nullptr) {}
        iterator(const iterator &other) : node(other.node), map(other.map) {}
        iterator &operator=(const iterator &other) {
            node = other.node;
            map = other.map;
            return *this;
        }
        iterator &operator++() {
            if(node->right) {
                node = jo_min(node->right);
            } else {
                Node *tmp = node;
                node = node->parent;
                while(node && node->right == tmp) {
                    tmp = node;
                    node = node->parent;
                }
            }
            return *this;
        }
        iterator operator++(int) {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }
        iterator &operator--() {
            if(node->left) {
                node = jo_max(node->left);
            } else {
                Node *tmp = node;
                node = node->parent;
                while(node && node->left == tmp) {
                    tmp = node;
                    node = node->parent;
                }
            }
            return *this;
        }
        iterator operator--(int) {
            iterator tmp = *this;
            --(*this);
            return tmp;
        }
        bool operator==(const iterator &other) const {
            return node == other.node;
        }
        bool operator!=(const iterator &other) const {
            return node != other.node;
        }
        Key &key() { return node->key; }
        Value &value() { return node->value; }
        const Key &key() const { return node->key; }
        const Value &value() const { return node->value; }
    };

    iterator begin() {
        if(!root) return iterator();
        return iterator(jo_min(root), this);
    }

    iterator end() {
        return iterator();
    }

    iterator find(const Key &key) {
        return iterator(find(root, key), this);
    }

    class reverse_iterator {
        Node *node;
        jo_map<Key, Value> *map;
        reverse_iterator(Node *node, jo_map<Key, Value> *map) : node(node), map(map) {}

    public:
        reverse_iterator() : node(nullptr), map(nullptr) {}
        reverse_iterator(const reverse_iterator &other) : node(other.node), map(other.map) {}
        reverse_iterator &operator=(const reverse_iterator &other) {
            node = other.node;
            map = other.map;
            return *this;
        }
        reverse_iterator &operator++() {
            if(node->left) {
                node = jo_max(node->left);
            } else {
                Node *tmp = node;
                node = node->parent;
                while(node && node->left == tmp) {
                    tmp = node;
                    node = node->parent;
                }
            }
            return *this;
        }
        reverse_iterator operator++(int) {
            reverse_iterator tmp = *this;
            ++(*this);
            return tmp;
        }
        reverse_iterator &operator--() {
            if(node->right) {
                node = jo_min(node->right);
            } else {
                Node *tmp = node;
                node = node->parent;
                while(node && node->right == tmp) {
                    tmp = node;
                    node = node->parent;
                }
            }
            return *this;
        }
        reverse_iterator operator--(int) {
            reverse_iterator tmp = *this;
            --(*this);
            return tmp;
        }
        bool operator==(const reverse_iterator &other) const {
            return node == other.node;
        }
        bool operator!=(const reverse_iterator &other) const {
            return node != other.node;
        }
        Key &key() { return node->key; }
        Value &value() { return node->value; }
        const Key &key() const { return node->key; }
        const Value &value() const { return node->value; }
    };

    reverse_iterator rbegin() {
        if(!root) return reverse_iterator();
        return reverse_iterator(jo_max(root), this);
    }

    reverse_iterator rend() {
        return reverse_iterator();
    }

    reverse_iterator rfind(const Key &key) {
        return reverse_iterator(find(root, key), this);
    }
};

template<typename Key, typename Value>
class jo_hash_map {
    struct Node {
        Key key;
        Value value;
        Node *left, *right;
        Node *parent;
        int height;
        Node(const Key &key, const Value &value) : key(key), value(value), left(nullptr), right(nullptr), parent(nullptr), height(1) {}
    };

    Node *root;
    int root_size;
    int (*hash)(const Key &key);
    bool (*equal)(const Key &a, const Key &b);

public:

    jo_hash_map() : root(nullptr), root_size(0), hash(nullptr), equal(nullptr) {}

    jo_hash_map(int (*hash)(const Key &key), bool (*equal)(const Key &a, const Key &b)) : root(nullptr), root_size(0), hash(hash), equal(equal) {}

    jo_hash_map(const jo_hash_map &other) : root(nullptr), root_size(0), hash(other.hash), equal(other.equal) {
        for(auto it = other.begin(); it != other.end(); ++it) {
            insert(new Node(it->key, it->value));
        }
    }

    jo_hash_map(jo_hash_map &&other) : root(other.root), root_size(other.root_size), hash(other.hash), equal(other.equal) {
        other.root = nullptr;
        other.root_size = 0;
    }

    jo_hash_map &operator=(const jo_hash_map &other) {
        jo_hash_map tmp(other);
        swap(tmp);
        return *this;
    }

    jo_hash_map &operator=(jo_hash_map &&other) {
        swap(other);
        return *this;
    }

    ~jo_hash_map() {
        clear();
    }

    void swap(jo_hash_map &other) {
        jo_swap(root, other.root);
        jo_swap(root_size, other.root_size);
        jo_swap(hash, other.hash);
        jo_swap(equal, other.equal);
    }

    void clear() {
        for(auto it = begin(); it != end(); ++it) {
            delete it.node;
        }
        root = nullptr;
        root_size = 0;
    }

    int count(const Key &key) const {
        return find(root, key) ? 1 : 0;
    }

    Value &operator[](const Key &key) {
        Node *node = find(root, key);
        if(node) return node->value;
        insert(new Node(key, Value()));
        return root->value;
    }

    const Value &operator[](const Key &key) const {
        Node *node = find(root, key);
        if(node) return node->value;
        throw jo_exception("jo_hash_map::operator[]");
    }

    bool contains(const Key &key) const {
        return find(root, key) != nullptr;
    }

    bool empty() const {
        return root_size == 0;
    }

    int size() const {
        return root_size;
    }

    class iterator {
        Node *node;
        jo_hash_map *map;
        friend class jo_hash_map;
        iterator(Node *node, jo_hash_map *map) : node(node), map(map) {}
    public:
        iterator() : node(nullptr), map(nullptr) {}
        iterator &operator++() {
            if(node->right) {
                node = jo_min(node->right);
            } else {
                Node *tmp = node;
                node = node->parent;
                while(node && node->right == tmp) {
                    tmp = node;
                    node = node->parent;
                }
            }
            return *this;
        }
        iterator operator++(int) {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }
        iterator &operator--() {
            if(node->left) {
                node = jo_max(node->left);
            } else {
                Node *tmp = node;
                node = node->parent;
                while(node && node->left == tmp) {
                    tmp = node;
                    node = node->parent;
                }
            }
            return *this;
        }
        iterator operator--(int) {
            iterator tmp = *this;
            --(*this);
            return tmp;
        }
        bool operator==(const iterator &other) const {
            return node == other.node;
        }
        bool operator!=(const iterator &other) const {
            return node != other.node;
        }
        const Key &key() const { return node->key; }
        Value &value() const { return node->value; }
    };

    iterator begin() {
        if(!root) return iterator();
        return iterator(jo_min(root), this);
    }

    iterator end() {
        return iterator();
    }

    iterator find(const Key &key) {
        Node *node = find(root, key);
        if(node) return iterator(node, this);
        return end();
    }

    void insert(const Key &key, const Value &value) {
        insert(new Node(key, value));
    }

    void insert(const jo_pair<Key, Value> &pair) {
        insert(new Node(pair.first, pair.second));
    }

    void insert(Node *node) {
        if(!root) {
            root = node;
            root_size++;
            return;
        }
        Node *tmp = root;
        while(tmp) {
            if(hash(node->key) < hash(tmp->key)) {
                if(tmp->left) {
                    tmp = tmp->left;
                } else {
                    tmp->left = node;
                    node->parent = tmp;
                    root_size++;
                    insert_fixup(node);
                    return;
                }
            } else if(hash(node->key) > hash(tmp->key)) {
                if(tmp->right) {
                    tmp = tmp->right;
                } else {
                    tmp->right = node;
                    node->parent = tmp;
                    root_size++;
                    insert_fixup(node);
                    return;
                }
            } else {
                if(equal(node->key, tmp->key)) {
                    tmp->value = node->value;
                    delete node;
                    return;
                }
                if(tmp->left) {
                    tmp = tmp->left;
                } else {
                    tmp->left = node;
                    node->parent = tmp;
                    root_size++;
                    insert_fixup(node);
                    return;
                }
            }
        }
    }

    void erase(const Key &key) {
        Node *node = find(root, key);
        if(!node) return;
        erase(node);
    }

    void erase(iterator it) {
        erase(it.node);
    }

    void erase(Node *node) {
        if(!node) return;
        if(node->left && node->right) {
            Node *tmp = jo_max(node->left);
            node->key = tmp->key;
            node->value = tmp->value;
            node = tmp;
        }
        Node *child = node->left ? node->left :
                        node->right ? node->right : nullptr;
        if(child) {
            child->parent = node->parent;
            if(node->parent) {
                if(node->parent->left == node) {
                    node->parent->left = child;
                } else {
                    node->parent->right = child;
                }
            } else {
                root = child;
            }
        } else {
            if(node->parent) {
                if(node->parent->left == node) {
                    node->parent->left = nullptr;
                } else {
                    node->parent->right = nullptr;
                }
            } else {
                root = nullptr;
            }
        }
        delete node;
        root_size--;
    }
};

// std sort implementation using quicksort
template<typename T>
struct jo_sort {
    static void sort(T *array, int size) {
        if(size <= 1) return;
        int pivot = size / 2;
        jo_swap(array[0], array[pivot]);
        int i = 1;
        for(int j = 1; j < size; j++) {
            if(array[j] < array[0]) {
                jo_swap(array[i], array[j]);
                i++;
            }
        }
        jo_swap(array[0], array[i - 1]);
        sort(array, i - 1);
        sort(array + i, size - i);
    }
};

// std stable sort implementation using merge sort
template<typename T>
struct jo_stable_sort {
    static void merge(T *array, int size, int start, int mid, int end) {
        T *tmp = new T[end - start];
        int i = start, j = mid, k = 0;
        while(i < mid && j < end) {
            if(array[i] < array[j]) {
                tmp[k++] = array[i++];
            } else {
                tmp[k++] = array[j++];
            }
        }
    }

    static void sort(T *array, int size, int start, int end) {
        if(end - start <= 1) return;
        int mid = (start + end) / 2;
        sort(array, size, start, mid);
        sort(array, size, mid, end);
        merge(array, size, start, mid, end);
    }

    static void sort(T *array, int size) {
        sort(array, size, 0, size);
    }

    static void merge(T *array, int size, int start, int end) {
        if(end - start <= 1) return;
        int mid = (start + end) / 2;
        merge(array, size, start, mid, end);
        merge(array, size, mid, end);
    }

    static void merge(T *array, int size) {
        merge(array, size, 0, size);
    }
};

template<typename T> 
struct jo_shared_ptr {
    T* ptr;
    int* ref_count;
    
    jo_shared_ptr() : ptr(nullptr), ref_count(nullptr) {}
    jo_shared_ptr(T* Ptr) : ptr(Ptr), ref_count(Ptr ? new int(1) : nullptr) {}
    jo_shared_ptr(const jo_shared_ptr& other) : ptr(other.ptr), ref_count(other.ref_count) {
        if(ref_count) (*ref_count)++;
    }
    jo_shared_ptr(jo_shared_ptr&& other) : ptr(other.ptr), ref_count(other.ref_count) {
        other.ptr = nullptr;
        other.ref_count = nullptr;
    }
    
    jo_shared_ptr& operator=(const jo_shared_ptr& other) {
        if (this != &other) {
            if(other.ref_count) (*other.ref_count)++;
            if(ref_count) {
                if(--(*ref_count) == 0) {
                    delete ptr;
                    delete ref_count;
                }
            }
            ptr = other.ptr;
            ref_count = ptr ? other.ref_count : nullptr;
        }
        return *this;
    }
    
    jo_shared_ptr& operator=(jo_shared_ptr&& other) {
        if (this != &other) {
            if(ref_count) {
                if(--(*ref_count) == 0) {
                    delete ptr;
                    delete ref_count;
                }
            }
            ptr = other.ptr;
            ref_count = ptr ? other.ref_count : nullptr;
            other.ptr = nullptr;
            other.ref_count = nullptr;
        }
        return *this;
    }
    
    ~jo_shared_ptr() {
        if(ref_count) {
            if(--(*ref_count) == 0) {
                delete ptr;
                delete ref_count;
                ptr = nullptr;
                ref_count = nullptr;
            }
        }
    }

    T& operator*() { return *ptr; }
    T* operator->() { return ptr; }
    const T& operator*() const { return *ptr; }
    const T* operator->() const { return ptr; }

    bool operator==(const jo_shared_ptr& other) const { return ptr == other.ptr; }
    bool operator!=(const jo_shared_ptr& other) const { return ptr != other.ptr; }

    bool operator!() const { return ptr == nullptr; }
    operator bool() const { return ptr != nullptr; }
};

// jo_unqiue_ptr
template<typename T>
struct jo_unique_ptr {
    T* ptr;
    jo_unique_ptr(T* ptr) : ptr(ptr) {}
    jo_unique_ptr(const jo_unique_ptr& other) = delete;
    jo_unique_ptr(jo_unique_ptr&& other) : ptr(other.ptr) {
        other.ptr = nullptr;
    }
    jo_unique_ptr& operator=(const jo_unique_ptr& other) = delete;
    jo_unique_ptr& operator=(jo_unique_ptr&& other) {
        if (this != &other) {
            delete ptr;
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }
    ~jo_unique_ptr() {
        delete ptr;
    }
};


template<typename KEY, typename VALUE>
class jo_unordered_hash_map {
    struct Node {
        KEY key;
        VALUE value;
        Node *next;
    };
    Node *buckets;
    int bucket_count;
    Node *get_bucket(const KEY& key) {
        return buckets + (key % bucket_count);
    }
public:
    jo_unordered_hash_map() : bucket_count(0) {
        buckets = nullptr;
    }
    jo_unordered_hash_map(int bucket_count) : bucket_count(bucket_count) {
        buckets = new Node[bucket_count];
    }
    jo_unordered_hash_map(const jo_unordered_hash_map& other) : bucket_count(other.bucket_count) {
        buckets = new Node[bucket_count];
        for(int i = 0; i < bucket_count; i++) {
            Node *bucket = get_bucket(other.buckets[i]->key);
            Node *node = new Node(other.buckets[i]);
            node->next = bucket->ptr;
            bucket->ptr = node;
        }
    }
    jo_unordered_hash_map(jo_unordered_hash_map&& other) : bucket_count(other.bucket_count) {
        buckets = other.buckets;
        other.buckets = nullptr;
    }
    jo_unordered_hash_map& operator=(const jo_unordered_hash_map& other) {
        if (this != &other) {
            for(int i = 0; i < bucket_count; i++) {
                Node *bucket = get_bucket(other.buckets[i]->key);
                Node *node = new Node(other.buckets[i]);
                node->next = bucket->ptr;
                bucket->ptr = node;
            }
        }
        return *this;
    }
    jo_unordered_hash_map& operator=(jo_unordered_hash_map&& other) {
        if (this != &other) {
            for(int i = 0; i < bucket_count; i++) {
                Node *bucket = get_bucket(other.buckets[i]->key);
                Node *node = new Node(other.buckets[i]);
                node->next = bucket->ptr;
                bucket->ptr = node;
            }
            other.buckets = nullptr;
        }
        return *this;
    }
    ~jo_unordered_hash_map() {
        for(int i = 0; i < bucket_count; i++) {
            Node *bucket = get_bucket(buckets[i]->key);
            while(bucket->ptr) {
                Node *node = bucket->ptr;
                bucket->ptr = node->next;
                delete node;
            }
        }
        delete[] buckets;
    }

    VALUE& operator[](const KEY& key) {
        Node *bucket = get_bucket(key);
        while(bucket->ptr) {
            if (bucket->ptr->key == key) {
                return bucket->ptr->value;
            }
            bucket = bucket->ptr;
        }
        Node *node = new Node;
        node->key = key;
        node->value = VALUE();
        node->next = bucket->ptr;
        bucket->ptr = node;
        return node->value;
    }

    bool contains(const KEY& key) {
        Node *bucket = get_bucket(key);
        while(bucket->ptr) {
            if (bucket->ptr->key == key) {
                return true;
            }
            bucket = bucket->ptr;
        }
        return false;
    }

    VALUE& get(const KEY& key) {
        Node *bucket = get_bucket(key);
        while(bucket->ptr) {
            if (bucket->ptr->key == key) {
                return bucket->ptr->value;
            }
            bucket = bucket->ptr;
        }
        throw "Key not found";
    }

    void remove(const KEY& key) {
        Node *bucket = get_bucket(key);
        while(bucket->ptr) {
            if (bucket->ptr->key == key) {
                Node *node = bucket->ptr;
                bucket->ptr = node->next;
                delete node;
                return;
            }
            bucket = bucket->ptr;
        }
        throw "Key not found";
    }

    void clear() {
        for(int i = 0; i < bucket_count; i++) {
            Node *bucket = get_bucket(buckets[i]->key);
            while(bucket->ptr) {
                Node *node = bucket->ptr;
                bucket->ptr = node->next;
                delete node;
            }
        }
    }

    int size() {
        int size = 0;
        for(int i = 0; i < bucket_count; i++) {
            Node *bucket = get_bucket(buckets[i]->key);
            while(bucket->ptr) {
                size++;
                bucket = bucket->ptr;
            }
        }
        return size;
    }

    bool empty() {
        for(int i = 0; i < bucket_count; i++) {
            Node *bucket = get_bucket(buckets[i]->key);
            while(bucket->ptr) {
                return false;
            }
        }
        return true;
    }

    class iterator {
        Node *bucket;
        Node *node;
        jo_unordered_hash_map *map;

        void next() {
            while(bucket->ptr) {
                node = bucket->ptr;
                bucket = map->get_bucket(node->key);
            }
        }
    public:
        iterator(jo_unordered_hash_map *map) : map(map) {
            bucket = map->buckets;
            node = nullptr;
            next();
        }
        iterator(const iterator& other) : map(other.map), bucket(other.bucket), node(other.node) {}

        iterator& operator=(const iterator& other) {
            if (this != &other) {
                bucket = other.bucket;
                node = other.node;
            }
            return *this;
        }

        iterator& operator++() { node = node->next; next(); return *this; }
        iterator operator++(int) { iterator tmp(*this); operator++(); return tmp; }

        bool operator==(const iterator& other) { return node == other.node; }
        bool operator!=(const iterator& other) { return node != other.node; }
        VALUE& operator*() { return node->value; }
        VALUE* operator->() { return &node->value; }
    };

    iterator begin() { return iterator(this); }
    iterator end() { return iterator(this); }

    // find implementation
    iterator find(const KEY& key) {
        Node *bucket = get_bucket(key);
        while(bucket->ptr) {
            if (bucket->ptr->key == key) {
                return iterator(this);
            }
            bucket = bucket->ptr;
        }
        return iterator(this);
    }
};

// Persistent Vector implementation (vector that supports versioning)
// For use in purely functional languages
template<typename T>
struct jo_persistent_vector
{
    struct node {
        jo_shared_ptr<node> children[32];
        T elements[32];

        node() : children(), elements() {}

        ~node() {
            for (int i = 0; i < 32; ++i) {
                children[i] = NULL;
            }
        }

        node(const jo_shared_ptr<node> other) : children(), elements() {
            if(other.ptr) {
                for (int i = 0; i < 32; ++i) {
                    children[i] = other->children[i];
                    elements[i] = other->elements[i];
                }
            }
        }
    };

    jo_shared_ptr<node> head;
    jo_shared_ptr<node> tail;
    size_t head_offset;
    size_t tail_length;
    size_t length;
    size_t depth;

    jo_persistent_vector() {
        head = new node();
        tail = new node();
        head_offset = 0;
        tail_length = 0;
        length = 0;
        depth = 0;
    }

    jo_persistent_vector(size_t initial_size) {
        head = new node();
        tail = new node();
        head_offset = 0;
        tail_length = 0;
        length = 0;
        depth = 0;
        for (size_t i = 0; i < initial_size; ++i) {
            push_back_inplace(T());
        }
    }

    jo_persistent_vector(const jo_persistent_vector &other) {
        head = other.head;
        tail = other.tail;
        head_offset = other.head_offset;
        tail_length = other.tail_length;
        length = other.length;
        depth = other.depth;
    }

    jo_persistent_vector *clone() const {
        return new jo_persistent_vector(*this);
    }

    void append_tail() {
        size_t tail_offset = length + head_offset - tail_length;
        size_t shift = 5 * (depth + 1);
        size_t max_size = 1 << (5 * shift);

        // check for root overflow, and if so expand tree by 1 level
        if(length >= max_size) {
            node *new_root = new node();
            new_root->children[0] = head;
            head = new_root;
            ++depth;
            shift = 5 * (depth + 1);
        }

        head = new node(head);

        // Set up our tree traversal. We subtract 5 from level each time
        // in order to get all the way down to the level above where we want to
        // insert this tail node.
        jo_shared_ptr<node> cur = NULL;
        jo_shared_ptr<node> prev = head;
        size_t index = 0;
        size_t key = tail_offset;
        for(size_t level = shift; level > 0; level -= 5) {
            index = (key >> level) & 31;
            // we are at the end of our tree, insert tail node
            if(level - 5 == 0) {
                assert(!prev->children[index]);
                prev->children[index] = tail;
                break;
            }
            cur = prev->children[index];
            if(!cur) {
                cur = new node();
            } else {
                cur = new node(cur);
            }
            prev->children[index] = cur;
            prev = cur;
        }

        // Make our new tail
        tail = new node();
        tail_length = 0;
    }

    jo_persistent_vector *append(const T &value) const {
        // Create a copy of our root node from which we will base our append
        jo_persistent_vector *copy = new jo_persistent_vector(*this);

        // do we have space?
        if(copy->tail_length >= 32) {
            copy->append_tail();
            copy->tail->elements[copy->tail_length] = value;
            copy->tail_length++;
            copy->length++;
            return copy;
        }

        copy->tail = new node(copy->tail);
        copy->tail->elements[copy->tail_length++] = value;
        copy->length++;
        return copy;
    }

    // append inplace
    jo_persistent_vector *append_inplace(const T &value) {
        // do we have space?
        if(tail_length >= 32) {
            append_tail();
        }

        tail->elements[tail_length++] = value;
        length++;
        return this;
    }

    jo_persistent_vector *assoc(size_t index, const T &value) const {
        index += head_offset;

        int shift = 5 * (depth + 1);
        size_t tail_offset = length + head_offset - tail_length;

        if(index >= length + head_offset) {
            return append(value);
        }

        // Create a copy of our root node from which we will base our append
        jo_persistent_vector *copy = new jo_persistent_vector(*this);

        if(index < tail_offset) {
            copy->head = new node(copy->head);

            jo_shared_ptr<node> cur = NULL;
            jo_shared_ptr<node> prev = copy->head;
            for (int level = shift; level > 0; level -= 5) {
                size_t i = (index >> level) & 31;
                // copy nodes as we traverse
                cur = new node(prev->children[i]);
                prev->children[i] = cur;
                prev = cur;
            }
            prev->elements[index & 31] = value;
        } else {
            copy->tail = new node(copy->tail);
            copy->tail->elements[index - tail_offset] = value;
        }

        return copy;
    }

    jo_persistent_vector *assoc_inplace(size_t index, const T &value) {
        index += head_offset;

        int shift = 5 * (depth + 1);
        size_t tail_offset = length + head_offset - tail_length;

        if(index >= length + head_offset) {
            return append(value);
        }

        // Create a copy of our root node from which we will base our append
        if(index < tail_offset) {
            head = new node(head);

            jo_shared_ptr<node> cur = NULL;
            jo_shared_ptr<node> prev = head;
            for (int level = shift; level > 0; level -= 5) {
                size_t i = (index >> level) & 31;
                // copy nodes as we traverse
                cur = new node(prev->children[i]);
                prev->children[i] = cur;
                prev = cur;
            }
            prev->elements[index & 31] = value;
        } else {
            tail = new node(tail);
            tail->elements[index - tail_offset] = value;
        }

        return this;
    }

    jo_persistent_vector *assoc_mutate(size_t index, const T &value) {
        index += head_offset;

        int shift = 5 * (depth + 1);
        size_t tail_offset = length + head_offset - tail_length;

        if(index >= length + head_offset) {
            return append_inplace(value);
        }

        if(index < tail_offset) {
            jo_shared_ptr<node> cur = NULL;
            jo_shared_ptr<node> prev = head;
            for (int level = shift; level > 0; level -= 5) {
                size_t i = (index >> level) & 31;
                cur = prev->children[i];
                prev->children[i] = cur;
                prev = cur;
            }
            prev->elements[index & 31] = value;
        } else {
            tail->elements[index - tail_offset] = value;
        }

        return this;
    }

    // dissoc
    jo_persistent_vector *dissoc(size_t index) const {
        return assoc(index, T());
    }

    jo_persistent_vector *dissoc_inplace(size_t index) {
        return assoc_inplace(index, T());
    }

    jo_persistent_vector *set(size_t index, const T &value) const {
        return assoc(index, value);
    }

    jo_persistent_vector *cons(const T &value) const {
        return append(value);
    }

    jo_persistent_vector *cons_inplace(const T &value) {
        return append_inplace(value);
    }

    jo_persistent_vector *conj(const T &value) const {
        return append(value);
    }

    jo_persistent_vector *conj_inplace(const T &value) {
        return append_inplace(value);
    }

    jo_persistent_vector *disj(const T &value) const {
        return dissoc(value);
    }

    jo_persistent_vector *conj(const jo_persistent_vector &other) const {
        jo_persistent_vector *copy = new jo_persistent_vector(*this);
        // loop through other, and append to copy
        for(size_t i = 0; i < other.length; ++i) {
            copy->append_inplace(other[i]);
        }
        return copy;
    }

    jo_persistent_vector *conj_inplace(const jo_persistent_vector &other) {
        // loop through other, and append to copy
        for(size_t i = 0; i < other.length; ++i) {
            append_inplace(other[i]);
        }
        return this;
    }

    jo_persistent_vector *push_back(const T &value) const {
        return append(value);
    }

    jo_persistent_vector *push_back_inplace(const T &value) {
        return append_inplace(value);
    }

    jo_persistent_vector *resize(size_t new_size) const {
        if(new_size == length) {
            return new jo_persistent_vector(*this);
        }

        if(new_size < length) {
            return take(new_size);
        }

        // This could be faster...
        jo_persistent_vector *copy = new jo_persistent_vector(*this);
        for(size_t i = length; i < new_size; ++i) {
            copy->append_inplace(T());
        }
        return copy;
    }

    jo_persistent_vector *push_front(const T &value) const {
        assert(head_offset > 0);
        jo_persistent_vector *copy = new jo_persistent_vector(*this);
        copy->head_offset--;
        copy->length++;
        copy->assoc_inplace(0, value);
        return copy;
    }

    jo_persistent_vector *push_front_inplace(const T &value) {
        assert(head_offset > 0);
        head_offset--;
        length++;
        assoc_inplace(0, value);
        return this;
    }
    
    jo_persistent_vector *pop_back() const {
        size_t shift = 5 * (depth + 1);
        size_t tail_offset = length + head_offset - tail_length;

        // Create a copy of our root node from which we will base our append
        jo_persistent_vector *copy = new jo_persistent_vector(*this);

        // Is it in the tail?
        if(length + head_offset >= tail_offset) {
            // copy the tail (since we are changing the data)
            copy->tail = new node(copy->tail);
            copy->tail_length--;
            copy->length--;
            return copy;
        }

        // traverse duplicating the way down.
        jo_shared_ptr<node> cur = NULL;
        jo_shared_ptr<node> prev = copy->head;
        size_t key = length + head_offset - 1;
        for (size_t level = shift; level > 0; level -= 5) {
            size_t index = (key >> level) & 0x1f;
            // copy nodes as we traverse
            cur = new node(prev->children[index]);
            prev->children[index] = cur;
            prev = cur;
        }
        prev->elements[key & 0x1f] = T();
        copy->tail_length--;
        copy->length--;
        return copy;
    }

    jo_persistent_vector *pop_back_inplace() {
        if(length == 0) {
            return this;
        }

        size_t shift = 5 * (depth + 1);
        size_t tail_offset = length + head_offset - tail_length;

        // Is it in the tail?
        if(length + head_offset >= tail_offset) {
            // copy the tail (since we are changing the data)
            tail = new node(tail);
            tail_length--;
            length--;
            return this;
        }

        // traverse duplicating the way down.
        jo_shared_ptr<node> cur = NULL;
        jo_shared_ptr<node> prev = head;
        size_t key = length + head_offset - 1;
        for (size_t level = shift; level > 0; level -= 5) {
            size_t index = (key >> level) & 0x1f;
            // copy nodes as we traverse
            cur = new node(prev->children[index]);
            prev->children[index] = cur;
            prev = cur;
        }
        prev->elements[key & 31] = T();
        tail_length--;
        length--;
        return this;
    }

    jo_persistent_vector *pop_front() const {
        jo_persistent_vector *copy = new jo_persistent_vector(*this);
        copy->head_offset++;
        copy->length--;
        return copy;
    }

    jo_persistent_vector *pop_front_inplace() {
        head_offset++;
        length--;
        return this;
    }

    jo_persistent_vector *rest() const {
        return pop_front();
    }

    jo_persistent_vector *pop() const {
        return pop_front();
    }

    jo_persistent_vector *drop(size_t n) const {
        if(n == 0) {
            return new jo_persistent_vector(*this);
        }
        if(n >= length) {
            return new jo_persistent_vector();
        }
        jo_persistent_vector *copy = new jo_persistent_vector(*this);
        copy->head_offset += n;
        copy->length -= n;
        return copy;
    }

    jo_persistent_vector *take(size_t n) const {
        if(n >= length) {
            return new jo_persistent_vector(*this);
        }
        jo_persistent_vector *copy = new jo_persistent_vector();
        for(size_t i = 0; i < n; ++i) {
            copy->append_inplace((*this)[i]);
        }
        return copy;
    }

    jo_persistent_vector *take_last(size_t n) const {
        if(n >= length) {
            return new jo_persistent_vector(*this);
        }
        return drop(length - n);
    }

    jo_persistent_vector *random_sample(float prob) const {
        if(prob <= 0.0f) {
            return new jo_persistent_vector();
        }
        if(prob >= 1.0f) {
            return new jo_persistent_vector(*this);
        }

        jo_persistent_vector *copy = new jo_persistent_vector();
        for(size_t i = 0; i < length; ++i) {
            if(jo_random_float() < prob) {
                copy->push_back_inplace(nth(i));
            }
        }
        return copy;
    }

    jo_persistent_vector *shuffle() const {
        jo_persistent_vector *copy = new jo_persistent_vector(*this);
        for(size_t i = 0; i < length; ++i) {
            size_t j = jo_random_int(i, length);
            T tmp = copy->nth(i);
            copy->assoc_inplace(i, copy->nth(j));
            copy->assoc_inplace(j, tmp);
        }
        return copy;
    }

    T &operator[] (size_t index) {
        index += head_offset;

        size_t shift = 5 * (depth + 1);
        size_t tail_offset = length + head_offset - tail_length;

        // Is it in the tail?
        if(index >= tail_offset) {
            return tail->elements[index - tail_offset];
        }

        // traverse 
        jo_shared_ptr<node> cur = NULL;
        jo_shared_ptr<node> prev = head;
        size_t key = index;
        for (size_t level = shift; level > 0; level -= 5) {
            size_t index = (key >> level) & 0x1f;
            cur = prev->children[index];
            if(!cur) {
                return tail->elements[key - tail_offset];
            }
            prev = cur;
        }
        return prev->elements[key & 0x1f];
    }

    const T &operator[] (size_t index) const {
        index += head_offset;

        size_t shift = 5 * (depth + 1);
        size_t tail_offset = length + head_offset - tail_length;

        // Is it in the tail?
        if(index >= tail_offset) {
            return tail->elements[index - tail_offset];
        }

        // traverse

        jo_shared_ptr<node> cur = NULL;
        jo_shared_ptr<node> prev = head;
        size_t key = index;
        for (size_t level = shift; level > 0; level -= 5) {
            size_t index = (key >> level) & 0x1f;
            cur = prev->children[index];
            if(!cur) {
                return tail->elements[key - tail_offset];
            }
            prev = cur;
        }
        return prev->elements[key & 0x1f];
    }

    T &nth(size_t index) {
        return (*this)[index];
    }

    const T &nth(size_t index) const {
        return (*this)[index];
    }

    T &nth_clamp(int index) {
        index = index < 0 ? 0 : index;
        index = index > (int)length-1 ? length-1 : index;
        return (*this)[index];
    }
    
    const T &nth_clamp(int index) const {
        index = index < 0 ? 0 : index;
        index = index > (int)length-1 ? length-1 : index;
        return (*this)[index];
    }

    const T &first_value() const {
        return (*this)[0];
    }

    size_t size() const {
        return length;
    }

    bool empty() const {
        return length == 0;
    }

    jo_persistent_vector *clear() {
        return new jo_persistent_vector();
    }

    jo_persistent_vector *reverse() {
        jo_persistent_vector *copy = new jo_persistent_vector();
        for(int i = length-1; i >= 0; --i) {
            copy->push_back_inplace((*this)[i]);
        }
        return copy;
    }

    size_t find(const T &value) const {
        size_t shift = 5 * (depth + 1);
        size_t tail_offset = length + head_offset - tail_length;

        // Is it in the tail?
        if(tail_length > 0) {
            for(size_t i = 0; i < tail_length; ++i) {
                if(tail->elements[i] == value) {
                    return tail_offset + i;
                }
            }
        }

        for(size_t i = 0; i < tail_offset; ++i) {
            if(nth(i) == value) {
                return i;
            }
        }

        return jo_npos;
    }

    bool contains(const T &value) const {
        return find(value) != jo_npos;
    }

    // find with lambda for comparison
    template<typename F>
    size_t find(const F &f) const {
        size_t shift = 5 * (depth + 1);
        size_t tail_offset = length + head_offset - tail_length;

        // Is it in the tail?
        if(tail_length > 0) {
            for(size_t i = 0; i < tail_length; ++i) {
                if(f(tail->elements[i])) {
                    return tail_offset + i;
                }
            }
        }

        for(size_t i = 0; i < tail_offset; ++i) {
            if(f(nth(i))) {
                return i;
            }
        }

        return jo_npos;
    }

    // contains with lambda for comparison
    template<typename F>
    bool contains(const F &f) const {
        return find(f) != jo_npos;
    }

    jo_persistent_vector *subvec(size_t start, size_t end) const {
        if(start == 0) {
            return take(end);
        }
        if(end == length) {
            return drop(start);
        }
        jo_persistent_vector *copy = new jo_persistent_vector();
        for(size_t i = start; i < end; ++i) {
            copy->push_back((*this)[i]);
        }
        return copy;
    }

    void print() const {
        printf("[");
        for(size_t i = 0; i < size(); ++i) {
            if(i > 0) {
                printf(", ");
            }
            printf("%d", (*this)[i]);
        }
        printf("]");
    }

    // iterator
    class iterator {
    public:
        iterator() : vec(NULL), index(0) {}
        iterator(const jo_persistent_vector *vec, size_t index) : vec(vec), index(index) {}
        iterator(const iterator &other) : vec(other.vec), index(other.index) {}
        iterator &operator++() {
            ++index;
            return *this;
        }
        iterator operator++(int) {
            iterator copy(*this);
            ++index;
            return copy;
        }
        bool operator==(const iterator &other) const {
            return vec == other.vec && index == other.index;
        }
        bool operator!=(const iterator &other) const {
            return vec != other.vec || index != other.index;
        }
        operator bool() const {
            return index < vec->size();
        }
        const T &operator*() const {
            return (*vec)[index];
        }
        const T *operator->() const {
            return &(*vec)[index];
        }
        iterator operator+(size_t offset) const {
            return iterator(vec, index + offset);
        }
        iterator operator-(size_t offset) const {
            return iterator(vec, index - offset);
        }
        iterator &operator+=(size_t offset) {
            index += offset;
            return *this;
        }
        iterator &operator-=(size_t offset) {
            index -= offset;
            return *this;
        }
        size_t operator-(const iterator &other) const {
            return index - other.index;
        }
        size_t operator-(const iterator &other) {
            return index - other.index;
        }
        iterator &operator=(const iterator &other) {
            vec = other.vec;
            index = other.index;
            return *this;
        }

    private:
        const jo_persistent_vector *vec;
        size_t index;
    };

    iterator begin() const {
        return iterator(this, 0);
    }

    iterator end() const {
        return iterator(this, size());
    }

    T &back() {
        return tail->elements[tail_length - 1];
    }

    const T &back() const {
        return tail->elements[tail_length - 1];
    }

    T &front() {
        return nth(0);
    }

    const T &front() const {
        return nth(0);
    }

    T &first_value() {
        return nth(0);
    }

    T &last_value() {
        return back();
    }

    jo_persistent_vector *erase(size_t index) const {
        jo_persistent_vector *copy = new jo_persistent_vector(*this);
        // @ This could be faster
        // iterate through values and just copy everything except that item
        for(size_t i = 0; i < index; ++i) {
            copy->push_back_inplace((*this)[i]);
        }
        for(size_t i = index + 1; i < size(); ++i) {
            copy->push_back_inplace((*this)[i]);
        }
        return copy;
    }

    jo_persistent_vector *erase(size_t start, size_t end) const {
        jo_persistent_vector *copy = new jo_persistent_vector(*this);
        // @ This could be faster
        // iterate through values and just copy everything except that item
        for(size_t i = 0; i < start; ++i) {
            copy->push_back_inplace((*this)[i]);
        }
        for(size_t i = end; i < size(); ++i) {
            copy->push_back_inplace((*this)[i]);
        }
        return copy;
    }

    jo_persistent_vector *erase_value(const T &value) const {
        jo_persistent_vector *copy = new jo_persistent_vector(*this);
        // iterate through values and just copy everything except that item
        for(size_t i = 0; i < size(); ++i) {
            if((*this)[i] != value) {
                copy->push_back_inplace((*this)[i]);
            }
        }
        return copy;
    }

    jo_persistent_vector *erase_value_inplace(const T &value) {
        jo_persistent_vector copy(*this);
        // iterate through values and just copy everything except that item
        for(size_t i = 0; i < size(); ++i) {
            if((*this)[i] != value) {
                copy.push_back_inplace((*this)[i]);
            }
        }
        *this = copy;
        return this;
    }
};

// A persistent vector class which is fast to both push to front and back
template<typename T>
struct jo_persistent_vector_bidirectional {
    jo_persistent_vector<T> positive;
    jo_persistent_vector<T> negative;

    jo_persistent_vector_bidirectional() {}

    jo_persistent_vector_bidirectional(const jo_persistent_vector<T> &positive, const jo_persistent_vector<T> &negative) :
        positive(positive), negative(negative) {}

    jo_persistent_vector_bidirectional(const jo_persistent_vector_bidirectional &other) :
        positive(other.positive), negative(other.negative) {}

    jo_persistent_vector_bidirectional &operator=(const jo_persistent_vector_bidirectional &other) {
        positive = other.positive;
        negative = other.negative;
        return *this;
    }

    size_t size() const {
        return positive.size() + negative.size();
    }

    jo_persistent_vector_bidirectional *push_back(const T &value) const {
        jo_persistent_vector_bidirectional *copy = new jo_persistent_vector_bidirectional(*this);
        if(copy->positive.length > 0) {
            copy->positive.push_back(value);
        } else if(copy->negative.head_offset > 0) {
            copy->negative = copy->negative.push_front(value);
        } else {
            copy->positive = copy->positive.push_back(value);
        }
        return copy;
    }

    jo_persistent_vector_bidirectional *push_back_inplace(const T &value) {
        if(positive.length > 0) {
            positive.push_back_inplace(value);
        } else if(negative.head_offset > 0) {
            negative.push_front_inplace(value);
        } else {
            positive.push_back_inplace(value);
        }
        return this;
    }

    jo_persistent_vector_bidirectional *push_front(const T &value) const {
        jo_persistent_vector_bidirectional *copy = new jo_persistent_vector_bidirectional(*this);
        if(copy->negative.length > 0) {
            copy->negative.push_back_inplace(value);
        } else if(copy->positive.head_offset > 0) {
            copy->positive.push_front_inplace(value);
        } else {
            copy->negative.push_back_inplace(value);
        }
        return copy;
    }

    jo_persistent_vector_bidirectional *push_front_inplace(const T &value) {
        if(negative.length > 0) {
            negative.push_back_inplace(value);
        } else if(positive.head_offset > 0) {
            positive.push_front_inplace(value);
        } else {
            negative.push_back_inplace(value);
        }
        return this;
    }

    jo_persistent_vector_bidirectional *pop_front() const {
        jo_persistent_vector_bidirectional *copy = new jo_persistent_vector_bidirectional(*this);
        if(copy->negative.length > 0) {
            copy->negative.pop_back_inplace();
        } else {
            copy->positive.pop_front_inplace();
        }
        return copy;
    }

    jo_persistent_vector_bidirectional *pop_front_inplace() {
        if(negative.length > 0) {
            negative.pop_back_inplace();
        } else {
            positive.pop_front_inplace();
        }
        return this;
    }

    jo_persistent_vector_bidirectional *pop_back() const {
        jo_persistent_vector_bidirectional *copy = new jo_persistent_vector_bidirectional(*this);
        if(copy->positive.length > 0) {
            copy->positive = copy->positive.pop_back();
        } else {
            copy->negative = copy->negative.pop_front();
        }
        return copy;
    }

    jo_persistent_vector_bidirectional *pop_back_inplace() {
        if(positive.length > 0) {
            positive.pop_back_inplace();
        } else {
            negative.pop_front_inplace();
        }
        return this;
    }

    T &back() {
        if(positive.length > 0) {
            return positive.back();
        } else {
            return negative.front();
        }
    }

    const T &back() const {
        if(positive.length > 0) {
            return positive.back();
        } else {
            return negative.front();
        }
    }

    T &front() {
        if(positive.length > 0) {
            return positive.front();
        } else {
            return negative.back();
        }
    }

    const T &front() const {
        if(positive.length > 0) {
            return positive.front();
        } else {
            return negative.back();
        }
    }

    T &first_value() {
        return front();
    }

    T &last_value() {
        return back();
    }

    jo_persistent_vector<T> *to_vector() {
        jo_persistent_vector<T> *vec = new jo_persistent_vector<T>();
        for(jo_persistent_vector_bidirectional::iterator it = begin(); it; ++it) {
            vec->push_back_inplace(*it);
        }
        return vec;
    }

    T &operator[](size_t index) {
        if(index < negative.length) {
            return negative[negative.length - index - 1];
        } else {
            return positive[index - negative.length];
        }
    }

    const T &operator[](size_t index) const {
        if(index < negative.length) {
            return negative[negative.length - index - 1];
        } else {
            return positive[index - negative.length];
        }
    }

    T &nth(size_t index) {
        return (*this)[index];
    }

    const T &nth(size_t index) const {
        return (*this)[index];
    }

    jo_persistent_vector_bidirectional *copy() const {
        return new jo_persistent_vector_bidirectional(*this);
    }

    jo_persistent_vector_bidirectional *cons(const T &value) const {
        return push_front(value);
    }

    jo_persistent_vector_bidirectional *cons_inplace(const T &value) {
        return push_front_inplace(value);
    }

    jo_persistent_vector_bidirectional *rest() const {
        return pop_front();
    }

    jo_persistent_vector_bidirectional *pop() const {
        return pop_front();
    }

    jo_persistent_vector_bidirectional *drop(size_t n) const {
        jo_persistent_vector_bidirectional *copy = new jo_persistent_vector_bidirectional(*this);
        for(size_t i = 0; i < n; ++i) {
            copy = copy->pop_front();
        }
        return copy;
    }

    jo_persistent_vector_bidirectional *conj(const jo_persistent_vector_bidirectional &other) const {
        jo_persistent_vector_bidirectional *copy = new jo_persistent_vector_bidirectional(*this);
        for(jo_persistent_vector_bidirectional::iterator it = other.begin(); it; ++it) {
            copy->push_back_inplace(*it);
        }
        return copy;
    }

    jo_persistent_vector_bidirectional *conj_inplace(const jo_persistent_vector_bidirectional &other) {
        for(jo_persistent_vector_bidirectional::iterator it = other.begin(); it; ++it) {
            push_back_inplace(*it);
        }
        return this;
    }

    // subvec (this could be faster)
    jo_persistent_vector_bidirectional *subvec(size_t start, size_t end) const {
        jo_persistent_vector_bidirectional *copy = new jo_persistent_vector_bidirectional();
        for(size_t i = start; i < end; ++i) {
            copy->push_back_inplace((*this)[i]);
        }
        return copy;
    }

    // reverse
    jo_persistent_vector_bidirectional *reverse() const {
        jo_persistent_vector_bidirectional *copy = new jo_persistent_vector_bidirectional();
        for(jo_persistent_vector_bidirectional::iterator it = begin(); it; ++it) {
            copy->push_front_inplace(*it);
        }
        return copy;
    }

    // clone
    jo_persistent_vector_bidirectional *clone() const {
        return copy();
    }

    class iterator {
    public:
        iterator() : vec(NULL), index(0) {}
        iterator(const jo_persistent_vector_bidirectional *vec, size_t index) : vec(vec), index(index) {}
        iterator(const iterator &other) : vec(other.vec), index(other.index) {}
        iterator &operator++() {
            ++index;
            return *this;
        }
        iterator &operator--() {
            --index;
            return *this;
        }
        iterator operator++(int) {
            iterator copy(*this);
            ++index;
            return copy;
        }
        iterator operator--(int) {
            iterator copy(*this);
            --index;
            return copy;
        }
        bool operator==(const iterator &other) const {
            return vec == other.vec && index == other.index;
        }
        bool operator!=(const iterator &other) const {
            return vec != other.vec || index != other.index;
        }
        operator bool() const {
            return index < vec->size();
        }
        const T &operator*() const {
            return (*vec)[index];
        }
        const T *operator->() const {
            return &(*vec)[index];
        }
    private:
        const jo_persistent_vector_bidirectional *vec;
        size_t index;
    };

    iterator begin() const {
        return iterator(this, 0);
    }

    iterator end() const {
        return iterator(this, size());
    }

    jo_persistent_vector_bidirectional::iterator find(const T &value) const {
        for(int i = 0; i < size(); ++i) {
            if(operator[](i) == value) {
                return iterator(this, i);
            }
        }
        return iterator(this, size());
    }

    // find with lambda for comparison
    template<typename F>
    jo_persistent_vector_bidirectional::iterator find(const F &f) const {
        for(int i = 0; i < size(); ++i) {
            if(f(operator[](i))) {
                return iterator(this, i);
            }
        }
        return iterator(this, size());
    }

    bool contains(const T &value) const {
        return find(value) != end();
    }

    // contains with lambda for comparison
    template<typename F>
    bool contains(const F &f) const {
        return find(f) != end();
    }

    jo_persistent_vector_bidirectional *erase(const T &value) const {
        jo_persistent_vector_bidirectional *copy = new jo_persistent_vector_bidirectional(*this);
        copy->negative.erase_value_inplace(value);
        copy->positive.erase_value_inplace(value);
        return copy;
    }
};

// A persistent (non-destructive) linked list implementation.
template<typename T>
struct jo_persistent_list {
    struct node {
        jo_shared_ptr<node> next;
        T value;
        node() : value(), next() {}
        node(const T &value, jo_shared_ptr<node> next) : value(value), next(next) {}
        node(const node &other) : value(other.value), next(other.next) {}
        node &operator=(const node &other) {
            value = other.value;
            next = other.next;
            return *this;
        }
        bool operator==(const node &other) const { return value == other.value && next == other.next; }
        bool operator!=(const node &other) const { return !(*this == other); }
    };

    jo_shared_ptr<node> head;
    jo_shared_ptr<node> tail;
    size_t length;

    jo_persistent_list() : head(NULL), tail(NULL), length(0) {}
    jo_persistent_list(const jo_persistent_list &other) : head(other.head), tail(other.tail), length(other.length) {}
    jo_persistent_list &operator=(const jo_persistent_list &other) {
        head = other.head;
        tail = other.tail;
        length = other.length;
        return *this;
    }

    ~jo_persistent_list() {}

    jo_persistent_list *cons(const T &value) const {
        jo_persistent_list *copy = new jo_persistent_list(*this);
        copy->head = new node(value, copy->head);
        if(!copy->tail) {
            copy->tail = copy->head;
        }
        copy->length++;
        return copy;
    }

    jo_persistent_list *cons_inplace(const T &value) {
        head = new node(value, head);
        if(!tail) {
            tail = head;
        }
        length++;
        return this;
    }

    // makes a new list in reverse order of the current one
    jo_persistent_list *reverse() const {
        jo_persistent_list *copy = new jo_persistent_list();
        jo_shared_ptr<node> cur = head;
        while(cur) {
            copy->head = new node(cur->value, copy->head);
            if(!copy->tail) {
                copy->tail = copy->head;
            }
            cur = cur->next;
        }
        copy->length = length;
        return copy;
    }

    // This is a destructive operation.
    jo_persistent_list *reverse_inplace() {
        jo_shared_ptr<node> cur = head;
        jo_shared_ptr<node> prev = NULL;
        while(cur) {
            jo_shared_ptr<node> next = cur->next;
            cur->next = prev;
            prev = cur;
            cur = next;
        }
        tail = head;
        head = prev;
        return this;
    }

    jo_persistent_list *clone() const {
        jo_persistent_list *copy = new jo_persistent_list();
        jo_shared_ptr<node> cur = head;
        jo_shared_ptr<node> cur_copy = NULL;
        jo_shared_ptr<node> prev = NULL;
        copy->head = NULL;
        while(cur) {
            cur_copy = new node(cur->value, NULL);
            if(prev) {
                prev->next = cur_copy;
            } else {
                copy->head = cur_copy;
            }
            prev = cur_copy;
            cur = cur->next;
        }
        copy->tail = prev;
        copy->length = length;
        return copy;
    }

    // Clone up to and including the given index and link the rest of the list to the new list.
    jo_persistent_list *clone(int depth) const {
        jo_persistent_list *copy = new jo_persistent_list();
        jo_shared_ptr<node> cur = head;
        jo_shared_ptr<node> cur_copy = NULL;
        jo_shared_ptr<node> prev = NULL;
        while(cur && depth >= 0) {
            cur_copy = new node(cur->value, NULL);
            if(prev) {
                prev->next = cur_copy;
            } else {
                copy->head = cur_copy;
            }
            prev = cur_copy;
            cur = cur->next;
            depth--;
        }
        if(cur) {
            prev->next = cur;
            copy->tail = tail;
        } else {
            copy->tail = prev;
        }
        copy->length = length;
        return copy;
    }

    jo_persistent_list *conj(const jo_persistent_list &other) const {
        jo_persistent_list *copy = clone();
        if(copy->tail) {
            copy->tail->next = other.head;
        } else {
            copy->head = other.head;
        }
        copy->tail = other.tail;
        copy->length += other.length;
        return copy;
    }

    jo_persistent_list *conj_inplace(const jo_persistent_list &other) {
        if(tail) {
            tail->next = other.head;
        } else {
            head = other.head;
        }
        tail = other.tail;
        length += other.length;
        return this;
    }

    // conj a value
    jo_persistent_list *conj(const T &value) const {
        jo_persistent_list *copy = clone();
        if(copy->tail) {
            copy->tail->next = new node(value, NULL);
            copy->tail = copy->tail->next;
        } else {
            copy->head = new node(value, NULL);
            copy->tail = copy->head;
        }
        copy->length++;
        return copy;
    }

    jo_persistent_list *conj_inplace(const T &value) {
        if(tail) {
            tail->next = new node(value, NULL);
            tail = tail->next;
        } else {
            head = new node(value, NULL);
            tail = head;
        }
        length++;
        return this;
    }

    jo_persistent_list *disj_inplace(const jo_persistent_list &other) {
        jo_shared_ptr<node> cur = head;
        jo_shared_ptr<node> prev = NULL;
        while(cur) {
            if(other.contains(cur->value)) {
                if(prev) {
                    prev->next = cur->next;
                } else {
                    head = cur->next;
                }
                length--;
                if(!cur->next) {
                    tail = prev;
                }
            } else {
                prev = cur;
            }
            cur = cur->next;
        }
        return this;
    }

    jo_persistent_list *disj(const jo_persistent_list &other) const {
        jo_persistent_list *copy = clone();
        copy->disj_inplace(other);
        return copy;
    }

    // disj with lambda
    template<typename F>
    jo_persistent_list *disj_inplace(F f) {
        jo_shared_ptr<node> cur = head;
        jo_shared_ptr<node> prev = NULL;
        while(cur) {
            if(f(cur->value)) {
                if(prev) {
                    prev->next = cur->next;
                } else {
                    head = cur->next;
                }
                length--;
                if(!cur->next) {
                    tail = prev;
                }
            } else {
                prev = cur;
            }
            cur = cur->next;
        }
        return this;
    }

    template<typename F>
    jo_persistent_list *disj(F f) const {
        jo_persistent_list *copy = clone();
        copy->disj_inplace(f);
        return copy;
    }

    jo_persistent_list *assoc(size_t index, const T &value) const {
        jo_persistent_list *copy = new jo_persistent_list();
        jo_shared_ptr<node> cur = head;
        jo_shared_ptr<node> cur_copy = NULL;
        jo_shared_ptr<node> prev = NULL;
        while(cur && index >= 0) {
            cur_copy = new node(cur->value, NULL);
            if(prev) {
                prev->next = cur_copy;
            } else {
                copy->head = cur_copy;
            }
            prev = cur_copy;
            cur = cur->next;
            index--;
        }
        prev->value = value;
        if(cur) {
            prev->next = cur;
            copy->tail = tail;
        } else {
            copy->tail = prev;
        }
        copy->length = length;
        return copy;
    }

    jo_persistent_list *erase_node(jo_shared_ptr<node> at) const {
        jo_persistent_list *copy = new jo_persistent_list();
        jo_shared_ptr<node> cur = head;
        jo_shared_ptr<node> cur_copy = NULL;
        jo_shared_ptr<node> prev = NULL;
        while(cur && cur != at) {
            cur_copy = new node(cur->value, NULL);
            if(prev) {
                prev->next = cur_copy;
            } else {
                copy->head = cur_copy;
            }
            prev = cur_copy;
            cur = cur->next;
        }
        if(cur) {
            if(prev) {
                prev->next = cur->next;
            } else {
                copy->head = cur->next;
            }
            copy->tail = tail;
        } else {
            copy->tail = prev;
        }
        copy->length = length-1;
        return copy;
    }

    // pop off value from front
    jo_persistent_list *pop() const {
        jo_persistent_list *copy = new jo_persistent_list(*this);
        if(head) {
            copy->head = head->next;
            if(!copy->head) {
                copy->tail = NULL;
            }
            copy->length = length - 1;
        }
        return copy;
    }

    const T &nth(int index) const {
        jo_shared_ptr<node> cur = head;
        while(cur && index > 0) {
            cur = cur->next;
            index--;
        }
        if(!cur) {
            throw jo_exception("nth");
        }
        return cur->value;
    }

    const T &operator[](int index) const {
        return nth(index);
    }

    jo_persistent_list *rest() const {
        jo_persistent_list *copy = new jo_persistent_list();
        if(head) {
            copy->head = head->next;
            copy->tail = tail;
            copy->length = length - 1;
        }
        return copy;
    }

    jo_persistent_list *first() const {
        // copy head node
        jo_persistent_list *copy = new jo_persistent_list();
        if(head) {
            copy->head = new node(head->value, NULL);
            copy->tail = copy->head;
            copy->length = 1;
        }
        return copy;
    }

    T first_value() const {
        if(!head) {
            return T();
        }
        return head->value;
    }

    T second_value() const {
        if(!head || !head->next) {
            return T();
        }
        return head->next->value;
    }

    T third_value() const {
        if(!head || !head->next || !head->next->next) {
            return T();
        }
        return head->next->next->value;
    }

    T last_value() const {
        if(!tail) {
            return T();
        }
        return tail->value;
    }

    jo_persistent_list *drop(int index) const {
        jo_persistent_list *copy = new jo_persistent_list();
        jo_shared_ptr<node> cur = head;
        while(cur && --index > 0) {
            cur = cur->next;
        }
        if(cur) {
            copy->head = cur->next;
            copy->tail = tail;
            copy->length = length - index;
        }
        return copy;
    }

    // push_back 
    jo_persistent_list *push_back_inplace(const T &value) {
        if(!head) {
            head = new node(value, NULL);
            tail = head;
        } else {
            tail->next = new node(value, NULL);
            tail = tail->next;
        }
        length++;
        return this;
    }

    jo_persistent_list *push_back(const T &value) const {
        jo_persistent_list *copy = clone();
        copy->push_back_inplace(value);
        return copy;
    }

    // push_front inplace
    jo_persistent_list *push_front_inplace(const T &value) {
        if(!head) {
            head = new node(value, NULL);
            tail = head;
        } else {
            jo_shared_ptr<node> new_head = new node(value, head);
            head = new_head;
        }
        length++;
        return this;
    }

    jo_persistent_list *push_front(const T &value) const {
        jo_persistent_list *copy = new jo_persistent_list(*this);
        copy->push_front_inplace(value);
        return copy;
    }

    jo_persistent_list *push_front2_inplace(const T &value1, const T &value2) {
        push_front_inplace(value2);
        push_front_inplace(value1);
        return this;
    }

    jo_persistent_list *push_front2(const T &value1, const T &value2) const {
        jo_persistent_list *copy = new jo_persistent_list(*this);
        copy->push_front2_inplace(value1, value2);
        return copy;
    }

    jo_persistent_list *push_front3_inplace(const T &value1, const T &value2, const T &value3) {
        push_front_inplace(value3);
        push_front_inplace(value2);
        push_front_inplace(value1);
        return this;
    }

    jo_persistent_list *push_front3(const T &value1, const T &value2, const T &value3) const {
        jo_persistent_list *copy = new jo_persistent_list(*this);
        copy->push_front3_inplace(value1, value2, value3);
        return copy;
    }

    jo_persistent_list *push_front4_inplace(const T &value1, const T &value2, const T &value3, const T &value4) {
        push_front_inplace(value4);
        push_front_inplace(value3);
        push_front_inplace(value2);
        push_front_inplace(value1);
        return this;
    }

    jo_persistent_list *push_front4(const T &value1, const T &value2, const T &value3, const T &value4) const {
        jo_persistent_list *copy = new jo_persistent_list(*this);
        copy->push_front4_inplace(value1, value2, value3, value4);
        return copy;
    }

    jo_persistent_list *push_front5_inplace(const T &value1, const T &value2, const T &value3, const T &value4, const T &value5) {
        push_front_inplace(value5);
        push_front_inplace(value4);
        push_front_inplace(value3);
        push_front_inplace(value2);
        push_front_inplace(value1);
        return this;
    }

    jo_persistent_list *push_front5(const T &value1, const T &value2, const T &value3, const T &value4, const T &value5) const {
        jo_persistent_list *copy = new jo_persistent_list(*this);
        copy->push_front5_inplace(value1, value2, value3, value4, value5);
        return copy;
    }

    jo_persistent_list *push_front6_inplace(const T &value1, const T &value2, const T &value3, const T &value4, const T &value5, const T &value6) {
        push_front_inplace(value6);
        push_front_inplace(value5);
        push_front_inplace(value4);
        push_front_inplace(value3);
        push_front_inplace(value2);
        push_front_inplace(value1);
        return this;
    }

    jo_persistent_list *push_front6(const T &value1, const T &value2, const T &value3, const T &value4, const T &value5, const T &value6) const {
        jo_persistent_list *copy = new jo_persistent_list(*this);
        copy->push_front6_inplace(value1, value2, value3, value4, value5, value6);
        return copy;
    }

    jo_persistent_list *push_front(jo_persistent_list &other) const {
        jo_persistent_list *copy = new jo_persistent_list(*this);
        jo_shared_ptr<node> cur = other.head;
        while(cur) {
            copy->push_front_inplace(cur->value);
            cur = cur->next;
        }
        return copy;
    }

    jo_persistent_list *pop_front_inplace() {
        if(head) {
            head = head->next;
            if(!head) {
                tail = NULL;
            }
            length--;
        }
        return this;
    }

    jo_persistent_list *pop_front_inplace(size_t num) {
        while(num-- > 0 && head) {
            head = head->next;
            if(!head) {
                tail = NULL;
            }
            length--;
        }
        return this;
    }

    jo_persistent_list *pop_front() const {
        jo_persistent_list *copy = new jo_persistent_list(*this);
        copy->pop_front_inplace();
        return copy;
    }

    jo_persistent_list *pop_front(size_t num) const {
        jo_persistent_list *copy = new jo_persistent_list(*this);
        copy->pop_front_inplace(num);
        return copy;
    }

    jo_persistent_list *subvec(int start, int end) const {
        jo_shared_ptr<node> cur = head;
        while(cur && start > 0) {
            cur = cur->next;
            start--;
        }
        jo_persistent_list *copy = new jo_persistent_list();
        while(cur && end > start) {
            copy->push_back_inplace(cur->value);
            cur = cur->next;
            end--;
        }
        return copy;
    }

    // pop_back
    jo_persistent_list *pop_back_inplace() {
        if(head) {
            if(head == tail) {
                head = tail = NULL;
            } else {
                jo_shared_ptr<node> cur = head;
                while(cur->next != tail) {
                    cur = cur->next;
                }
                tail = cur;
                tail->next = NULL;
            }
            length--;
        }
        return this;
    }

    jo_persistent_list *pop_back() const {
        jo_persistent_list *copy = new jo_persistent_list(*this);
        copy->pop_back_inplace();
        return copy;
    }

    bool contains(const T &value) const {
        jo_shared_ptr<node> cur = head;
        while(cur) {
            if(cur->value == value) {
                return true;
            }
            cur = cur->next;
        }
        return false;
    }

    // contains with lambda for comparison
    template<typename F>
    bool contains(const F &f) const {
        jo_shared_ptr<node> cur = head;
        while(cur) {
            if(f(cur->value)) {
                return true;
            }
            cur = cur->next;
        }
        return false;
    }

    jo_persistent_list *erase(const T &value) {
        jo_shared_ptr<node> cur = head;
        jo_shared_ptr<node> prev = NULL;
        while(cur) {
            if(cur->value == value) {
                return erase_node(cur);
            }
            prev = cur;
            cur = cur->next;
        }
        return this;
    }

    jo_persistent_list *take(int N) const {
        jo_persistent_list *copy = new jo_persistent_list();
        jo_shared_ptr<node> cur = head;
        while(cur && N > 0) {
            copy->push_back_inplace(cur->value);
            cur = cur->next;
            N--;
        }
        return copy;
    }

    jo_persistent_list *take_last(int N) const {
        return drop(length - N);
    }

    // return a random permutation of the elements of the list
    jo_persistent_list *shuffle() const {
        // convert to jo_vector
        jo_vector<T> v;
        jo_shared_ptr<node> cur = head;
        while(cur) {
            v.push_back(cur->value);
            cur = cur->next;
        }
        // shuffle
        jo_random_shuffle(v.begin(), v.end());
        // convert back to jo_persistent_list
        jo_persistent_list *copy = new jo_persistent_list();
        for(int i = 0; i < v.size(); i++) {
            copy->push_back_inplace(v[i]);
        }
        return copy;
    }

    // return items with a random probability of p
    jo_persistent_list *random_sample(float p) const { 
        if(p <= 0.0f) {
            return new jo_persistent_list();
        }
        if(p >= 1.0f) {
            return new jo_persistent_list(*this);
        }
        jo_persistent_list *copy = new jo_persistent_list();
        jo_shared_ptr<node> cur = head;
        while(cur) {
            if(jo_random_float() < p) {
                copy->push_back_inplace(cur->value);
            }
            cur = cur->next;
        }
        return copy;
    }

    // iterator
    struct iterator {
        jo_shared_ptr<node> cur;
        int index;

        iterator() : cur(NULL), index() {}
        iterator(jo_shared_ptr<node> cur) : cur(cur), index() {}
        iterator(const iterator &other) : cur(other.cur), index(other.index) {}
        iterator &operator=(const iterator &other) {
            cur = other.cur;
            index = other.index;
            return *this;
        }
        bool operator==(const iterator &other) const {
            return cur == other.cur;
        }
        bool operator!=(const iterator &other) const {
            return cur != other.cur;
        }
        operator bool() const {
            return cur;
        }
        iterator &operator++() {
            if(cur) {
                cur = cur->next;
                ++index;
            }
            return *this;
        }
        iterator operator++(int) {
            iterator copy = *this;
            if(cur) {
                cur = cur->next;
                ++index;
            }
            return copy;
        }
        iterator operator+=(int n) {
            while(cur && n > 0) {
                cur = cur->next;
                n--;
                ++index;
            }
            return *this;
        }
        iterator operator+(int n) const {
            iterator copy = *this;
            copy += n;
            return copy;
        }
        T &operator*() {
            return cur->value;
        }
        T *operator->() {
            return &cur->value;
        }
        const T &operator*() const {
            return cur->value;
        }
        const T *operator->() const {
            return &cur->value;
        }

    };

    iterator begin() const {
        return iterator(head);
    }

    iterator end() const {
        return iterator(tail);
    }

    jo_persistent_list *rest(const iterator &it) const {
        jo_persistent_list *copy = new jo_persistent_list();
        copy->head = it.cur;
        copy->tail = tail;
        copy->length = length - it.index;
        return copy;
    }

    const T &front() const {
        return head->value;
    }

    const T &back() const {
        return tail->value;
    }

    size_t size() const {
        return length;
    }

    bool empty() const {
        return !head;
    }

    void clear() {
        head = NULL;
        tail = NULL;
        length = 0;
    }

    jo_persistent_list *erase(const iterator &it) const {
        return erase(it.cur);
    }
};

// jo_hash
template<typename T>
struct jo_hash {
    size_t operator()(const T &value) const {
        return jo_hash_value(value);
    }
};

// jo_hash_value
template<typename T> size_t jo_hash_value(T value);

template<> size_t jo_hash_value(const bool &value) { return value ? 1 : 0; }
template<> size_t jo_hash_value(const char &value) { return value; }
template<> size_t jo_hash_value(const unsigned char &value) { return value; }
template<> size_t jo_hash_value(const short &value) { return value; }
template<> size_t jo_hash_value(const unsigned short &value) { return value; }
template<> size_t jo_hash_value(const int &value) { return value; }
template<> size_t jo_hash_value(const unsigned int &value) { return value; }
template<> size_t jo_hash_value(const long &value) { return value; }
template<> size_t jo_hash_value(const unsigned long &value) { return value; }
template<> size_t jo_hash_value(const long long &value) { return value; }
template<> size_t jo_hash_value(const unsigned long long &value) { return value; }
template<> size_t jo_hash_value(const double value) { return *(size_t *)&value; }
template<> size_t jo_hash_value(const long double &value) { return *(size_t *)&value; }
template<> size_t jo_hash_value(const char *value) {
    size_t hash = 0;
    while(*value) {
        hash = hash * 31 + *value;
        value++;
    }
    return hash;
}
template<> size_t jo_hash_value(const jo_string &value) { return jo_hash_value(value.c_str()); }

// jo_persistent_unordered_map is a persistent hash map
// that uses a persistent vector as the underlying data structure
// for the map.
template<typename K, typename V>
struct jo_persistent_unordered_map {
    // the way this works is that you make your hash value of K and
    // then use that as an index into the vector.
    typedef jo_triple<K, V, bool> entry_t;
    // vec is used to store the keys and values
    jo_persistent_vector< entry_t > vec;
    // number of entries actually used in the hash table
    size_t length;

    jo_persistent_unordered_map() : vec(16), length() {}
    jo_persistent_unordered_map(const jo_persistent_unordered_map &other) : vec(other.vec), length(other.length) {}
    jo_persistent_unordered_map &operator=(const jo_persistent_unordered_map &other) {
        vec = other.vec;
        length = other.length;
        return *this;
    }

    size_t size() const {
        return length;
    }

    bool empty() const {
        return !length;
    }

    // iterator
    class iterator {
        typename jo_persistent_vector< entry_t >::iterator cur;
    public:
        iterator(const typename jo_persistent_vector< entry_t >::iterator &it) : cur(it) {
            // find first non-erased entry
            while(cur && !cur->third) {
                cur++;
            }
        }
        iterator() : cur() {}
        iterator &operator++() {
            if(cur) do {
                cur++;
            } while(cur && !cur->third);
            return *this;
        }
        iterator operator++(int) {
            iterator ret = *this;
            if(cur) do {
                cur++;
            } while(cur && !cur->third);
            return ret;
        }
        bool operator==(const iterator &other) const {
            return cur == other.cur;
        }
        bool operator!=(const iterator &other) const {
            return cur != other.cur;
        }
        const entry_t &operator*() const {
            return *cur;
        }
        const entry_t *operator->() const {
            return &*cur;
        }
        iterator operator+(int i) const {
            iterator ret = *this;
            ret.cur += i;
            return ret;
        }
        operator bool() const {
            return cur;
        }
    };

    iterator begin() { return iterator(vec.begin()); }
    iterator begin() const { return iterator(vec.begin()); }
    iterator end() { return iterator(vec.end()); }
    iterator end() const { return iterator(vec.end()); }

    jo_persistent_unordered_map *resize(size_t new_size) const {
        jo_persistent_unordered_map *copy = new jo_persistent_unordered_map();
        copy->vec = jo_persistent_vector< entry_t >(new_size);
        for(iterator it = begin(); it; ++it) {
            int index = jo_hash_value(it->first) % new_size;
            while(copy->vec[index].third) {
                index = (index + 1) % new_size;
            }
            copy->vec.assoc_inplace(index, entry_t(it->first, it->second, true));
        }
        return copy;
    }

    jo_persistent_unordered_map *resize_inplace(size_t new_size) {
        jo_persistent_vector<entry_t> new_vec(new_size);
        for(iterator it = begin(); it; ++it) {
            auto entry = *it;
            int index = jo_hash_value(entry.first) % new_size;
            while(new_vec[index].third) {
                index = (index + 1) % new_size;
            }
            new_vec.assoc_inplace(index, entry_t(entry.first, entry.second, true));
        }
        vec = new_vec;
        return this;
    }

    // remove a value from the map
    jo_persistent_unordered_map *erase(const K &key) const {
        jo_persistent_unordered_map *copy = new jo_persistent_unordered_map(*this);
        int index = jo_hash_value(key) % copy->vec.size();
        while(copy->vec[index].third) {
            if(copy->vec[index].first == key) {
                copy->vec.assoc_inplace(index, entry_t(copy->vec[index].first, V(), true));
                return copy;
            }
            index = (index + 1) % copy->vec.size();
        } 
        return copy;
    }

    // insert a new value into the map, if the value already exists, replaces it
    jo_persistent_unordered_map *assoc(const K &key, const V &value) const {
        jo_persistent_unordered_map *copy = new jo_persistent_unordered_map(*this);
        if(copy->vec.size() - copy->length < copy->vec.size() / 8) {
            copy->resize_inplace(copy->vec.size() * 2);
        }
        int index = jo_hash_value(key) % copy->vec.size();
        while(copy->vec[index].third) {
            if(copy->vec[index].first == key) {
                copy->vec.assoc_inplace(index, entry_t(key, value, true));
                return copy;
            }
            index = (index + 1) % copy->vec.size();
        } 
        copy->vec.assoc_inplace(index, entry_t(key, value, true));
        ++copy->length;
        return copy;
    }

    // assoc with lambda for equality
    template<typename F>
    jo_persistent_unordered_map *assoc(const K &key, const V &value, F eq) const {
        jo_persistent_unordered_map *copy = new jo_persistent_unordered_map(*this);
        if(copy->vec.size() - copy->length < copy->vec.size() / 8) {
            copy->resize_inplace(copy->vec.size() * 2);
        }
        int index = jo_hash_value(key) % copy->vec.size();
        while(copy->vec[index].third) {
            if(eq(copy->vec[index].first, key)) {
                copy->vec.assoc_inplace(index, entry_t(key, value, true));
                return copy;
            }
            index = (index + 1) % copy->vec.size();
        } 
        copy->vec.assoc_inplace(index, entry_t(key, value, true));
        ++copy->length;
        return copy;
    }

    // remove a value from the map
    jo_persistent_unordered_map *erase(const iterator &it) const {
        return erase(it.cur->first);
    }

    // dissoc
    jo_persistent_unordered_map *dissoc(const K &key) const {
        jo_persistent_unordered_map *copy = new jo_persistent_unordered_map(*this);
        int index = jo_hash_value(key) % copy->vec.size();
        while(copy->vec[index].third) {
            if(copy->vec[index].first == key) {
                copy->vec.assoc_inplace(index, entry_t(copy->vec[index].first, V(), false));
                --copy->length;
                return copy;
            }
            index = (index + 1) % copy->vec.size();
        } 
        return copy;
    }

    // dissoc with lambda for equality
    template<typename F>
    jo_persistent_unordered_map *dissoc(const K &key, F eq) const {
        jo_persistent_unordered_map *copy = new jo_persistent_unordered_map(*this);
        int index = jo_hash_value(key) % copy->vec.size();
        while(copy->vec[index].third) {
            if(eq(copy->vec[index].first, key)) {
                copy->vec.assoc_inplace(index, entry_t(copy->vec[index].first, V(), false));
                --copy->length;
                return copy;
            }
            index = (index + 1) % copy->vec.size();
        } 
        return copy;
    }

    // find a value in the map
    entry_t find(const K &key) {
        if(vec.size() == 0) {
            return entry_t();
        }
        size_t index = jo_hash_value(key) % vec.size();
        while(vec[index].third) {
            if(vec[index].first == key) {
                return vec[index];
            }
            index = (index + 1) % vec.size();
        } 
        return entry_t();
    }

    // find using lambda
    template<typename F>
    entry_t find(const K &key, const F &f) {
        if(vec.size() == 0) {
            return entry_t();
        }
        size_t index = jo_hash_value(key) % vec.size();
        while(vec[index].third) {
            if(f(vec[index].first, key)) {
                return vec[index];
            }
            index = (index + 1) % vec.size();
        } 
        return entry_t();
    }

    bool contains(const K &key) {
        return find(key).third;
    }
     
    // contains using lambda
    template<typename F>
    bool contains(const K &key, const F &f) {
        return find(key, f).third;
    }

    V get(const K &key) const {
        if(vec.size() == 0) {
            return V();
        }
        size_t index = jo_hash_value(key) % vec.size();
        auto it = vec.begin() + index;
        while(it && it->first != key && it->third) {
            it++;
        }
        if(!it) {
            return V();
        }
        return it->second;
    }

    template<typename F>
    V get(const K &key, const F &f) const {
        if(vec.size() == 0) {
            return V();
        }
        size_t index = jo_hash_value(key) % vec.size();
        auto it = vec.begin() + index;
        while(it && !f(it->first, key) && it->third) {
            it++;
        }
        if(!it) {
            return V();
        }
        return it->second;
    }

    // conj with lambda
    template<typename F>
    jo_shared_ptr<jo_persistent_unordered_map> conj(jo_persistent_unordered_map *other, F eq) const {
        jo_shared_ptr<jo_persistent_unordered_map> copy = new jo_persistent_unordered_map(*this);
        for(auto it = other->vec.begin(); it; ++it) {
            if(it->third) {
                copy = copy->assoc(it->first, it->second, eq);
            }
        }
        return copy;
    }

    jo_persistent_vector<V> *to_vector() const {
        jo_persistent_vector<V> *vec = new jo_persistent_vector<V>();
        for(auto it = this->vec.begin(); it; ++it) {
            if(it->third) {
                vec->conj_inplace(it->second);
            }
        }
        return vec;
    }

    entry_t first() const {
        for(auto it = this->vec.begin(); it; ++it) {
            if(it->third) {
                return *it;
            }
        }
        return entry_t();
    }

    V first_value() const {
        for(auto it = vec.begin(); it; ++it) {
            if(it->third) {
                return it->second;
            }
        }
        return V();
    }

    K last_value() const {
        for(long long i = vec.size()-1; i >= 0; --i) {
            auto entry = vec[i];
            if(entry.third) {
                return entry.second;
            }
        }
        return K();
    }

    // TODO: this is not fast... speedup by caching first value index.
    jo_persistent_unordered_map *rest() const {
        jo_persistent_unordered_map *copy = new jo_persistent_unordered_map(*this);
        for(size_t i = 0; i < vec.size(); ++i) {
            if(vec[i].third) {
                copy->vec.assoc_inplace(i, entry_t(vec[i].first, V(), false));
                --copy->length;
                break;
            }
        }
        return copy;
    }
};

// jo_persistent_unordered_set
template<typename K>
class jo_persistent_unordered_set {
    // the way this works is that you make your hash value of K and
    // then use that as an index into the vector.
    typedef jo_pair<K, bool> entry_t;
    // vec is used to store the keys and values
    jo_persistent_vector< entry_t > vec;
    // number of entries actually used in the hash table
    size_t length;

public:
    jo_persistent_unordered_set() : vec(32), length() {}
    jo_persistent_unordered_set(const jo_persistent_unordered_set &other) : vec(other.vec), length(other.length) {}
    jo_persistent_unordered_set &operator=(const jo_persistent_unordered_set &other) {
        vec = other.vec;
        length = other.length;
        return *this;
    }

    size_t size() const {
        return length;
    }

    bool empty() const {
        return !length;
    }

    // iterator
    class iterator {
        typename jo_persistent_vector< entry_t >::iterator cur;
    public:
        iterator(const typename jo_persistent_vector< entry_t >::iterator &it) : cur(it) {
            // find first non-erased entry
            while(cur && !cur->second) {
                cur++;
            }
        }
        iterator() : cur() {}
        iterator &operator++() {
            if(cur) do {
                cur++;
            } while(cur && !cur->second);
            return *this;
        }
        iterator operator++(int) {
            iterator ret = *this;
            if(cur) do {
                cur++;
            } while(cur && !cur->second);
            return ret;
        }
        bool operator==(const iterator &other) const {
            return cur == other.cur;
        }
        bool operator!=(const iterator &other) const {
            return cur != other.cur;
        }
        const entry_t &operator*() const {
            return *cur;
        }
        const entry_t *operator->() const {
            return &*cur;
        }
        iterator operator+(int i) const {
            iterator ret = *this;
            ret.cur += i;
            return ret;
        }
        operator bool() const {
            return cur;
        }
    };

    iterator begin() { return iterator(vec.begin()); }
    iterator begin() const { return iterator(vec.begin()); }
    iterator end() { return iterator(vec.end()); }
    iterator end() const { return iterator(vec.end()); }

    jo_persistent_unordered_set *resize(size_t new_size) const {
        jo_persistent_unordered_set *copy = new jo_persistent_unordered_set();
        copy->vec = jo_persistent_vector< entry_t >(new_size);
        for(iterator it = begin(); it; ++it) {
            int index = jo_hash_value(it->first) % new_size;
            while(copy->vec[index].second) {
                index = (index + 1) % new_size;
            }
            copy->vec.assoc_inplace(index, entry_t(it->first, true));
        }
        return copy;
    }

    jo_persistent_unordered_set *resize_inplace(size_t new_size) {
        jo_persistent_vector<entry_t> new_vec(new_size);
        for(iterator it = begin(); it; ++it) {
            int index = jo_hash_value(it->first) % new_size;
            while(new_vec[index].second) {
                index = (index + 1) % new_size;
            }
            new_vec.assoc_inplace(index, entry_t(it->first, true));
        }
        vec = new_vec;
        return this;
    }

    // remove a value from the set
    jo_persistent_unordered_set *erase(const K &key) const {
        jo_persistent_unordered_set *copy = new jo_persistent_unordered_set(*this);
        int index = jo_hash_value(key) % vec.size();
        while(copy->vec[index].second) {
            if(copy->vec[index].first == key) {
                copy->vec.assoc_inplace(index, entry_t(copy->vec[index].first, true));
                return copy;
            }
            index = (index + 1) % vec.size();
        } 
        return copy;
    }

    // insert a new value into the set, if the value already exists, replaces it
    jo_persistent_unordered_set *assoc(const K &key) const {
        jo_persistent_unordered_set *copy = new jo_persistent_unordered_set(*this);
        if(copy->vec.size() - copy->length < copy->vec.size() / 8) {
            copy->resize_inplace(copy->vec.size() * 2);
        }
        int index = jo_hash_value(key) % copy->vec.size();
        while(copy->vec[index].second) {
            if(copy->vec[index].first == key) {
                copy->vec.assoc_inplace(index, entry_t(key, true));
                return copy;
            }
            index = (index + 1) % copy->vec.size();
        } 
        copy->vec.assoc_inplace(index, entry_t(key, true));
        ++copy->length;
        return copy;
    }

    // assoc with lambda for equality
    template<typename F>
    jo_persistent_unordered_set *assoc(const K &key, F eq) const {
        jo_persistent_unordered_set *copy = new jo_persistent_unordered_set(*this);
        if(copy->vec.size() - copy->length < copy->vec.size() / 8) {
            copy->resize_inplace(copy->vec.size() * 2);
        }
        int index = jo_hash_value(key) % copy->vec.size();
        while(copy->vec[index].second) {
            if(eq(copy->vec[index].first, key)) {
                copy->vec.assoc_inplace(index, entry_t(key, true));
                return copy;
            }
            index = (index + 1) % copy->vec.size();
        } 
        copy->vec.assoc_inplace(index, entry_t(key, true));
        ++copy->length;
        return copy;
    }

    // remove a key from the set
    jo_persistent_unordered_set *erase(const iterator &it) const {
        return erase(it.cur->first);
    }

    // dissoc
    jo_persistent_unordered_set *dissoc(const K &key) const {
        jo_persistent_unordered_set *copy = new jo_persistent_unordered_set(*this);
        int index = jo_hash_value(key) % copy->vec.size();
        while(copy->vec[index].second) {
            if(copy->vec[index].first == key) {
                copy->vec.assoc_inplace(index, entry_t(copy->vec[index].first, false));
                --copy->length;
                return copy;
            }
            index = (index + 1) % copy->vec.size();
        } 
        return copy;
    }

    // dissoc with lambda for equality
    template<typename F>
    jo_persistent_unordered_set *dissoc(const K &key, F eq) const {
        jo_persistent_unordered_set *copy = new jo_persistent_unordered_set(*this);
        int index = jo_hash_value(key) % copy->vec.size();
        while(copy->vec[index].second) {
            if(eq(copy->vec[index].first, key)) {
                copy->vec.assoc_inplace(index, entry_t(copy->vec[index].first, false));
                --copy->length;
                return copy;
            }
            index = (index + 1) % copy->vec.size();
        } 
        return copy;
    }

    // find a key in the set
    entry_t find(const K &key) {
        if(vec.size() == 0) {
            return entry_t();
        }
        size_t index = jo_hash_value(key) % vec.size();
        while(vec[index].second) {
            if(vec[index].first == key) {
                return vec[index];
            }
            index = (index + 1) % vec.size();
        } 
        return entry_t();
    }

    // find using lambda
    template<typename F>
    entry_t find(const K &key, const F &f) {
        if(vec.size() == 0) {
            return entry_t();
        }
        size_t index = jo_hash_value(key) % vec.size();
        while(vec[index].second) {
            if(f(vec[index].first, key)) {
                return vec[index];
            }
            index = (index + 1) % vec.size();
        } 
        return entry_t();
    }

    bool contains(const K &key) {
        return find(key).second;
    }
     
    // contains using lambda
    template<typename F>
    bool contains(const K &key, const F &f) {
        return find(key, f).second;
    }

    // conj
    jo_persistent_unordered_set *conj(jo_persistent_unordered_set *other) const {
        jo_persistent_unordered_set *copy = new jo_persistent_unordered_set(*this);
        for(auto it = other->vec.begin(); it; ++it) {
            if(it->second) {
                copy->assoc_inplace(it->first);
            }
        }
        return copy;
    }

    jo_persistent_vector<K> *to_vector() const {
        jo_persistent_vector<K> *vec = new jo_persistent_vector<K>();
        for(auto it = this->vec.begin(); it; ++it) {
            if(it->second) {
                vec->conj_inplace(it->first);
            }
        }
        return vec;
    }

    K first_value() const {
        for(auto it = vec.begin(); it; ++it) {
            if(it->second) {
                return it->first;
            }
        }
        return K();
    }

    K last_value() const {
        for(long long i = vec.size()-1; i >= 0; --i) {
            auto entry = vec[i];
            if(entry.second) {
                return entry.first;
            }
        }
        return K();
    }

    // TODO: this is not fast... speedup by caching first key index.
    jo_persistent_unordered_set *rest() const {
        jo_persistent_unordered_set *copy = new jo_persistent_unordered_set(*this);
        for(auto it = this->vec.begin(); it; ++it) {
            if(it->second) {
                copy->vec.assoc_inplace(it - copy->vec.begin(), entry_t(it->first, false));
                --copy->length;
                break;
            }
        }
        return copy;
    }
};



static const char *va(const char *fmt, ...) {
    static thread_local char tmp[0x10000];
    static thread_local int at = 0;
    char *ret = tmp+at;
    va_list args;
    va_start(args, fmt);
    at += 1 + vsnprintf(ret, sizeof(tmp)-at-1, fmt, args);
    va_end(args);
    if(at > sizeof(tmp) - 0x400) {
        at = 0;
    }
    return ret;
}

// Arbitrary precision integer
struct jo_bigint {
    jo_shared_ptr<jo_persistent_vector<int> > digits;
    bool negative;

    jo_bigint() : negative(false) {
        digits = new jo_persistent_vector<int>();
        digits->append_inplace(0);
    }
    jo_bigint(int n) : negative(n < 0) {
        digits = new jo_persistent_vector<int>();
        if(n < 0) {
            n = -n;
        }
        do {
            digits->append_inplace(n % 10);
            n /= 10;
        } while(n);
    }
    jo_bigint(const jo_bigint &other) : digits(other.digits), negative(other.negative) {}
    jo_bigint(const char *str) : negative(false) {
        digits = new jo_persistent_vector<int>();
        if(str[0] == '-') {
            negative = true;
            str++;
        }
        for(int i = strlen(str)-1; i >= 0; i--) {
            digits->append_inplace(str[i] - '0');
        }
    }
    jo_bigint(const char *str, int base) {
        if(base < 2 || base > 36) {
            throw jo_exception("jo_bigint: invalid base");
        }
        digits = new jo_persistent_vector<int>();
        negative = false;
        while(*str) {
            int digit = 0;
            digit = *str - '0';
            if(digit > 9) {
                digit = *str - 'a' + 10;
            }
            if(digit > 35) {
                digit = *str - 'A' + 10;
            }
            if(digit >= base) {
                throw jo_exception("jo_bigint: invalid digit");
            }
            digits->append_inplace(digit);
            str++;
        }
    }
    jo_bigint &operator=(const jo_bigint &other) {
        digits = other.digits;
        negative = other.negative;
        return *this;
    }
    jo_bigint &operator=(int n) {
        negative = n < 0;
        if(n < 0) {
            n = -n;
        }
        digits = digits->clear();
        while(n) {
            digits = digits->push_back(n % 10);
            n /= 10;
        }
        return *this;
    }
    jo_bigint &operator+=(const jo_bigint &other) {
        if(negative == other.negative) {
            int carry = 0;
            for(int i = 0; i < digits->size() || i < other.digits->size(); i++) {
                int a = i < digits->size() ? digits->nth(i) : 0;
                int b = i < other.digits->size() ? other.digits->nth(i) : 0;
                int sum = a + b + carry;
                carry = sum / 10;
                digits = digits->assoc(i, sum % 10);
            }
            if(carry) {
                digits = digits->push_back(carry);
            }
        } else {
            if(negative) {
                // -20 + 11 = -9 => 11 - 20 = -9
                *this = other - (-*this);
            } else {
                jo_bigint other_copy = other;
                other_copy.negative = false;
                *this -= other_copy;
            }
        }
        return *this;
    }

    jo_bigint &operator-=(const jo_bigint &other) {
        if(negative == other.negative) {
            if(negative) {
                *this += -other;
            } else {
                if(*this < other) {
                    jo_bigint other_copy = other;
                    other_copy.negative = false;
                    *this = other_copy - *this;
                    negative = true;
                } else {
                    //printf("%s - %s\n", to_string().c_str(), other.to_string().c_str());
                    int carry = 0;
                    for(int i = 0; i < digits->size() || i < other.digits->size(); i++) {
                        int a = i < digits->size() ? digits->nth(i) : 0;
                        int b = i < other.digits->size() ? other.digits->nth(i) : 0;
                        int diff = a - b - carry;
                        if(diff < 0) {
                            diff += 10;
                            carry = 1;
                        } else {
                            carry = 0;
                        }
                        digits = digits->assoc(i, diff);
                    }
                    negative = carry;
                    if(carry) {
                        digits = digits->pop_back();
                    } else {
                        // Remove trailing zeros
                        while(digits->size() > 1 && digits->nth(digits->size()-1) == 0) {
                            digits = digits->pop_back();
                        }
                    }
                }
            }
        } else {
            if(negative) {
                negative = false;
                *this += other;
                negative = true;
            } else {
                jo_bigint other_copy = other;
                other_copy.negative = false;
                *this += other_copy;
            }
        }
        return *this;
    }

    jo_bigint &operator++() {
        *this += 1;
        return *this;
    }

    jo_bigint &operator--() {
        *this -= 1;
        return *this;
    }

    jo_bigint operator++(int) {
        jo_bigint ret = *this;
        *this += 1;
        return ret;
    }

    jo_bigint operator--(int) {
        jo_bigint ret = *this;
        *this -= 1;
        return ret;
    }

    jo_bigint operator-() const {
        jo_bigint ret = *this;
        ret.negative = !ret.negative;
        return ret;
    }

    jo_bigint operator+(const jo_bigint &other) const {
        jo_bigint ret = *this;
        ret += other;
        return ret;
    }

    jo_bigint operator-(const jo_bigint &other) const {
        jo_bigint ret = *this;
        ret -= other;
        return ret;
    }

    bool operator==(const jo_bigint &other) const {
        if(negative != other.negative) {
            return false;
        }
        if(digits->size() != other.digits->size()) {
            return false;
        }
        if(digits->size() == 0) {
            return true;
        }
        auto it = digits->begin();
        auto it_other = other.digits->begin();
        while(it != digits->end() && it_other != other.digits->end()) {
            if(*it != *it_other) {
                return false;
            }
            it++;
            it_other++;
        }
        return true;
    }

    bool operator!=(const jo_bigint &other) const {
        return !(*this == other);
    }

    bool operator<(const jo_bigint &other) const {
        if(negative != other.negative) {
            return negative;
        }
        if(digits->size() != other.digits->size()) {
            return digits->size() < other.digits->size();
        }
        if(digits->size() == 0) {
            return false;
        }
        for(int i = digits->size()-1; i >= 0; --i) {
            if(digits->nth(i) != other.digits->nth(i)) {
                return digits->nth(i) < other.digits->nth(i);
            }
        }
        return false;
    }

    bool operator>(const jo_bigint &other) const {
        return other < *this;
    }

    bool operator<=(const jo_bigint &other) const {
        return !(other < *this);
    }

    bool operator>=(const jo_bigint &other) const {
        return !(*this < other);
    }

    // to string
    jo_string to_string() const {
        jo_string ret;
        if(negative) {
            ret += '-';
        }
        for(int i = digits->size() - 1; i >= 0; i--) {
            ret += digits->nth(i) + '0';
        }
        return ret;
    }

    // print
    void print() const {
        if(negative) {
            printf("-");
        }
        for(int i = digits->size() - 1; i >= 0; i--) {
            printf("%d", digits->nth(i));
        }
        printf("\n");
    }

    int to_int() const {
        int ret = 0;
        for(int i = digits->size() - 1; i >= 0; i--) {
            ret *= 10;
            ret += digits->nth(i);
        }
        if(negative) {
            ret *= -1;
        }
        return ret;
    }


};

// arbitrary precision floating point
struct jo_float {
    bool negative;
    jo_bigint mantissa;
    int exponent;

    jo_float() {
        negative = false;
        mantissa.digits = mantissa.digits->push_back(0);
        exponent = 0;
    }

    jo_float(const jo_float &other) {
        negative = other.negative;
        mantissa = other.mantissa;
        exponent = other.exponent;
    }

    jo_float(const int &other) {
        negative = other < 0;
        mantissa = jo_bigint(other);
        exponent = 0;
    }

    jo_float(const jo_bigint &other) {
        negative = other.negative;
        mantissa = other;
        exponent = 0;
    }

    jo_float(const double &other) {
        negative = other < 0;
        mantissa = jo_bigint(other);
        exponent = 0;
    }

    jo_float(const char *other) {
        negative = other[0] == '-';
        mantissa = jo_bigint(other);
        exponent = 0;
    }

    jo_float(const char *other, int base) {
        negative = other[0] == '-';
        mantissa = jo_bigint(other, base);
        exponent = 0;
    }

    jo_float(const char *other, int base, int exp) {
        negative = other[0] == '-';
        mantissa = jo_bigint(other, base);
        exponent = exp;
    }

    jo_float(const char *other, int base, int exp, int exp_base) {
        negative = other[0] == '-';
        mantissa = jo_bigint(other, base);
        exponent = exp * exp_base;
    }

    // operator overloads
    jo_float operator+(const jo_float &other) const {
        jo_float ret;
        ret.negative = negative;
        ret.exponent = exponent;
        ret.mantissa = mantissa + other.mantissa;
        return ret;
    }

    jo_float operator-(const jo_float &other) const {
        jo_float ret;
        ret.negative = negative;
        ret.exponent = exponent;
        ret.mantissa = mantissa - other.mantissa;
        return ret;
    }

    /*
    jo_float operator*(const jo_float &other) const {
        jo_float ret;
        ret.negative = negative;
        ret.exponent = exponent + other.exponent;
        ret.mantissa = mantissa * other.mantissa;
        return ret;
    }

    jo_float operator/(const jo_float &other) const {
        jo_float ret;
        ret.negative = negative;
        ret.exponent = exponent - other.exponent;
        ret.mantissa = mantissa / other.mantissa;
        return ret;
    }
    */

    // to string
    /*
    jo_string to_string() const {
        jo_string ret;
        if(negative) {
            ret += '-';
        }
        ret += mantissa.to_string();
        ret += '.';
        jo_bigint tmp = mantissa;
        tmp /= 10;
        while(tmp > 0) {
            ret += tmp.to_string();
            tmp /= 10;
        }
        ret += 'e';
        ret += jo_string::format("%i", exponent);
        return ret;
    }
    */
};

#ifdef _WIN32
#pragma warning(pop)
#endif

#endif // JO_STDCPP

