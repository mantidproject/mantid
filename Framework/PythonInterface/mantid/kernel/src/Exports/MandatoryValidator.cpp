// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/MandatoryValidator.h"
#include <boost/python/class.hpp>
#include <string>

using Mantid::Kernel::IValidator;
using Mantid::Kernel::MandatoryValidator;
using namespace boost::python;

namespace {
/// A macro for generating exports for each type
#define EXPORT_MANDATORYVALIDATOR(ElementType, prefix)                                                                 \
  class_<MandatoryValidator<ElementType>, bases<IValidator>, boost::noncopyable>(#prefix "MandatoryValidator");
} // namespace

void export_MandatoryValidator() {
  EXPORT_MANDATORYVALIDATOR(double, Float);
  EXPORT_MANDATORYVALIDATOR(int, Int);
  EXPORT_MANDATORYVALIDATOR(std::string, String);

  // Array types
  EXPORT_MANDATORYVALIDATOR(std::vector<double>, FloatArray);
  EXPORT_MANDATORYVALIDATOR(std::vector<int>, IntArray);
  EXPORT_MANDATORYVALIDATOR(std::vector<std::string>, StringArray);
}
