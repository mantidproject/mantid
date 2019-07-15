// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Strings.h"
#include <boost/make_shared.hpp>
#include <list>

namespace Mantid {
namespace API {
using Kernel::Strings::join;

/**
 * Construct a validator with requirements (default = SamplePosition)
 * @param flags A combination of flags to specify requirements
 */
InstrumentValidator::InstrumentValidator(const unsigned int flags)
    : m_requires(flags) {}

/**
 * @return A string type identifier for the object
 */
std::string InstrumentValidator::getType() const { return "Instrument"; }

/**
 * @return A copy of the current state of the object
 */
Kernel::IValidator_sptr InstrumentValidator::clone() const {
  return boost::make_shared<InstrumentValidator>(*this);
}

/** Checks that the workspace has an instrument defined
 *  @param value :: The workspace to test
 *  @return A user-level description if a problem exists or ""
 */
std::string InstrumentValidator::checkValidity(
    const boost::shared_ptr<ExperimentInfo> &value) const {
  const auto inst = value->getInstrument();
  if (!inst)
    return "The workspace must have an instrument defined";

  std::list<std::string> missing;
  if ((m_requires & SourcePosition) && !inst->getSource()) {
    missing.emplace_back("source");
  }
  if ((m_requires & SamplePosition) && !inst->getSample()) {
    missing.emplace_back("sample holder");
  }

  if (missing.empty())
    return "";
  else {
    return "The instrument is missing the following "
           "components: " +
           join(missing.begin(), missing.end(), ",");
  }
}

} // namespace API
} // namespace Mantid
