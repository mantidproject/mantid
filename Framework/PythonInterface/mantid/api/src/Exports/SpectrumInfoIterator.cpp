#include "MantidAPI/SpectrumInfoIterator.h"

#include <boost/python/class.hpp>
#include <boost/python/module.hpp>

using Mantid::API::SpectrumInfoIterator;
using namespace boost::python;

// Export SpectrumInfoIterator
void export_SpectrumInfoIterator() {

  // Export to Python
  class_<SpectrumInfoIterator>("SpectrumInfoIterator", no_init);
}
