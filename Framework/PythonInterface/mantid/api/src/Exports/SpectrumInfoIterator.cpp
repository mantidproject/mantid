// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
