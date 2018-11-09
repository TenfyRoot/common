#include "log.h"
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>

void log(const char* format, va_list valst)
{
	NSLOG::Log::get_instance()->write_log(0, format, valst);
}

void log(int level, const char* format, va_list valst)
{
	NSLOG::Log::get_instance()->write_log(level, format, valst);
}

void log(const char* format, ...)
{
	va_list valst;
	va_start(valst, format);
	NSLOG::Log::get_instance()->write_log(0, format, valst);
	va_end(valst);
}

void log(int level, const char* format, ...)
{
	va_list valst;
	va_start(valst, format);
	NSLOG::Log::get_instance()->write_log(level, format, valst);
	va_end(valst);
}

namespace NSLOG
{
Log::Log()
{
    m_count = 0;
    m_mutex = new pthread_mutex_t;
    m_cond = new pthread_cond_t;
    m_is_async = false;
    pthread_mutex_init(m_mutex, NULL);
    m_fp = 0;
    m_buf = 0;
    init();
}
Log::~Log()
{
    if(m_fp != NULL)
    {
        fclose(m_fp);
    }
    pthread_cond_destroy(m_cond);  
    if(m_cond != NULL) 
    {
        delete m_cond; 
    }
    pthread_mutex_destroy(m_mutex);
    if(m_mutex != NULL)
    {
        delete m_mutex;
    }
}
bool Log::init(const char* file_name, int log_buf_size, int split_lines, bool is_async)
{
    if(is_async)
    {
        m_is_async = is_async;  
        m_log_deque = new std::deque<std::string>();
        pthread_t tid;
        pthread_create(&tid, NULL, flush_log_thread, NULL);
    }
    m_log_buf_size = log_buf_size;
    if (m_buf)
    {
    	delete m_buf;
    	m_buf = 0;
    }
    m_buf = new char[m_log_buf_size];
    bzero(m_buf, m_log_buf_size);
    m_split_lines = split_lines;
    time_t t = time(NULL);
    struct tm* sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;
    m_today = my_tm.tm_mday;
    
    if (file_name)
    {
    	if (m_fp)
    		fclose(m_fp);
		const char *p = strrchr(file_name, '/');
		char log_full_name[256] = {0};
		if(p == NULL)
		{
		    snprintf(log_full_name, 255, "%d_%02d_%02d_%s",my_tm.tm_year+1900, my_tm.tm_mon+1, my_tm.tm_mday, file_name);
		}
		else
		{
		    strcpy(m_log_name, p+1);
		    strncpy(m_dir_name, file_name, p - file_name + 1);
		    snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s", m_dir_name, my_tm.tm_year+1900, my_tm.tm_mon+1, my_tm.tm_mday, m_log_name );
		}
		
		m_fp = fopen(log_full_name, "a");
	}
    return true;
}

void Log::write_log(int level, const char* format, va_list valst)
{
	pthread_mutex_lock(m_mutex);
	bzero(m_buf, m_log_buf_size);
    struct timeval now = {0,0};
    gettimeofday(&now, NULL);
    time_t t = now.tv_sec;
    struct tm* sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;
    char prefix[16] = {0};
    switch(level)
    {
        case 0 : strcpy(prefix, "[infor]"); break;
        case 1 : strcpy(prefix, "[warng]"); break;
        case 2 : strcpy(prefix, "[debug]"); break;
        case 3 : strcpy(prefix, "[error]"); break;
        default:
                 strcpy(prefix, "[infor]"); break;
    }

    m_count++;
    if(m_fp && (m_today != my_tm.tm_mday || m_count % m_split_lines == 0)) //everyday log  
    {
        char new_log[256] = {0};
        fflush(m_fp);
        fclose(m_fp);
        char tail[16] = {0};
        snprintf(tail, 16,  "%d_%02d_%02d_", my_tm.tm_year+1900, my_tm.tm_mon+1, my_tm.tm_mday);
        if(m_today != my_tm.tm_mday)
        {
            snprintf(new_log, 255, "%s%s%s", m_dir_name, tail, m_log_name);
            m_today = my_tm.tm_mday;
            m_count = 0;
        }
        else
        {
            snprintf(new_log, 255, "%s%s%s.%lld", m_dir_name, tail, m_log_name, m_count/m_split_lines);
        }
        m_fp = fopen(new_log, "a");
    }
    
    int n = snprintf(m_buf, 48, "[%d-%02d-%02d %02d:%02d:%02d.%06d]%s ",
            (my_tm.tm_year+1900)%100, my_tm.tm_mon+1, my_tm.tm_mday,
            my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, (int)now.tv_usec, prefix);
    int m = vsnprintf(m_buf + n, m_log_buf_size - n, format, valst);
	m_buf[n + m] = '\n';
	if(m_is_async) {
    	m_log_deque->push_back(std::string(m_buf));
    	pthread_cond_broadcast(m_cond);
	} else {
		if (m_fp) fputs(m_buf, m_fp);
    	printf(m_buf);
	}
	pthread_mutex_unlock(m_mutex);
    return;
}
void Log::flush(void)
{
    pthread_mutex_lock(m_mutex);
    fflush(m_fp);
    pthread_mutex_unlock(m_mutex);
}
void *Log::async_write_log()
{
    while(1){
        if(!m_log_deque->size())
        {
            pthread_mutex_lock(m_mutex);
            {
		        pthread_cond_wait(m_cond, m_mutex);
		        const char* pch = m_log_deque->front().c_str();
		        if (m_fp) fputs(pch, m_fp);
    			printf(pch);
		        m_log_deque->pop_front();
		        pthread_mutex_unlock(m_mutex);
            }
        } else {
            pthread_mutex_lock(m_mutex);
            const char* pch = m_log_deque->front().c_str();
	        if (m_fp) fputs(pch, m_fp);
			printf(pch);
            m_log_deque->pop_front();
            pthread_mutex_unlock(m_mutex);
        }
    }
    return 0;
}
}
