

# Class jac::FilesystemFeature

**template &lt;class Next class Next&gt;**



[**ClassList**](annotated.md) **>** [**jac**](namespacejac.md) **>** [**FilesystemFeature**](classjac_1_1FilesystemFeature.md)








Inherits the following classes: Next












## Classes

| Type | Name |
| ---: | :--- |
| class | [**Path**](classjac_1_1FilesystemFeature_1_1Path.md) <br> |


## Public Types

| Type | Name |
| ---: | :--- |
| typedef [**Class**](classjac_1_1Class.md)&lt; [**FileProtoBuilder**](structjac_1_1FileProtoBuilder.md) &gt; | [**FileClass**](#typedef-fileclass)  <br> |




## Public Attributes

| Type | Name |
| ---: | :--- |
|  Fs | [**fs**](#variable-fs)  <br> |
|  [**Path**](classjac_1_1FilesystemFeature_1_1Path.md) | [**path**](#variable-path)  <br> |
















## Public Functions

| Type | Name |
| ---: | :--- |
|   | [**FilesystemFeature**](#function-filesystemfeature) () <br> |
|  void | [**initialize**](#function-initialize) () <br> |
|  void | [**setCodeDir**](#function-setcodedir) (std::string path\_) <br> |
|  void | [**setWorkingDir**](#function-setworkingdir) (std::string path\_) <br> |




























## Public Types Documentation




### typedef FileClass 

```C++
using jac::FilesystemFeature< Next >::FileClass =  Class<FileProtoBuilder>;
```



## Public Attributes Documentation




### variable fs 

```C++
Fs jac::FilesystemFeature< Next >::fs;
```






### variable path 

```C++
Path jac::FilesystemFeature< Next >::path;
```



## Public Functions Documentation




### function FilesystemFeature 

```C++
inline jac::FilesystemFeature::FilesystemFeature () 
```






### function initialize 

```C++
inline void jac::FilesystemFeature::initialize () 
```






### function setCodeDir 

```C++
inline void jac::FilesystemFeature::setCodeDir (
    std::string path_
) 
```






### function setWorkingDir 

```C++
inline void jac::FilesystemFeature::setWorkingDir (
    std::string path_
) 
```




------------------------------
The documentation for this class was generated from the following file `src/jac/features/filesystemFeature.h`

