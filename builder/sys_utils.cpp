#include <shlwapi.h>

typedef LARGE_INTEGER Ticks;

namespace sys_utils
{
    Ticks get_ticks()
    {
        LARGE_INTEGER ticks;
        
        QueryPerformanceCounter(&ticks);

        return (Ticks)ticks;
    }

    Ticks get_tick_frequency()
    {
        LARGE_INTEGER frequency;

        QueryPerformanceFrequency(&frequency);

        return (Ticks)frequency;
    }

    float get_dt_from_tick_difference(Ticks t1, Ticks t2, Ticks frequency)
    {
        float dt = (float)((t2.QuadPart - t1.QuadPart) / (double)frequency.QuadPart);
        return dt;
    }

    char *get_current_time_and_date_string(char *date_format, char *time_format, bool date_first = true, AllocatorType allocator_type = PERSISTENT)
    {
        SYSTEMTIME current_time;
        GetLocalTime(&current_time);

        uint32_t bytes_needed_date = GetDateFormat(LOCALE_USER_DEFAULT, NULL, &current_time, date_format, NULL, 0);
        if (bytes_needed_date == 0)
        {
            printf("Error obtaining date format string.\n");
            return NULL;
        }

        uint32_t bytes_needed_time = GetTimeFormat(LOCALE_USER_DEFAULT, NULL, &current_time, time_format, NULL, 0);
        if (bytes_needed_time == 0)
        {
            printf("Error obtaining time format string.\n");
            return NULL;
        }

        uint32_t bytes_needed_total = bytes_needed_date + bytes_needed_time - 1; // Don't count ntc twice
        auto mem_state_temp = memory::set(allocator_type);
        char *time_date_string_buffer = memory::allocate<char>(bytes_needed_total, allocator_type);
        char *date_part = date_first ? time_date_string_buffer : time_date_string_buffer + bytes_needed_time - 1;
        char *time_part = date_first ? time_date_string_buffer + bytes_needed_date - 1 : time_date_string_buffer;

        if(date_first)
        {
            uint32_t bytes_written_date = GetDateFormat(LOCALE_USER_DEFAULT, NULL, &current_time, date_format, date_part, bytes_needed_date);
            if (bytes_written_date == 0)
            {
                printf("Error obtaining date format string.\n");
                memory::reset(mem_state_temp, allocator_type);
                return NULL;
            }
        }
        
        uint32_t bytes_written_time = GetTimeFormat(LOCALE_USER_DEFAULT, NULL, &current_time, time_format, time_part, bytes_needed_time);
        if (bytes_written_time == 0)
        {
            printf("Error obtaining time format string.\n");
            memory::reset(mem_state_temp, allocator_type);
            return NULL;
        }

        if(!date_first)
        {
            uint32_t bytes_written_date = GetDateFormat(LOCALE_USER_DEFAULT, NULL, &current_time, date_format, date_part, bytes_needed_date);
            if (bytes_written_date == 0)
            {
                printf("Error obtaining date format string.\n");
                memory::reset(mem_state_temp, allocator_type);
                return NULL;
            }
        }

        return time_date_string_buffer;
    }
}

struct Timer
{
    Ticks frequency;
    Ticks start;
};

namespace timer
{
    Timer get()
    {
        Timer timer = {};
        timer.frequency = sys_utils::get_tick_frequency();
        return timer;
    }

    void start(Timer *timer)
    {
        timer->start = sys_utils::get_ticks();
    }

    float end(Timer *timer)
    {
        Ticks current = sys_utils::get_ticks();
        float dt = sys_utils::get_dt_from_tick_difference(timer->start, current, timer->frequency);
        return dt;
    }

    float checkpoint(Timer *timer)
    {
        Ticks current = sys_utils::get_ticks();
        float dt = sys_utils::get_dt_from_tick_difference(timer->start, current, timer->frequency);
        timer->start = current;
        
        return dt;
    }
}