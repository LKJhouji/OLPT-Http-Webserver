#include <unistd.h>
#include <errno.h>
#include "LogFile.h"

void LogFile::append(const char* logline, int len) {
    std::unique_lock<std::mutex> lk(mutex_);
    size_t written = 0;
    while (written != len) {
        size_t remain = len - written;
        size_t n = ::fwrite_unlocked(logline + written, 1, remain, fp_);
        if (n != remain) {
            int err = ferror(fp_);
            if (err) {
                fprintf(stderr, "AppendFile::append() failed\n");
                break;
            }
        }
        written += n;
    }
} 

void LogFile::flush() {
    std::unique_lock<std::mutex> lk(mutex_);
    ::fflush(fp_);
}