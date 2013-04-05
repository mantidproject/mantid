#include "MantidAPI/AlgorithmProperty.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidPythonInterface/kernel/PropertyWithValue.h"
#include <boost/python/class.hpp>

using Mantid::API::AlgorithmProperty;
using Mantid::API::IAlgorithm;
using Mantid::Kernel::PropertyWithValue;
using namespace boost::python;

void export_AlgorithmProperty()
{
  // AlgorithmProperty has base PropertyWithValue<boost::shared_ptr<IAlgorithm>>
  // which must be exported
  typedef boost::shared_ptr<IAlgorithm> HeldType;
  EXPORT_PROP_W_VALUE(HeldType, IAlgorithm);

  class_<AlgorithmProperty, bases<PropertyWithValue<HeldType>>, boost::noncopyable>("AlgorithmProperty", no_init)
    ;
}

