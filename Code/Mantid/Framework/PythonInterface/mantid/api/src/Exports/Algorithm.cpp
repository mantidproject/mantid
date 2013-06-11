#ifdef _MSC_VER
  #pragma warning( disable: 4250 ) // Disable warning regarding inheritance via dominance, we have no way around it with the design
#endif
#include "MantidAPI/AlgorithmProxy.h"
#include "MantidAPI/Algorithm.h"

#include "MantidPythonInterface/kernel/SharedPtrToPythonMacro.h"

#include <boost/python/class.hpp>


using Mantid::API::IAlgorithm;
using Mantid::API::Algorithm;
using Mantid::API::AlgorithmProxy;

using namespace boost::python;

void export_algorithm()
{
  REGISTER_SHARED_PTR_TO_PYTHON(Algorithm);

  class_<Algorithm, bases<IAlgorithm>, boost::noncopyable>("Algorithm", "Base-class for C algorithms", no_init)
    .def("fromString", &Algorithm::fromString, "Initialize the algorithm from a string representation")
    .staticmethod("fromString")
    
    .def("setOptionalMessage", &Algorithm::setOptionalMessage)
    .def("createChildAlgorithm", &Algorithm::createChildAlgorithm, 
         (arg("name"),arg("startProgress")=-1.0,arg("endProgress")=-1.0,
          arg("enableLogging")=true,arg("version")=-1), "Creates and intializes a named child algorithm. Output workspaces are given a dummy name.")
    ;


  //---------------------------- AlgorithmProxy ----------------------------------
  REGISTER_SHARED_PTR_TO_PYTHON(AlgorithmProxy);

  class_<AlgorithmProxy, bases<IAlgorithm>, boost::noncopyable>("AlgorithmProxy", "Proxy class returned by managed algorithms", no_init);
}

#ifdef _MSC_VER
  #pragma warning( default: 4250 )
#endif
