/**
 * Boost Python 1.43
 *
 * We need our old- and new-style Python interfaces to be able to coexist within
 * the same process. If each binds to boost python dynamically then they
 * share a single copy of the "to-python converter" registry (a static map)
 * and cannot be used together.
 *
 * This sharing can be avoided by statically linking the old-style API to boost
 * python so that each manages its own registry. While this works on Windows
 * there is a problem with using the static libraries that ship with the linux 
 * distributions as they have not been compiled with the correct flags to 
 * allow them to be statically linked in to another dynamic library.

 * Rather than recompiling and then trying to redistribute new libraries, it was
 * decided to simple include the boost python src as part of the compilation thereby
 * acheiving the same end result as static linking.

 * Single src file for all boost python source in order to kill compiler
 * warnings easily.
 */

#include "MantidKernel/WarningSuppressions.h"

#ifdef _MSC_VER
  #pragma warning(push)
  #pragma warning(disable:4244)
  #pragma warning(disable:4267)
  #pragma warning(disable:4250)
  #pragma warning(disable:4005)
  #pragma warning(disable:4503)
#endif

GCC_DIAG_OFF(conversion)
GCC_DIAG_OFF(missing-field-initializers)
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_OFF(type-limits)
GCC_DIAG_OFF(unused-value)
GCC_DIAG_OFF(strict-aliasing)
GCC_DIAG_OFF(uninitialized)
#if GCC_VERSION >= 40800 // 4.8.0
  GCC_DIAG_OFF(unused-local-typedefs)
#endif

#include "boostpython/converter/arg_to_python_base.cpp"
#include "boostpython/converter/builtin_converters.cpp"
#include "boostpython/converter/from_python.cpp"
#include "boostpython/converter/registry.cpp"
#include "boostpython/converter/type_id.cpp"

#include "boostpython/dict.cpp"
#include "boostpython/errors.cpp"
#include "boostpython/exec.cpp"
#include "boostpython/import.cpp"
#include "boostpython/list.cpp"
#include "boostpython/long.cpp"
#include "boostpython/module.cpp"
#include "boostpython/numeric.cpp"

#include "boostpython/object/class.cpp"
#include "boostpython/object/enum.cpp"
#include "boostpython/object/function.cpp"
#include "boostpython/object/function_doc_signature.cpp"
#include "boostpython/object/inheritance.cpp"
#include "boostpython/object/iterator.cpp"
#include "boostpython/object/life_support.cpp"
#include "boostpython/object/pickle_support.cpp"
#include "boostpython/object/stl_iterator.cpp"

#include "boostpython/object_operators.cpp"
#include "boostpython/object_protocol.cpp"
#include "boostpython/slice.cpp"
#include "boostpython/str.cpp"
#include "boostpython/tuple.cpp"
#include "boostpython/wrapper.cpp"

GCC_DIAG_ON(conversion)
GCC_DIAG_ON(missing-field-initializers)
GCC_DIAG_ON(unused-parameter)
GCC_DIAG_ON(type-limits)
GCC_DIAG_ON(unused-value)
GCC_DIAG_ON(strict-aliasing)
GCC_DIAG_ON(uninitialized)
#if GCC_VERSION >= 40800 // 4.8.0
  GCC_DIAG_ON(unused-local-typedefs)
#endif
