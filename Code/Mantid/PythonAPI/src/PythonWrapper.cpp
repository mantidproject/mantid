//------------------------
// Includes
//------------------------
// Boost
#include <boost/python/module.hpp>
#include "MantidPythonAPI/stl_proxies.h"

// I'll put this back in when we start to do something with it
//#include "MantidPythonAPI/PyAlgorithm.h"

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

  //Standard containers first
  // A vector of ints
  vector_proxy<int>::wrap("stl_vector_integer");
  // A vector of doubles
  vector_proxy<double>::wrap("stl_vector_double");
  // A vector of strings
  vector_proxy<std::string>::wrap("stl_vector_string");

  // Export functions
  export_kernel_namespace();
  export_geometry_namespace();
  export_api_namespace();
}   

// //========================================= Python algorithms ======================================
//    /** I'll put this back in when we start doing something with them */
//    //    //PyAlgorithm Class
//    //    class_< Mantid::PythonAPI::PyAlgorithm, boost::noncopyable, Mantid_PythonAPI_PyAlgorithm_Wrapper >("PyAlgorithm", init< std::string >())
//    //      .def("name", &Mantid::PythonAPI::PyAlgorithm::name, &Mantid_PythonAPI_PyAlgorithm_Wrapper::default_name)
//    //      .def("PyInit", &Mantid::PythonAPI::PyAlgorithm::PyInit, &Mantid_PythonAPI_PyAlgorithm_Wrapper::default_PyInit)
//    //      .def("PyExec", &Mantid::PythonAPI::PyAlgorithm::PyExec, &Mantid_PythonAPI_PyAlgorithm_Wrapper::default_PyExec)
// //      ;


