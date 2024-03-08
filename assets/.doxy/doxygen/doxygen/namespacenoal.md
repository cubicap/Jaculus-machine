

# Namespace noal



[**Namespace List**](namespaces.md) **>** [**noal**](namespacenoal.md)




















## Classes

| Type | Name |
| ---: | :--- |
| class | [**callableany**](classnoal_1_1callableany.md) &lt;typename Func, typename Sign&gt;<br> |
| class | [**callableany&lt; Func, Res(Args...)&gt;**](classnoal_1_1callableany_3_01Func_00_01Res_07Args_8_8_8_08_4.md) &lt;typename Func, typename Res, Args&gt;<br> |
| class | [**funcptr**](classnoal_1_1funcptr.md) &lt;typename Res, Args&gt;<br> |
| class | [**function**](classnoal_1_1function.md) &lt;typename Sign, dataSize&gt;<br> |
| class | [**function&lt; Res(Args...), dataSize &gt;**](classnoal_1_1function_3_01Res_07Args_8_8_8_08_00_01dataSize_01_4.md) &lt;typename Res, Args, dataSize&gt;<br> |
| class | [**memberconstfuncptr**](classnoal_1_1memberconstfuncptr.md) &lt;class Class, typename Res, Args&gt;<br> |
| class | [**memberfuncptr**](classnoal_1_1memberfuncptr.md) &lt;class Class, typename Res, Args&gt;<br> |
| struct | [**signatureHelper**](structnoal_1_1signatureHelper.md) &lt;typename Func&gt;<br> |
| struct | [**signatureHelper&lt; Res(Func::\*)(Args...) & &gt;**](structnoal_1_1signatureHelper_3_01Res_07Func_1_1_5_08_07Args_8_8_8_08_01_6_01_4.md) &lt;typename Func, typename Res, Args&gt;<br> |
| struct | [**signatureHelper&lt; Res(Func::\*)(Args...) const & &gt;**](structnoal_1_1signatureHelper_3_01Res_07Func_1_1_5_08_07Args_8_8_8_08_01const_01_6_01_4.md) &lt;typename Func, typename Res, Args&gt;<br> |
| struct | [**signatureHelper&lt; Res(Func::\*)(Args...) const &gt;**](structnoal_1_1signatureHelper_3_01Res_07Func_1_1_5_08_07Args_8_8_8_08_01const_01_4.md) &lt;typename Func, typename Res, Args&gt;<br> |
| struct | [**signatureHelper&lt; Res(Func::\*)(Args...)&gt;**](structnoal_1_1signatureHelper_3_01Res_07Func_1_1_5_08_07Args_8_8_8_08_4.md) &lt;typename Func, typename Res, Args&gt;<br> |






















## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**callableany**](#function-callableany) (Func) <br> |
|   | [**function**](#function-function) (Res(\*)(Args...)) <br> |
|   | [**function**](#function-function) (Res(Class::\*)(Args...), Class \*) <br> |
|   | [**function**](#function-function) (Func) <br> |
|  Res | [**invoker**](#function-invoker) (void \* func, Args... args) <br> |




























## Public Functions Documentation




### function callableany 

```C++
template<typename Func typename Func, typename Sign typename Sign>
noal::callableany (
    Func
) 
```






### function function 

```C++
template<typename Res typename Res, typename... Args>
noal::function (
    Res(*)(Args...)
) 
```






### function function 

```C++
template<class Class class Class, typename Res typename Res, typename... Args>
noal::function (
    Res(Class::*)(Args...),
    Class *
) 
```






### function function 

```C++
template<typename Func typename Func, typename Sign typename Sign>
noal::function (
    Func
) 
```






### function invoker 

```C++
template<typename Res typename Res, typename... Args>
Res noal::invoker (
    void * func,
    Args... args
) 
```




------------------------------
The documentation for this class was generated from the following file `src/noal_func.h`

