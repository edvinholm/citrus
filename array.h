

#define ARRLEN(c_array) (sizeof(c_array)/sizeof(c_array[0]))


template<typename T, Allocator_ID A>
struct Array
{
    s64 n;
    T *e;
    s64 allocated;

    T &operator [] (const s64 index);
};

template<typename T, Allocator_ID A>
inline
void clear(Array<T, A> *array)
{   
    if(array->e) dealloc(array->e, A);
    memset(array, 0, sizeof(Array<T, A>));
}

template<typename T, Allocator_ID A>
inline
void clear_deep(Array<T, A> *array, Allocator_ID element_allocator)
{
    for(int i = 0; i < array->n; i++)
        clear_deep(array->e + i, element_allocator);

    clear(array);
}



template<typename T, Allocator_ID A>
T *array_add(Array<T, A> &array, T *elements, s64 num_elements = 1);


template<typename T, Allocator_ID A>
T *array_add(Array<T, A> &array, T item);


template<typename T, Allocator_ID A>
inline
T *last_element_pointer(Array<T, A> &array);


template<typename T, Allocator_ID A>
void array_unordered_remove(Array<T, A> &array, s64 index, s64 n = 1);

template<typename T, Allocator_ID A>
void array_ordered_remove(Array<T, A> &array, s64 index, s64 n = 1);
    
template<typename T, Allocator_ID A>
bool in_array(Array<T, A> &array, T element, s64 *_index = NULL);


template<typename T, Allocator_ID A>
void ensure_in_array(Array<T, A> &array, T &element);


template<typename T, Allocator_ID A>
void ensure_not_in_array(Array<T, A> &array, T &element);



template<typename T, int Size>
struct Static_Array
{
    s64 n;
    T e[Size];
    
    T &operator [] (const s64 index);
};

template<typename T, int Size>
int capacity_of(Static_Array<T, Size> &array);

template<typename T, int Size>
T *array_add(Static_Array<T, Size> &array, T *elements, s64 num_elements = 1);


template<typename T, int Size>
T *array_add(Static_Array<T, Size> &array, T item);


template<typename T, int Size>
inline
T *last_element_pointer(Static_Array<T, Size> &array);


template<typename T, int Size>
void array_unordered_remove(Static_Array<T, Size> &array, s64 index, s64 n = 1);
    
template<typename T, int Size>
bool in_array(Static_Array<T, Size> &array, T element, s64 *_index = NULL);


template<typename T, int Size>
void ensure_in_array(Static_Array<T, Size> &array, T &element);


template<typename T, int Size>
void ensure_not_in_array(Static_Array<T, Size> &array, T &element);
