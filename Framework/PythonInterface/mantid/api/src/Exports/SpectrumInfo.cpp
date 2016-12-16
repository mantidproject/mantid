#include "MantidAPI/SpectrumInfo.h"
#include <boost/python/class.hpp>

using Mantid::API::SpectrumInfo;
using namespace boost::python;

void export_SpectrumInfo() {
  class_<SpectrumInfo, boost::noncopyable>("SpectrumInfo", no_init)
      .def("isMasked", &SpectrumInfo::isMasked, (arg("self"), arg("index")),
           "Returns true if the detector(s) associated with the spectrum are "
           "masked.");
}
