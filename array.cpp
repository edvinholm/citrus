



template<typename T, Allocator_ID A>
T &Array<T, A>::operator [] (const s64 index)
{
    Assert(this->n > index);
    return this->e[index];
}

template<typename T, Allocator_ID A>
bool ensure_capacity(Array<T, A> &array, s64 capacity)
{
    s64 new_allocated = max(array.allocated, (s64)1);
    while(new_allocated < capacity)
    {
        new_allocated *= 2;
    }
    if(new_allocated != array.allocated)
    {
        T *new_e = (T *)alloc(new_allocated * sizeof(T), A);
        Release_Assert_Args(new_e,    "new_allocated", new_allocated, "sizeof(T)", sizeof(T), "A", A, "array.allocated", array.allocated, "array.n", array.n, "array.e", (s64)array.e);
        if(!new_e) return false;

        if(array.n)
        {
            Assert(array.e);
            memcpy(new_e, array.e, sizeof(T)*array.n);
        }
        if(array.e) dealloc_if_legal(array.e, A);
        
        array.e = new_e;
        array.allocated = new_allocated;
    }
    return true;
}


template<typename T, Allocator_ID A>
inline
T *array_add_uninitialized(Array<T, A> &array, s64 num_elements = 1)
{
    ensure_capacity(array, array.n + num_elements);

    T *result = array.e + array.n;
    array.n += num_elements;

    return result;
}

template<typename T, Allocator_ID A>
T *array_add(Array<T, A> &array, T *elements, s64 num_elements /* = 1 */)
{
    T *element0 = array_add_uninitialized(array, num_elements);
    
    memcpy(element0, elements, sizeof(T) * num_elements);

    return element0;
}

template<typename T, Allocator_ID A>
inline
T *array_add(Array<T, A> &array, T item)
{
    return array_add(array, &item, 1);
}

template<typename T, Allocator_ID A>
void array_set(Array<T, A> &array, T *elements, s64 num_elements)
{
    array.n = 0;
    array_add(array, elements, num_elements);
}

template<typename T, Allocator_ID A, Allocator_ID B>
void array_set(Array<T, A> &dest, Array<T, B> &src)
{
    array_set(dest, src.e, src.n);
}


template<typename T, Allocator_ID A>
T *array_insert(Array<T, A> &array, T *elements, int index, s64 num_elements = 1)
{
    Assert(index <= array.n);
    Assert(index >= 0);

    s64 index_64 = index;
    
    if(index_64 == array.n)
        return array_add(array, elements, num_elements);
    
    ensure_capacity(array, array.n + num_elements);

    // Shift elements forward
    //TODO @Speed: IMPORTANT We should do something better here. Reason to why we don't just do a memcpy is because it's undefined behaviour to have the destination overlap the source data.
    /*s64 num_elements_64 = num_elements;
    auto *dest = array.e + array.n + num_elements_64 - 1;
    auto *src  = dest - 1;
    auto *end = array.e + index_64 + num_elements_64;
    auto *end_plus_8 = end + 8;
    while(dest >= end_plus_8) {
        *dest = *src; dest--; src--;
        *dest = *src; dest--; src--;
        *dest = *src; dest--; src--;
        *dest = *src; dest--; src--;
        *dest = *src; dest--; src--;
        *dest = *src; dest--; src--;
        *dest = *src; dest--; src--;
        *dest = *src; dest--; src--;
    }
    while(dest >= end) {
        *dest = *src; dest--; src--;
    }*/

    size_t size_to_move = sizeof(T) * (array.n - index_64);
    T *tmp = (T *)tmp_alloc(size_to_move);
    memcpy(tmp, array.e + index_64, size_to_move);
    memcpy(array.e + index_64 + num_elements, tmp, size_to_move);
        

    // Copy in new elements
    memcpy(array.e + index_64, elements, sizeof(T) * num_elements);
    
    array.n += num_elements;

    return array.e + index_64;
}


template<typename T, Allocator_ID A>
T *array_insert(Array<T, A> &array, T item, int index)
{
    return array_insert(array, &item, index, 1);
}



template<typename T, Allocator_ID A>
inline
bool merge_arrays(Array<T, A> &array, Array<T, A> &other_array)
{
    return array_add(array, other_array.e, other_array.n);
}


// NOTE: Two arrays of the same length are guaranteed to move the remaining elements in the same way.
template<typename T, Allocator_ID A>
inline
void array_unordered_remove(Array<T, A> &array, s64 index, s64 n /* = 1*/)
{
    Assert(n <= array.n - index);
    
    s64 to_copy = min(n, array.n - (index + n));
    memcpy(array.e + index, array.e + array.n - to_copy, sizeof(T)*to_copy);
    array.n -= n;
}

