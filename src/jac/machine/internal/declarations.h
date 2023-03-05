#pragma once


namespace jac {


template<bool managed>
class ValueWrapper;
template<bool managed>
class ObjectWrapper;
template<bool managed>
class ArrayWrapper;
template<bool managed>
class FunctionWrapper;
template<bool managed>
class PromiseWrapper;
template<bool managed>
class ExceptionWrapper;


using Value = ValueWrapper<true>;        // value/strong reference
using ValueConst = ValueWrapper<false>;  // weak reference

using Object = ObjectWrapper<true>;
using ObjectConst = ObjectWrapper<false>;

using Function = FunctionWrapper<true>;
using FunctionConst = FunctionWrapper<false>;

using Array = ArrayWrapper<true>;
using ArrayConst = ArrayWrapper<false>;

using Promise = PromiseWrapper<true>;
using PromiseConst = PromiseWrapper<false>;

using Exception = ExceptionWrapper<true>;
using ExceptionConst = ExceptionWrapper<false>;


}  // namespace jac
