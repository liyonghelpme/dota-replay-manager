#ifndef __BASE_ARRAY_H__
#define __BASE_ARRAY_H__

#include <stdlib.h>
#include <new.h>

template<class T>
class Array
{
  static int (*compFunc) (T const& a, T const& b);
  static int tlCompFunc(void const* a, void const* b)
  {
    return compFunc(*(T*) a, *(T*) b);
  }
  int& maxSize()
  {return ((int*) items)[-1];}
  int& size()
  {return ((int*) items)[-2];}
  int& ref()
  {return ((int*) items)[-3];}
  int const& maxSize() const
  {return ((int*) items)[-1];}
  int const& size() const
  {return ((int*) items)[-2];}
  int const& ref() const
  {return ((int*) items)[-3];}
  void splice()
  {
    if (ref() <= 1)
      return;
    ref()--;
    int mxs = maxSize();
    int s = size();
    char* temp = (char*) malloc(sizeof(T) * mxs + 12) + 12;
    for (int i = 0; i < s; i++)
      new((T*) temp + i) T(((T*) items)[i]);
    items = temp;
    maxSize() = mxs;
    size() = s;
    ref() = 1;
  }
  char* items;
public:
  Array(int space = 8)
  {
    items = (char*) malloc(sizeof(T) * space + 12) + 12;
    maxSize() = space;
    size() = 0;
    ref() = 1;
  }
  ~Array()
  {
    if (--ref() <= 0)
    {
      for (int i = 0; i < size(); i++)
        ((T*) items)[i].~T();
      free(items - 12);
    }
  }
  Array(Array<T> const& a)
  {
    items = a.items;
    ref()++;
  }

  Array& operator = (Array const& a)
  {
    if (&a == this)
      return *this;
    if (--ref() <= 0)
    {
      for (int i = 0; i < size(); i++)
        ((T*) items)[i].~T();
      free(items - 12);
    }
    items = a.items;
    ref()++;
    return *this;
  }

  int length() const
  {
    return size();
  }

  T const& operator [] (int i) const
  {
    return ((T*) items)[i];
  }
  T& operator [] (int i)
  {
    splice();
    return ((T*) items)[i];
  }

  void clear()
  {
    splice();
    while (size() > 0)
      ((T*) items)[--size()].~T();
  }
  void reserve(int space)
  {
    if (space > maxSize())
    {
      splice();
      items = (char*) realloc(items - 12, sizeof(T) * space + 12) + 12;
      maxSize() = space;
    }
  }
  void resize(int new_size)
  {
    splice();
    reserve(new_size);
    while (size() < new_size)
      new((T*) items + (size()++)) T;
    while (size() > new_size)
      ((T*) items)[--size()].~T();
  }
  void resize(int new_size, T const& c)
  {
    splice();
    reserve(new_size);
    while (size() < new_size)
      new((T*) items + (size()++)) T(c);
    while (size() > new_size)
      ((T*) items)[--size()].~T();
  }
  int push(T const& e)
  {
    splice();
    reserve(size() + 1);
    new((T*) items + size()) T(e);
    return size()++;
  }
  T& push()
  {
    splice();
    reserve(size() + 1);
    new((T*) items + size()) T;
    return ((T*) items)[size()++];
  }
  void pop()
  {
    splice();
    ((T*) items)[--size()].~T();
  }

  void sort(int (*comp) (T const& a, T const& b))
  {
    compFunc = comp;
    qsort(items, size(), sizeof(T), tlCompFunc);
  }
};

template<class T>
int (*Array<T>::compFunc) (T const& a, T const& b);

template<class T>
class PtrArray
{
  static int (*compFunc) (T const* a, T const* b);
  static int tlCompFunc(void const* a, void const* b)
  {
    return compFunc(*(T**) a, *(T**) b);
  }
  int& maxSize()
  {return ((int*) items)[-1];}
  int& size()
  {return ((int*) items)[-2];}
  int& ref()
  {return ((int*) items)[-3];}
  int const& maxSize() const
  {return ((int*) items)[-1];}
  int const& size() const
  {return ((int*) items)[-2];}
  int const& ref() const
  {return ((int*) items)[-3];}
  void splice()
  {
    if (ref() <= 1)
      return;
    ref()--;
    int mxs = maxSize();
    int s = size();
    char* temp = (char*) malloc(sizeof(T*) * mxs + 12) + 12;
    for (int i = 0; i < s; i++)
      ((T**) temp)[i] = new T(*((T**) items)[i]);
    items = temp;
    maxSize() = mxs;
    size() = s;
    ref() = 1;
  }
  char* items;
public:
  PtrArray(int space = 8)
  {
    items = (char*) malloc(sizeof(T*) * space + 12) + 12;
    maxSize() = space;
    size() = 0;
    ref() = 1;
  }
  ~PtrArray()
  {
    if (--ref() <= 0)
    {
      for (int i = 0; i < size(); i++)
        delete ((T**) items)[i];
      free(items - 12);
    }
  }
  PtrArray(PtrArray<T> const& a)
  {
    items = a.items;
    ref()++;
  }

  PtrArray& operator = (PtrArray const& a)
  {
    if (&a == this)
      return *this;
    if (--ref() <= 0)
    {
      for (int i = 0; i < size(); i++)
        delete ((T**) items)[i];
      free(items - 12);
    }
    items = a.items;
    ref()++;
    return *this;
  }

  int length() const
  {
    return size();
  }

  T const& operator [] (int i) const
  {
    splice();
    return *((T**) items)[i];
  }
  T& operator [] (int i)
  {
    return *((T**) items)[i];
  }

  void clear()
  {
    splice();
    while (size() > 0)
      delete ((T**) items)[--size()];
  }
  void reserve(int space)
  {
    if (space > maxSize())
    {
      splice();
      items = (char*) realloc(items - 12, sizeof(T*) * space + 12) + 12;
      maxSize() = space;
    }
  }
  void resize(int new_size)
  {
    splice();
    reserve(new_size);
    while (size() < new_size)
      ((T**) items)[size()++] = new T;
    while (size() > new_size)
      delete ((T**) items)[--size()];
  }
  void resize(int new_size, T const& c)
  {
    splice();
    reserve(new_size);
    while (size() < new_size)
      ((T**) items)[size()++] = new T(c);
    while (size() > new_size)
      delete ((T**) items)[--size()];
  }
  int push(T const& e)
  {
    splice();
    reserve(size() + 1);
    ((T**) items)[size()] = new T(e);
    return size()++;
  }
  T& push()
  {
    splice();
    reserve(size() + 1);
    ((T**) items)[size()] = new T;
    return *((T**) items)[size()++];
  }
  void pop()
  {
    splice();
    delete ((T**) items)[--size()];
  }

  void sort(int (*comp) (T const& a, T const& b))
  {
    compFunc = comp;
    qsort(items, size(), sizeof(T*), tlCompFunc);
  }
};

template<class T>
int (*PtrArray<T>::compFunc) (T const* a, T const* b);

#endif // __BASE_ARRAY_H__
