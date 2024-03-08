

# Class jac::ExceptionWrapper

**template &lt;bool managed&gt;**



[**ClassList**](annotated.md) **>** [**jac**](namespacejac.md) **>** [**ExceptionWrapper**](classjac_1_1ExceptionWrapper.md)



_An exception wrapper which can either wrap a JSValue or contain an exception description and can be thrown into JS as a specific Error type._ [More...](#detailed-description)

* `#include <values.h>`



Inherits the following classes: [jac::ValueWrapper](classjac_1_1ValueWrapper.md),  std::exception














## Public Types

| Type | Name |
| ---: | :--- |
| enum  | [**Type**](#enum-type)  <br> |








































## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**ExceptionWrapper**](#function-exceptionwrapper-23) ([**ValueWrapper**](classjac_1_1ValueWrapper.md)&lt; managed &gt; value) <br>_Wrap an existing JSValue. If managed is true, JSValue will be freed when the Exception is destroyed._  |
|   | [**ExceptionWrapper**](#function-exceptionwrapper-33) ([**ContextRef**](classjac_1_1ContextRef.md) ctx, JSValue val) <br> |
|  std::string | [**stackTrace**](#function-stacktrace) () noexcept<br>_Get the exception stack trace._  |
|  JSValue | [**throwJS**](#function-throwjs) ([**ContextRef**](classjac_1_1ContextRef.md) ctx) <br>_Throw the exception into JS._  |
|  const char \* | [**what**](#function-what) () noexcept override const<br>_Get the exception message._  |


## Public Functions inherited from jac::ValueWrapper

See [jac::ValueWrapper](classjac_1_1ValueWrapper.md)

| Type | Name |
| ---: | :--- |
|   | [**ValueWrapper**](#function-valuewrapper-13) ([**ContextRef**](classjac_1_1ContextRef.md) ctx, JSValue val) <br>_Wrap an existing JSValue. If managed is true, JSValue will be freed when the Value is destroyed._  |
|   | [**ValueWrapper**](#function-valuewrapper-23) (const [**ValueWrapper**](classjac_1_1ValueWrapper.md) & other) <br> |
|   | [**ValueWrapper**](#function-valuewrapper-33) ([**ValueWrapper**](classjac_1_1ValueWrapper.md) && other) <br> |
|  JSValue & | [**getVal**](#function-getval) () <br>_Get reference to the underlying JSValue._  |
|  bool | [**isArray**](#function-isarray) () <br>_Check if the Value is an array._  |
|  bool | [**isFunction**](#function-isfunction) () <br>_Check if the Value is a function._  |
|  bool | [**isNull**](#function-isnull) () <br>_Check if the Value is null._  |
|  bool | [**isObject**](#function-isobject) () <br>_Check if the Value is an object._  |
|  bool | [**isUndefined**](#function-isundefined) () <br>_Check if the Value is undefined._  |
|  std::pair&lt; [**ContextRef**](classjac_1_1ContextRef.md), JSValue &gt; | [**loot**](#function-loot) () <br>_Release ownership of the JSValue. The JSValue will have to be freed manually._  |
|   | [**operator ValueWeak**](#function-operator-valueweak) () <br> |
|  [**ValueWrapper**](classjac_1_1ValueWrapper.md) & | [**operator=**](#function-operator) (const [**ValueWrapper**](classjac_1_1ValueWrapper.md) & other) <br> |
|  [**ValueWrapper**](classjac_1_1ValueWrapper.md) & | [**operator=**](#function-operator_1) ([**ValueWrapper**](classjac_1_1ValueWrapper.md) && other) <br> |
|  [**Value**](classjac_1_1ValueWrapper.md) | [**stringify**](#function-stringify) (int indent=0) <br>_Convert the Value to a JSON representation._  |
|  T | [**to**](#function-to) () <br>_Convert the Value to a specified type._  |
|  [**StringView**](classjac_1_1StringView.md) | [**toString**](#function-tostring) () <br>_Convert the Value to a_ [_**StringView**_](classjac_1_1StringView.md) _._ |
|   | [**~ValueWrapper**](#function-valuewrapper) () <br> |


## Public Static Functions

| Type | Name |
| ---: | :--- |
|  [**Exception**](classjac_1_1ExceptionWrapper.md) | [**create**](#function-create) (Type type, std::string message) <br>_Create a new Exception._  |


## Public Static Functions inherited from jac::ValueWrapper

See [jac::ValueWrapper](classjac_1_1ValueWrapper.md)

| Type | Name |
| ---: | :--- |
|  [**Value**](classjac_1_1ValueWrapper.md) | [**from**](#function-from) ([**ContextRef**](classjac_1_1ContextRef.md) ctx, T val) <br>_Create a new Value by converting a given value._  |
|  [**Value**](classjac_1_1ValueWrapper.md) | [**fromJSON**](#function-fromjson) ([**ContextRef**](classjac_1_1ContextRef.md) ctx, std::string json, std::string filename="&lt;json&gt;", bool extended=false) <br>_Create a new Value from a given JSON string._  |
|  [**Value**](classjac_1_1ValueWrapper.md) | [**null**](#function-null) ([**ContextRef**](classjac_1_1ContextRef.md) ctx) <br>_Create a new Value containing null._  |
|  [**Value**](classjac_1_1ValueWrapper.md) | [**undefined**](#function-undefined) ([**ContextRef**](classjac_1_1ContextRef.md) ctx) <br>_Create a new Value containing undefined._  |












## Protected Attributes inherited from jac::ValueWrapper

See [jac::ValueWrapper](classjac_1_1ValueWrapper.md)

| Type | Name |
| ---: | :--- |
|  [**ContextRef**](classjac_1_1ContextRef.md) | [**\_ctx**](#variable-_ctx)  <br> |
|  JSValue | [**\_val**](#variable-_val)  <br> |






































# Detailed Description




**Template parameters:**


* `managed` whether the JSValue should be freed when the wrapper is destroyed. 




    
## Public Types Documentation




### enum Type 

```C++
enum jac::ExceptionWrapper::Type {
    Any,
    Error,
    SyntaxError,
    TypeError,
    ReferenceError,
    RangeError,
    InternalError
};
```



## Public Functions Documentation




### function ExceptionWrapper [2/3]

_Wrap an existing JSValue. If managed is true, JSValue will be freed when the Exception is destroyed._ 
```C++
inline jac::ExceptionWrapper::ExceptionWrapper (
    ValueWrapper < managed > value
) 
```





**Note:**

Used internally when directly working with QuickJS API. New Exception should be created using [**Exception::create()**](classjac_1_1ExceptionWrapper.md#function-create) or by converting an existing Value to an Exception using [**Value::to&lt;Exception&gt;()**](classjac_1_1ValueWrapper.md#function-to).




**Parameters:**


* `ctx` context to work in 
* `val` JSValue to wrap 




        



### function ExceptionWrapper [3/3]

```C++
inline jac::ExceptionWrapper::ExceptionWrapper (
    ContextRef ctx,
    JSValue val
) 
```






### function stackTrace 

_Get the exception stack trace._ 
```C++
std::string jac::ExceptionWrapper::stackTrace () noexcept
```





**Returns:**

std::string containing the stack trace 





        



### function throwJS 

_Throw the exception into JS._ 
```C++
JSValue jac::ExceptionWrapper::throwJS (
    ContextRef ctx
) 
```





**Note:**

Used internally when directly working with QuickJS API. In most cases, exceptions should be thrown using a throw statement and wrapper functions will propagate the exception to JS. 




**Parameters:**


* `ctx` context to throw the exception in



**Returns:**

JSValue containing the exception 





        



### function what 

_Get the exception message._ 
```C++
inline const char * jac::ExceptionWrapper::what () noexcept override const
```





**Returns:**

std::string containing the exception message 





        
## Public Static Functions Documentation




### function create 

_Create a new Exception._ 
```C++
static inline Exception jac::ExceptionWrapper::create (
    Type type,
    std::string message
) 
```





**Parameters:**


* `type` type of the exception 
* `message` exception message 



**Returns:**

The resulting Exception 





        

------------------------------
The documentation for this class was generated from the following file `src/jac/machine/values.h`

