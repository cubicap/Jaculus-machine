

# Class noal::memberfuncptr

**template &lt;class Class class Class, typename Res typename Res, typename... Args&gt;**



[**ClassList**](annotated.md) **>** [**noal**](namespacenoal.md) **>** [**memberfuncptr**](classnoal_1_1memberfuncptr.md)










































## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**memberfuncptr**](#function-memberfuncptr) (Res(Class::\*)(Args...) func, Class \* self) <br> |
|  Res | [**operator()**](#function-operator()) (Args... args) <br> |


## Public Static Functions

| Type | Name |
| ---: | :--- |
|  Res | [**call**](#function-call) (void \* self, Args... args) <br> |


























## Public Functions Documentation




### function memberfuncptr 

```C++
inline noal::memberfuncptr::memberfuncptr (
    Res(Class::*)(Args...) func,
    Class * self
) 
```






### function operator() 

```C++
inline Res noal::memberfuncptr::operator() (
    Args... args
) 
```



## Public Static Functions Documentation




### function call 

```C++
static inline Res noal::memberfuncptr::call (
    void * self,
    Args... args
) 
```




------------------------------
The documentation for this class was generated from the following file `src/noal_func.h`

