//------------------------
// Includes
//------------------------
// Boost
#include "MantidPythonAPI/BoostPython_Silent.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidPythonAPI/stl_proxies.h"
#include "MantidPythonAPI/MantidVecHelper.h"

// Noisy MSVC compiler that complains about Boost stuff we can do nothing about. For some reason
// it wouldn't accept it in the BoostPython_Silent.h header
#ifdef _MSC_VER
  #pragma warning(disable:4503)
#endif

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

  // MG: Must be removed as it is a plugin
  //  void export_crystal_namespace();
} 
}

const char* mantid_version()
{
  return MANTID_VERSION;
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

  boost::python::def("mantid_build_version", mantid_version);
  MantidVecHelper::initializeDependencies();
  // Export some frequently used stl containers
  vector_proxy<Mantid::Kernel::DateAndTime>::wrap("cpp_list_dateandtime");
  vector_proxy<int>::wrap("cpp_list_int");
  vector_proxy<long>::wrap("cpp_list_long");
  vector_proxy<double>::wrap("cpp_list_dbl");
  vector_proxy<bool>::wrap("cpp_list_bool");
  vector_proxy<std::string>::wrap("cpp_list_str");
  set_proxy<std::string>::wrap("cpp_set_string");

  // Export Mantid API
  export_kernel_namespace();
  export_geometry_namespace();
  export_api_namespace();

  // MG: Must be removed as it is a plugin
  //  export_crystal_namespace();
}   


