#include "MantidKernel/MandatoryValidator.h"
#include <boost/python/class.hpp>
#include <string>

using Mantid::Kernel::MandatoryValidator;
using Mantid::Kernel::IValidator;
using namespace boost::python;

namespace {
/// A macro for generating exports for each type
#define EXPORT_MANDATORYVALIDATOR(ElementType, prefix)                         \
  class_<MandatoryValidator<ElementType>, bases<IValidator>,                   \
         boost::noncopyable>(#prefix "MandatoryValidator");
}

void export_MandatoryValidator() {
  EXPORT_MANDATORYVALIDATOR(double, Float);
  EXPORT_MANDATORYVALIDATOR(int64_t, Int);
  EXPORT_MANDATORYVALIDATOR(std::string, String);

  // Array types
  EXPORT_MANDATORYVALIDATOR(std::vector<double>, FloatArray);
  EXPORT_MANDATORYVALIDATOR(std::vector<int64_t>, IntArray);
  EXPORT_MANDATORYVALIDATOR(std::vector<std::string>, StringArray);
}
