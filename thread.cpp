



inline
bool create_thread(DWORD (*proc)(void *), void *param, Thread *_thread)
{
    return platform_create_thread(proc, param, _thread);
}



inline
void create_mutex(Mutex &mutex)
{
    platform_create_mutex(&mutex);
}

inline
void delete_mutex(Mutex &mutex)
{
    platform_delete_mutex(&mutex);
}

inline
void lock_mutex(Mutex &mutex)
{
    platform_lock_mutex(&mutex);
}


inline
void unlock_mutex(Mutex &mutex)
{
    platform_unlock_mutex(&mutex);
}
