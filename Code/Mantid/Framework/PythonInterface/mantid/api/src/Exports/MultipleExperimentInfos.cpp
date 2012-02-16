#include "MantidAPI/MultipleExperimentInfos.h"
#include <boost/python/class.hpp>

using Mantid::API::MultipleExperimentInfos;
using namespace boost::python;

void export_MultipleExperimentInfos()
{
  class_<MultipleExperimentInfos,boost::noncopyable>("MultipleExperimentInfos", no_init)
    ;
}

