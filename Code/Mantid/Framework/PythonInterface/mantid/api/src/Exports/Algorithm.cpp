#ifdef _MSC_VER
  #pragma warning( disable: 4250 ) // Disable warning regarding inheritance via dominance, we have no way around it with the design
#endif
#include "MantidAPI/AlgorithmProxy.h"
#include "MantidAPI/Algorithm.h"

#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using namespace Mantid::API;
using namespace boost::python;

void export_algorithm()
{
  register_ptr_to_python<boost::shared_ptr<Algorithm>>();

  class_<Algorithm, bases<IAlgorithm>, boost::noncopyable>("Algorithm", "Base-class for C algorithms", no_init)
    .def("fromString", &Algorithm::fromString, "Initialize the algorithm from a string representation")
    .staticmethod("fromString")
    
    .def("setOptionalMessage", &Algorithm::setOptionalMessage)
    .def("createChildAlgorithm", &Algorithm::createChildAlgorithm, 
         (arg("name"),arg("startProgress")=-1.0,arg("endProgress")=-1.0,
          arg("enableLogging")=true,arg("version")=-1), "Creates and intializes a named child algorithm. Output workspaces are given a dummy name.")
    ;


  //---------------------------- AlgorithmProxy ----------------------------------
  register_ptr_to_python<boost::shared_ptr<AlgorithmProxy>>();

  class_<AlgorithmProxy, bases<IAlgorithm>, boost::noncopyable>("AlgorithmProxy", "Proxy class returned by managed algorithms", no_init);
}

#ifdef _MSC_VER
  #pragma warning( default: 4250 )
#endif
