#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <pthread.h>
#include <deque>

namespace NSLOG
{
class Log
{
    public:
        static Log* get_instance()
        {
            static Log instance;
            return &instance;
        }
        static void *flush_log_thread(void* args)
        {
            return Log::get_instance()->async_write_log();
        }
        bool init(const char* file_name = 0, int log_buf_size = 8192, int split_lines = 5000000, bool is_async = true);
        void write_log(int level, const char* format, va_list valst);
        void flush(void);
    private:
        Log();
        virtual ~Log();
        
        void *async_write_log();
    private:
        pthread_mutex_t *m_mutex;
        pthread_cond_t  *m_cond;
        char m_dir_name[128];
        char m_log_name[128];
        int m_split_lines;
        long long  m_count;
        int m_today;
        FILE *m_fp;
        std::deque<std::string>* m_log_deque;
        bool m_is_async;
        char *m_buf;
        int m_log_buf_size;
};
}

/*0-infor,1-warng,2-debug,3-error*/
void log(const char* format, va_list valst);
void log(int level, const char* format, va_list valst);
void log(const char* format, ...);
void log(int level, const char* format, ...);
#endif
