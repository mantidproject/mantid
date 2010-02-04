//------------------------
// Includes
//------------------------
// Boost
#include <boost/python/module.hpp>
#include "MantidPythonAPI/stl_proxies.h"

namespace Mantid 
{
namespace PythonAPI
{
  /**
   * The boost documentation recommends splitting the wrappers up to reduce the memory footprint of the compilation
   * These are forward declarations of functions in *_exports.cpp files that actually do the exporting
   */
  void export_kernel_namespace();
  void export_geometry_namespace();
  void export_api_namespace();
} 
}

/**
 * The actual module definition begins here. The names are different for
 * Windows and Linux due to the difference in library names
 */
#ifdef _WIN32
BOOST_PYTHON_MODULE(MantidPythonAPI)
#else
BOOST_PYTHON_MODULE(libMantidPythonAPI)
#endif
{
  using namespace Mantid::PythonAPI;

  // Standard containers first
  // A vector of ints
  vector_proxy<int>::wrap("cpp_list_int");
  // A vector of doubles
  vector_proxy<double>::wrap("cpp_list_dbl");
  // A vector of bool
  vector_proxy<bool>::wrap("cpp_list_bool");
  // A vector of strings
  vector_proxy<std::string>::wrap("cpp_list_str");
  // A set of strings
  set_proxy<std::string>::wrap("cpp_set_string");

  // Export functions
  export_kernel_namespace();
  export_geometry_namespace();
  export_api_namespace();
}   


