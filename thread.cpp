
// @Temporary?
template<typename T>
struct Atomic {
    Mutex mutex;
    T value;
};

template<typename T>
void init_atomic(Atomic<T> *a)
{
    create_mutex(a->mutex);
}

template<typename T>
void deinit_atomic(Atomic<T> *a)
{
    delete_mutex(a->mutex);
}

template<typename T>
T get(Atomic<T> *a) {
    T result;
    lock_mutex(a->mutex);
    {
        result = a->value;
    }
    unlock_mutex(a->mutex);
    return result;
}

template<typename T>
void set(Atomic<T> *a, T value) {
    lock_mutex(a->mutex);
    {
        a->value = value;
    }
    unlock_mutex(a->mutex);
}


inline
bool create_thread(DWORD (*proc)(void *), void *param, Thread *_thread)
{
    return platform_create_thread(proc, param, _thread);
}

inline
void join_thread(Thread &thread)
{
    return platform_join_thread(thread);
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