template<typename T, Allocator_ID A>
inline
void array_ordered_remove(Array<T, A> &array, s64 index, s64 n /* = 1*/)
{
    Assert(index >= 0 && index < array.n);
    memcpy(array.e + index, array.e + index + n, sizeof(T) * (array.n - (index + n)));
    array.n -= n;
}


template<typename T, Allocator_ID A>
inline
void array_swap(Array<T, A> &array, s64 a, s64 b)
{
    T tmp = array.e[a];
    array.e[a] = array.e[b];
    array.e[b] = tmp;
}

template<typename T, Allocator_ID A>
inline
T *element_pointer(Array<T, A> &array, s64 index)
{
    Assert(array.n > index);
    return array.e + index;
}

template<typename T, Allocator_ID A>
inline
T *last_element_pointer(Array<T, A> &array)
{
    return array.e + array.n-1;
}

template<typename T, Allocator_ID A>
inline
T last_element(Array<T, A> &array)
{
    return array.e[array.n-1];
}

template<typename T, Allocator_ID A>
bool in_array(Array<T, A> &array, T element, s64 *_index /* = NULL */)
{
    for(s64 i = 0; i < array.n; i++)
    {
        if(array.e[i] == element) {
            if(_index) *_index = i;
            return true;
        }
    }
    
    return false;
}


template<typename T, Allocator_ID A>
inline
void ensure_in_array(Array<T, A> &array, T &element)
{
    if(!in_array(array, element))
        array_add(array, element);
}


//IMPORTANT: This function assumes there is at most 1 occurrence of the element in the array.
//IMPORTANT: This function does an UNORDERED remove if the element is found.
template<typename T, Allocator_ID A>
inline
void ensure_not_in_array(Array<T, A> &array, T &element)
{
    s64 index;
    if(in_array(array, element, &index))
        array_unordered_remove(array, index);
}


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
Array<T, A> array_copy(Array<T, A> original)
{
    Array<T, A> copy = original;
    copy.e = (T *)alloc(sizeof(T) * original.allocated);
    memcpy(copy.e, original.e, sizeof(T) * original.n);

    return copy;
}


template<typename T, Allocator_ID A>
bool all_elements_equal(Array<T, A> &a, Array<T, A> &b)
{
    if(a.n != b.n) return false;
    for(int i = 0; i < a.n; i++)
    {
        if(!equal(a[i], b[i])) return false;
    }

    return true;
}


















template<typename T, int Size>
T &Static_Array<T, Size>::operator [] (const s64 index)
{
    Assert(index < this->n);
    Assert(index >= 0);
    
    return this->e[index];
}


template<typename T, int Size>
int capacity_of(Static_Array<T, Size> &array)
{
    return Size;
}

template<typename T, int Size>
T *array_add(Static_Array<T, Size> &array, T *elements, s64 num_elements/* = 1*/)
{
    Assert(array.n + num_elements <= Size);

    T *result = array.e + array.n;
    
    memcpy(array.e + array.n, elements, sizeof(T) * num_elements);
    array.n += num_elements;

    return result;
}


template<typename T, int Size>
T *array_add(Static_Array<T, Size> &array, T item)
{
    return array_add(array, &item, 1);
}


template<typename T, int Size>
T *last_element_pointer(Static_Array<T, Size> &array)
{
    return array.e + array.n-1;
}


template<typename T, int Size>
void array_unordered_remove(Static_Array<T, Size> &array, s64 index, s64 n/* = 1*/)
{
    Assert(n <= array.n - index);
    
    s64 to_copy = min(n, array.n - (index + n));
    memcpy(array.e + index, array.e + array.n - to_copy, sizeof(T) * to_copy);
    array.n -= n;
}
    
template<typename T, int Size>
bool in_array(Static_Array<T, Size> &array, T element, s64 *_index/* = NULL*/)
{
    for(s64 i = 0; i < array.n; i++)
    {
        if(array.e[i] == element) {
            if(_index) *_index = i;
            return true;
        }
    }
    
    return false;
}


template<typename T, int Size>
void ensure_in_array(Static_Array<T, Size> &array, T &element)
{
    if(!in_array(array, element))
        array_add(array, element);
}


//IMPORTANT: This function assumes there is at most 1 occurrence of the element in the array.
//IMPORTANT: This function does an UNORDERED remove if the element is found.
template<typename T, int Size>
void ensure_not_in_array(Static_Array<T, Size> &array, T &element)
{
    s64 index;
    if(in_array(array, element, &index))
        array_unordered_remove(array, index);
}
