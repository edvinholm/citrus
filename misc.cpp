
template<typename T>
void swap(T *a, T *b)
{
    T tmp = *a;
    *a = *b;
    *b = tmp;
}



template<typename T>
struct Optional {
    bool present;
    T value;

    void operator = (T &v)
    {
        this->present = true;
        this->value = v;
    }

    operator bool() {
        return present;
    }
};

template<typename T>
bool operator == (Optional<T> &o, T &v)
{
    if(!o.present) return false;
    return o.value == v;
}

template<typename T>
bool operator != (Optional<T> &o, T &v)
{
    return !(o == v);
}

template<typename T>
bool operator == (T &v, Optional<T> &o) { return o == v; }

template<typename T>
bool operator != (T &v, Optional<T> &o) { return o != v; }


template<typename T>
bool get(Optional<T> &o, T *_value)
{
    if(!o.present) return false;
    *_value = o.value;
    return true;
}

template<typename T>
T get_or_default(Optional<T> &o, T default_value)
{
    if(!o.present) return default_value;
    return o.value;
}

template<typename T>
Optional<T> opt(T value, bool present = true)
{
    Optional<T> o = {0};
    o.present = present;
    o.value = value;
    return o;
}





#define Scoped_Push(Stack, Value) \
    push(Stack, Value); \
    defer(pop(Stack);)


template<typename T, int Max>
struct Static_Stack
{
    T e[Max];
    int size;
};

template<typename T, int Max>
void push(Static_Stack<T, Max> &stack, T elem)
{
    Assert(stack.size < Max);
    Assert(stack.size >= 0);
    stack.e[stack.size++] = elem;
}

template<typename T, int Max>
T pop(Static_Stack<T, Max> &stack)
{
    Assert(stack.size <= Max);
    Assert(stack.size > 0);
    return stack.e[--stack.size];
}


// IMPORTANT: Don't call this directly. This is called by the specific
//            implementations of current for the different T's.
//            All this nonsense we have to do just because you can't
//            have struct values as template parameters.
template<typename T, int Max>
T current_(Static_Stack<T, Max> &stack, T default_if_empty)
{
    Assert(stack.size <= Max);
    Assert(stack.size >= 0);
    if(stack.size == 0) return default_if_empty;
    else return stack.e[stack.size-1];
}

template<typename T, int Max>
T *current(Static_Stack<T *, Max> &stack)
{
    return current_<T *, Max>(stack, NULL);
}


template<typename T>
void copy_elements(T *dest, T *src, size_t num)
{
    memcpy(dest, src, sizeof(T) * num);
}

