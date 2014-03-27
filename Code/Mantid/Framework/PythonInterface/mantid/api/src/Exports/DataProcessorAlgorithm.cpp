#include "MantidPythonInterface/api/PythonAlgorithm/DataProcessorAdapter.h"
#include <boost/python/class.hpp>

using Mantid::API::Algorithm;
using Mantid::API::DataProcessorAlgorithm;
using Mantid::PythonInterface::DataProcessorAdapter;
using namespace boost::python;

void export_DataProcessorAlgorithm()
{
  class_<DataProcessorAlgorithm, bases<Algorithm>, boost::shared_ptr<DataProcessorAdapter>,
         boost::noncopyable>("DataProcessorAlgorithm", "Base class workflow-type algorithms")
    ;
}

