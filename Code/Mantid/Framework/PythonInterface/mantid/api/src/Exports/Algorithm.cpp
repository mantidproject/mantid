#ifdef _MSC_VER
  #pragma warning( disable: 4250 ) // Disable warning regarding inheritance via dominance, we have no way around it with the design
#endif
#include "MantidAPI/AlgorithmProxy.h"
#include "MantidAPI/Algorithm.h"
#ifdef _MSC_VER
  #pragma warning( default: 4250 )
#endif

#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::API::IAlgorithm;
using Mantid::API::Algorithm;
using Mantid::API::AlgorithmProxy;

using namespace boost::python;

void export_algorithm()
{
  register_ptr_to_python<boost::shared_ptr<Algorithm> >();
  class_<Algorithm, bases<IAlgorithm>, boost::noncopyable>("Algorithm", "Base-class for C algorithms", no_init);

  register_ptr_to_python<boost::shared_ptr<AlgorithmProxy> >();
  class_<AlgorithmProxy, bases<IAlgorithm>, boost::noncopyable>("AlgorithmProxy", "Proxy class returned by managed algorithms", no_init);
}

