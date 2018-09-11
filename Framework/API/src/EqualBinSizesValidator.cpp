#include "MantidAPI/EqualBinSizesValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/EqualBinsChecker.h"

namespace Mantid {
namespace API {

/**
 * Constructor - sets properties
 */
EqualBinSizesValidator::EqualBinSizesValidator(const double errorLevel,
                                               const double warningLevel)
    : m_errorLevel(errorLevel), m_warningLevel(warningLevel) {}

/// Clone the current state
Kernel::IValidator_sptr EqualBinSizesValidator::clone() const {
  return boost::make_shared<EqualBinSizesValidator>(*this);
}

/** Checks that the bin sizes of each histogram in the workspace are the same
 * @param value :: [input] The workspace to test
 * @return :: An error message (empty if no error)
 */
std::string
EqualBinSizesValidator::checkValidity(const MatrixWorkspace_sptr &value) const {
  if (!value)
    return "Enter an existing workspace";
  if (!value->isCommonBins())
    return "The workspace must have common bin boundaries for all histograms";
  if (value->getNumberHistograms() == 0 || value->blocksize() == 0)
    return "Enter a workspace with some data in it";

  Kernel::EqualBinsChecker checker(value->readX(0), m_errorLevel,
                                   m_warningLevel);
  return checker.validate();
}

} // namespace API
} // namespace Mantid
