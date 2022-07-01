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
#define jo_alloca _alloca
#pragma warning(push)
#pragma warning(disable : 4345)
#elif defined(__APPLE__)
#include <mach/mach_time.h>
#include <unistd.h>
#include <termios.h>
#else
#include <unistd.h>
#include <termios.h>
#endif

// if clang or GCC
#if defined(__clang__) || defined(__GNUC__)
#define jo_strdup strdup
#define jo_chdir chdir
#define jo_alloca __builtin_alloca
#define jo_memcpy __builtin_memcpy
#define jo_memmove __builtin_memmove
#define jo_memset __builtin_memset
#define jo_assume __builtin_assume
#define jo_expect __builtin_expect
#else
#define jo_memcpy memcpy
#define jo_memmove memmove
#define jo_memset memset
#define jo_assume sizeof
#define jo_expect(expr, val) expr
#endif

template<typename T1, typename T2> static constexpr inline T1 jo_min(T1 a, T2 b) { return a < b ? a : b; }
template<typename T1, typename T2> static constexpr inline T1 jo_max(T1 a, T2 b) { return a > b ? a : b; }

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
    while((nread = fread(buf, 1, sizeof(buf), fsrc)) != 0) {
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

// returns in floating point seconds
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

void jo_sleep(double seconds) {
#ifdef _WIN32
#if 0
    LARGE_INTEGER due;
    due.QuadPart = -int64_t(seconds * 1e7);
    HANDLE timer = CreateWaitableTimer(NULL, FALSE, NULL);
    SetWaitableTimerEx(timer, &due, 0, NULL, NULL, NULL, 0);
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
#elif 1
    static thread_local double estimate = 5e-3;
    static thread_local double mean = 5e-3;
    static thread_local double m2 = 0;
    static thread_local int64_t count = 1;

	double A,B;
    while (seconds - estimate > 1e-7) {
        double toWait = seconds - estimate;
        LARGE_INTEGER due;
        due.QuadPart = -int64_t(toWait * 1e7);

        A = jo_time();
        HANDLE timer = CreateWaitableTimer(NULL, FALSE, NULL);
        SetWaitableTimerEx(timer, &due, 0, NULL, NULL, NULL, 0);
        WaitForSingleObject(timer, INFINITE);
        CloseHandle(timer);
        B = jo_time();
        double observed = B - A;
        seconds -= observed;
    
        ++count;
        double error = observed - toWait;
        double delta = error - mean;
        mean += delta / count;
        m2   += delta * (error - mean);
        double stddev = sqrt(m2 / (count - 1));
        estimate = mean + stddev;
    }
    if(seconds > 0) {
        A = B = jo_time();
        while(B - A < seconds) {
            std::this_thread::yield();
            B = jo_time();
        }
    }
#else  
    Sleep(seconds * 1000);
#endif
#else
    usleep(seconds * 1000000);
#endif
}

// yield exponential backoff
static void jo_yield_backoff(int *count) {
    if(*count <= 3) {
        // do nothing, just try again
    } else if(*count <= 16) {
        std::this_thread::yield();
    } else {
        const int lmin_ns = 1000;
        const int lmax_ns = 1000000;
        int sleep_ns = jo_min(lmin_ns + (int)((pow(*count + 1, 2) - 1) / 2), lmax_ns);
        jo_sleep(sleep_ns / 1000000.0f);
    }
    (*count)++;
}

static FILE *jo_fmemopen(void *buf, size_t size, const char *mode) {
    if (!size) {
        return 0;
    }

#ifdef _WIN32
    (void)mode;
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

static char *jo_tmpnam() {
#ifdef _WIN32
    char *buf = (char *)malloc(MAX_PATH);
    if(!buf) return NULL;
    if(!GetTempFileNameA(NULL, "jotmp", 0, buf)) {
        free(buf);
        return NULL;
    }
    return buf;
#else // not ideal, but shuts up warnings...
    char *tmpfile = jo_strdup("/tmp/indi_XXXXXX");
    int fd = mkstemp(tmpfile);
    if(fd == -1) return NULL;
    close(fd);
    return tmpfile;
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

// jo_pair is a simple pair of values
template<typename T1, typename T2>
struct jo_pair {
    T1 first;
    T2 second;

    jo_pair() : first(), second() {}
    jo_pair(const T1 &a, const T2 &b) : first(a), second(b) {}

    bool operator==(const jo_pair<T1, T2> &other) const { return first == other.first && second == other.second; }
    bool operator!=(const jo_pair<T1, T2> &other) const { return !(*this == other); }
    bool operator<(const jo_pair<T1, T2> &other) const {
        if(first < other.first) return true;
        if(first > other.first) return false;
        return second < other.second;
    }
};

// jo_make_pair
template<typename T1, typename T2>
inline jo_pair<T1, T2> jo_make_pair(const T1 &a, const T2 &b) {
    return jo_pair<T1, T2>(a, b);
}

// jo_tuple
template<typename T1, typename T2, typename T3>
struct jo_tuple {
    T1 first;
    T2 second;
    T3 third;

    jo_tuple() : first(), second(), third() {}
    jo_tuple(const T1 &a, const T2 &b, const T3 &c) : first(a), second(b), third(c) {}

    bool operator==(const jo_tuple<T1, T2, T3> &other) const { return first == other.first && second == other.second && third == other.third; }
    bool operator!=(const jo_tuple<T1, T2, T3> &other) const { return !(*this == other); }
    bool operator<(const jo_tuple<T1, T2, T3> &other) const {
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

static uint64_t thread_local jo_rnd_state = 0x4d595df4d0f33173; 

static uint32_t jo_rotr32(uint32_t x, unsigned r) {
#ifdef _WIN32
    return _rotr(x, r);
#else
    return (x >> r) | (x << (32 - r));
#endif
}

uint32_t jo_pcg32(uint64_t *state) {
	uint64_t x = *state;
	unsigned count = (unsigned)(x >> 59);		// 59 = 64 - 5

	*state = x * 6364136223846793005u + 1442695040888963407u;
	x ^= x >> 18;								// 18 = (64 - 27)/2
	return jo_rotr32((uint32_t)(x >> 27), count);	// 27 = 32 - 5
}

uint64_t jo_pcg32_init(uint64_t seed) {
	uint64_t state = seed + 1442695040888963407u;
	(void)jo_pcg32(&state);
    return state;
}

// jo_random_int
inline int jo_random_int(int min, int max) {
    return min + (jo_pcg32(&jo_rnd_state) % (max - min + 1));
}

inline int jo_random_int(int max) {
    return jo_random_int(0, max);
}

inline int jo_random_int() {
    return jo_random_int(0, 0x7ffffff0);
}

inline double jo_random_float() {
    return (double)jo_pcg32(&jo_rnd_state) / (double)UINT32_MAX;
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

struct jo_object {
    virtual ~jo_object() {}
};

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
        jo_memcpy(str, a, size);
        str[size] = 0;
    }
    jo_string(const char *a, const char *b) {
        ptrdiff_t size = b - a;
        str = (char*)malloc(size+1);
        jo_memcpy(str, a, size);
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
        jo_memcpy(str+l0, s, l1);
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
        jo_memmove(str+n, str+m, l-m+1);
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
        jo_memmove(str+n+l1, str+n, l0-n+1);
        jo_memcpy(str+n, s, l1);
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
            str[i] = (char)jo_tolower(str[i]);
        }
        return *this;
    }

    jo_string &upper() {
        for(size_t i = 0; i < size(); i++) {
            str[i] = (char)jo_toupper(str[i]);
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
        str[0] = (char)jo_toupper(str[0]);
        for(size_t i = 1; i < size(); i++) {
            str[i] = (char)jo_tolower(str[i]);
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
            jo_memcpy(tmp, str+start, end-start+1);
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
            jo_memcpy(tmp, str+start, size()-start);
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
            jo_memcpy(tmp, str, end+1);
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
        jo_memcpy(tmp, str, n);
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
        jo_memcpy(tmp, str+n, l-n);
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

    // copy 
    jo_vector(const jo_vector &other) {
        ptr = 0;
        ptr_size = 0;
        ptr_capacity = 0;
        resize(other.ptr_size);
        if(std::is_pod<T>::value) {
            jo_memcpy(ptr, other.ptr, other.ptr_size*sizeof(T));
        } else {
            for(size_t i=0; i<other.ptr_size; i++) {
                ptr[i] = other.ptr[i];
            }
        }
    }

    // move 
    jo_vector(jo_vector &&other) {
        ptr = other.ptr;
        ptr_size = other.ptr_size;
        ptr_capacity = other.ptr_capacity;
        other.ptr = 0;
        other.ptr_size = 0;
        other.ptr_capacity = 0;
    }

    // assign
    jo_vector &operator=(const jo_vector &other) {
        resize(other.ptr_size);
        if(std::is_pod<T>::value) {
            jo_memcpy(ptr, other.ptr, other.ptr_size*sizeof(T));
        } else {
            for(size_t i=0; i<other.ptr_size; i++) {
                ptr[i] = other.ptr[i];
            }
        }
        return *this;
    }

    // move assign
    jo_vector &operator=(jo_vector &&other) {
        clear();
        ptr = other.ptr;
        ptr_size = other.ptr_size;
        ptr_capacity = other.ptr_capacity;
        other.ptr = 0;
        other.ptr_size = 0;
        other.ptr_capacity = 0;
        return *this;
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
                jo_memcpy(newptr, ptr, ptr_size*sizeof(T));
                //jo_memset(ptr, 0xFE, ptr_size*sizeof(T));
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
                jo_memcpy(newptr, ptr, ptr_size*sizeof(T));
                //jo_memset(ptr, 0xFE, ptr_size*sizeof(T));
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
        jo_memmove(ptr + where_at + how_many, ptr + where_at, sizeof(T)*(ptr_size - where_at));
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
        jo_memcpy(newptr, ptr, ptr_size*sizeof(T));
        //jo_memset(ptr, 0xFE, ptr_capacity*sizeof(T)); // DEBUG
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
                jo_memcpy(newptr, ptr, ptr_size*sizeof(T));
                //jo_memset(ptr, 0xFE, ptr_size*sizeof(T));
                free(ptr);
            }
            ptr = newptr;
            ptr_capacity = n;
        }
    }
    
};

struct jo_semaphore {
    std::mutex m;
    std::condition_variable cv;
    std::atomic<int> count;

    jo_semaphore(int n) : m(), cv(), count(n) {}
    void notify() {
        std::unique_lock<std::mutex> l(m);
        ++count;
        cv.notify_one();
    }
    void wait() {
        std::unique_lock<std::mutex> l(m);
        cv.wait(l, [this]{ return count!=0; });
        --count;
    }
};

struct jo_semaphore_waiter_notifier {
    jo_semaphore &s;
    jo_semaphore_waiter_notifier(jo_semaphore &s) : s{s} { s.wait(); }
    ~jo_semaphore_waiter_notifier() { s.notify(); }
};

// https://cbloomrants.blogspot.com/2011/07/07-09-11-lockfree-thomasson-simple-mpmc.html
struct jo_fastsemaphore {
    std::atomic<int> count;
    jo_semaphore wait_set;

    jo_fastsemaphore(long count = 0) : count(count), wait_set(0) {
        assert(count > -1);
    }

    void notify() {
        if (count.fetch_add(1) < 0) {
            wait_set.notify();
        }
    }

    void wait() {
        if (count.fetch_sub(1) < 1) {
            wait_set.wait();
        }
    }
};

// https://cbloomrants.blogspot.com/2011/07/07-09-11-lockfree-thomasson-simple-mpmc.html
template <typename T, T invalid_value, int T_depth>
struct jo_mpmcq {
    std::atomic<T> slots[T_depth];
    char pad1[64];
    std::atomic<size_t> push_idx;
    char pad2[64];
    std::atomic<size_t> pop_idx;
    char pad3[64];
    jo_fastsemaphore push_sem;
    char pad4[64];
    jo_fastsemaphore pop_sem;
    char pad5[64];
    volatile bool closing;
    char pad6[64];

    jo_mpmcq() : push_idx(T_depth), pop_idx(0), push_sem(T_depth), pop_sem(0), closing(false) {
        for (size_t i = 0; i < T_depth; ++i) {
            slots[i].store(invalid_value);
        }
    }

    void push(T ptr) {
        push_sem.wait();
        size_t idx = push_idx.fetch_add(1, std::memory_order_relaxed) & (T_depth - 1);
        int count = 0;
        while (slots[idx].load() != invalid_value) {
            jo_yield_backoff(&count);
        }
        //assert(slots[idx].load() == invalid_value);
        slots[idx].store(ptr);
        pop_sem.notify();
    }

    T pop() {
        pop_sem.wait();
        if (closing) {
            pop_sem.notify();
            return invalid_value;
        }
        int idx = pop_idx.fetch_add(1, std::memory_order_relaxed) & (T_depth - 1);
        T res;
        int count = 0;
        while ((res = slots[idx].load()) == invalid_value) {
            jo_yield_backoff(&count);
        }
        slots[idx].store(invalid_value);
        push_sem.notify();
        return res;
    }

    void close() {
        closing = true;
        pop_sem.notify();
    }

    size_t size() const { return T_depth - push_sem.count.load(); }
    bool empty() const { return push_sem.count.load() == T_depth; }
    bool full() const { return push_sem.count.load() <= 1; }
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

    jo_pinned_vector() : buckets(), num_elements(), grow_mutex() {}

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

    size_t push_back(T &&val) {
        size_t this_elem = num_elements.fetch_add(1, std::memory_order_relaxed);
        int top = index_top(this_elem);
        size_t bottom = index_bottom(this_elem, top);
        if(buckets[top] == 0) {
            jo_lock_guard guard(grow_mutex);
            if(buckets[top] == 0) {
                buckets[top] = (T*)malloc(sizeof(T)*bucket_size(top));
            }
        }
        new(buckets[top] + bottom) T(val);
        return this_elem;
    }

    size_t push_back(const T& val) {
        size_t this_elem = num_elements.fetch_add(1, std::memory_order_relaxed);
        int top = index_top(this_elem);
        size_t bottom = index_bottom(this_elem, top);
        if(buckets[top] == 0) {
            jo_lock_guard guard(grow_mutex);
            if(buckets[top] == 0) {
                buckets[top] = (T*)malloc(sizeof(T)*bucket_size(top));
            }
        }
        if(std::is_pod<T>::value) {
            jo_memcpy(buckets[top] + bottom, &val, sizeof(T));
        } else {
            new(buckets[top] + bottom) T(val);
        }
        return this_elem;
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
    const iterator begin() const { return iterator(this, 0); }
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
        while(i < mid) {
            tmp[k++] = array[i++];
        }
        while(j < end) {
            tmp[k++] = array[j++];
        }
        for(int l = 0; l < k; l++) {
            array[start + l] = tmp[l];
        }
        delete[] tmp;
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
    std::atomic<int> *ref_count;
    
    jo_shared_ptr() : ptr(nullptr), ref_count(nullptr) {}
    jo_shared_ptr(T* Ptr) : ptr(Ptr), ref_count(Ptr ? new std::atomic<int>(1) : nullptr) {}
    jo_shared_ptr(const jo_shared_ptr& other) : ptr(other.ptr), ref_count(other.ref_count) {
        if(ref_count) ref_count->fetch_add(1);
    }
    jo_shared_ptr(jo_shared_ptr&& other) : ptr(other.ptr), ref_count(other.ref_count) {
        other.ptr = nullptr;
        other.ref_count = nullptr;
    }
    
    jo_shared_ptr& operator=(const jo_shared_ptr& other) {
        if (this != &other) {
#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
            if(other.ref_count) ++(*other.ref_count);
#else
            if(other.ref_count) other.ref_count->fetch_add(1);
#endif
            if(ref_count && ref_count->fetch_sub(1, std::memory_order_acq_rel) == 1) {
                delete ptr;
                delete ref_count;
            }
            ptr = other.ptr;
            ref_count = other.ref_count;
        }
        return *this;
    }
    
    jo_shared_ptr& operator=(jo_shared_ptr&& other) {
        if (this != &other) {
            if(ref_count) {
                if(ref_count->fetch_sub(1, std::memory_order_acq_rel) == 1) {
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
        if(ref_count && ref_count->fetch_sub(1, std::memory_order_acq_rel) == 1) {
            delete ptr;
            delete ref_count;
            ptr = nullptr;
            ref_count = nullptr;
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

    template<typename U> jo_shared_ptr<U> &cast() { 
        static_assert(std::is_base_of<T, U>::value || std::is_base_of<U, T>::value, "type parameter of this class must derive from T or U");
        return (jo_shared_ptr<U>&)(*this); 
    }
    template<typename U> const jo_shared_ptr<U> &cast() const { 
        static_assert(std::is_base_of<T, U>::value || std::is_base_of<U, T>::value, "type parameter of this class must derive from T or U");
        return (const jo_shared_ptr<U>&)(*this); 
    }
};

template<typename T> jo_shared_ptr<T> jo_make_shared() { return jo_shared_ptr<T>(new T()); }
template<typename T> jo_shared_ptr<T> jo_make_shared(const T& other) { return jo_shared_ptr<T>(new T(other)); }

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

// jo_hash
template<typename T>
struct jo_hash {
    size_t operator()(const T &value) const {
        return jo_hash_value(value);
    }
};

// jo_hash_value
size_t jo_hash_value(const bool &value) { return value ? 1 : 0; }
size_t jo_hash_value(const char &value) { return value; }
size_t jo_hash_value(const unsigned char &value) { return value; }
size_t jo_hash_value(const short &value) { return value; }
size_t jo_hash_value(const unsigned short &value) { return value; }
size_t jo_hash_value(const int &value) { return value; }
size_t jo_hash_value(const unsigned int &value) { return value; }
size_t jo_hash_value(const long &value) { return value; }
size_t jo_hash_value(const unsigned long &value) { return value; }
size_t jo_hash_value(const long long &value) { return value; }
size_t jo_hash_value(const unsigned long long &value) { return value; }
size_t jo_hash_value(const double value) { return *(size_t *)&value; }
size_t jo_hash_value(const long double &value) { return *(size_t *)&value; }
size_t jo_hash_value(const char *value) {
#if 1
    // FNV1 https://cbloomrants.blogspot.com/2010/11/11-29-10-useless-hash-test.html
    size_t hash = 2166136261;
    while(*value) {
        hash = (16777619 * hash) ^ (*value++);
    }
    return hash & 0x7FFFFFFFFFFFFFFFull;
#else
    size_t hash = 0;
    while(*value) {
        hash = hash * 31 + *value++;
    }
    return hash;
#endif
}
size_t jo_hash_value(const jo_string &value) { return jo_hash_value(value.c_str()); }

template<typename K, typename V>
struct jo_hash_map {
    typedef jo_tuple<K, V, bool> entry_t;

    // vec is used to store the keys and values
    jo_vector<entry_t> vec;
    // number of entries actually in the hash table
    size_t length;

    jo_hash_map() : vec(32), length() {}
    jo_hash_map(const jo_hash_map &other) : vec(other.vec), length(other.length) {}
    jo_hash_map &operator=(const jo_hash_map &other) {
        vec = other.vec;
        length = other.length;
        return *this;
    }

    size_t size() const { return length; }
    bool empty() const { return !length; }

    void clear() {
        vec = std::move(jo_vector<entry_t>(32));
        length = 0;
    }

    // iterator
    class iterator {
        const entry_t *cur;
        const entry_t *end;
    public:
        iterator(const entry_t *_cur, const entry_t *_end) : cur(_cur), end(_end) {
            while(cur != end && !cur->third) {
                ++cur;
            }
        }
        iterator() : cur(), end() {}
        iterator &operator++() {
            if(cur != end) {
                ++cur;
                while(cur != end && !cur->third) {
                    ++cur;
                }
            }
            return *this;
        }
        iterator operator++(int) {
            iterator tmp = *this;
            ++*this;
            return tmp;
        }
        bool operator==(const iterator &other) const { return cur == other.cur; }
        bool operator!=(const iterator &other) const { return cur != other.cur; }
        operator bool() const { return cur != end; }
        const entry_t &operator*() const { return *cur; }
        const entry_t *operator->() const { return &*cur; }
        const entry_t *get() const { return cur; }
    };

    iterator begin() { return length ? iterator(vec.begin(), vec.end()) : iterator(); }
    iterator begin() const  { return length ? iterator(vec.begin(), vec.end()) : iterator(); }

    void resize(size_t new_size) {
        jo_vector<entry_t> nv(new_size);
        for(iterator it = begin(); it; ++it) {
            auto &entry = *it;
            int index = jo_hash_value(entry.first) % new_size;
            while(nv[index].third) {
                index = (index + 1) % new_size;
            }
            nv[index] = entry;
        }
        vec = std::move(nv);
    }

    // assoc with lambda for equality
    entry_t &assoc(const K &key, const V &value) {
        if(vec.size() - length < vec.size() / 8) {
            resize(vec.size() * 2);
        }
        int index = jo_hash_value(key) % vec.size();
        entry_t e = vec[index];
        while(e.third) {
            if(e.first == key) {
                vec[index] = std::move(entry_t(key, value, true));
                return vec[index];
            }
            index = (index + 1) % vec.size();
            e = vec[index];
        } 
        vec[index] = entry_t(key, value, true);
        ++length;
        return vec[index];
    }

    // assoc with lambda for equality
    template<typename F>
    entry_t &assoc(const K &key, const V &value, F eq) {
        if(vec.size() - length < vec.size() / 8) {
            resize(vec.size() * 2);
        }
        int index = jo_hash_value(key) % vec.size();
        entry_t e = vec[index];
        while(e.third) {
            if(eq(e.first, key)) {
                vec[index] = std::move(entry_t(key, value, true));
                return vec[index];
            }
            index = (index + 1) % vec.size();
            e = vec[index];
        } 
        vec[index] = entry_t(key, value, true);
        ++length;
        return vec[index];
    }

    // dissoc_inplace
    template<typename F>
    void dissoc(const K &key, F eq) {
        int index = jo_hash_value(key) % vec.size();
        entry_t e = vec[index];
        while(e.third) {
            if(eq(e.first, key)) {
                // TODO: optimize
                vec[index] = std::move(entry_t());
                --length;
                // need to shuffle entries up to fill in the gap
                int i = index;
                int j = i;
                while(true) {
                    j = (j + 1) % vec.size();
                    if(!vec[j].third) {
                        break;
                    }
                    entry_t next_entry = vec[j];
                    if(jo_hash_value(next_entry.first) % vec.size() <= i) {
                        vec[i] = next_entry;
                        vec[j] = entry_t();
                        i = j;
                    }
                }
                return;
            }
            index = (index + 1) % vec.size();
            e = vec[index];
        }
    }

    // find using lambda
    entry_t find(const K &key) const {
        size_t index = jo_hash_value(key) % vec.size();
        entry_t e = vec[index];
        while(e.third) {
            if(e.first == key) {
                return e;
            }
            index = (index + 1) % vec.size();
            e = vec[index];
        }
        return entry_t();
    }

    // find using lambda
    template<typename F>
    entry_t find(const K &key, const F &f) const {
        size_t index = jo_hash_value(key) % vec.size();
        entry_t e = vec[index];
        while(e.third) {
            if(f(e.first, key)) {
                return e;
            }
            index = (index + 1) % vec.size();
            e = vec[index];
        }
        return entry_t();
    }
     
    // contains using lambda
    template<typename F>
    bool contains(const K &key, const F &f) {
        return find(key, f).third;
    }

    V &get(const K &key) {
        size_t index = jo_hash_value(key) % vec.size();
        entry_t e = vec[index];
        while(e.third) {
            if(e.first == key) {
                return vec[index].second;
            }
            index = (index + 1) % vec.size();
            e = vec[index];
        } 
        // make a new entry
        return assoc(key, V()).second;
    }

    template<typename F>
    V &get(const K &key, const F &f) {
        size_t index = jo_hash_value(key) % vec.size();
        entry_t e = vec[index];
        while(e.third) {
            if(f(e.first, key)) {
                return e.second;
            }
            index = (index + 1) % vec.size();
            e = vec[index];
        } 
        // make a new entry
        return assoc(key, V()).second;
    }
};

#ifdef _WIN32
#pragma warning(pop)
#endif

#endif // JO_STDCPP

