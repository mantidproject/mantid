//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAPI/WorkspaceValidators.h"

namespace Mantid {
using Kernel::Strings::join;

namespace API {

//------------------------------------------------------------------------------
// InstrumentValidator
//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
// SampleValidator
//------------------------------------------------------------------------------
/**
 * Construct a validator with a set of requirements
 * @param flags A combination of SampleValidator::Requirements flags. Defaults
 * to requiring both a shape and material
 */
SampleValidator::SampleValidator(const unsigned int flags)
    : m_requires(flags) {}

/**
 * @return A string type identifier for the object
 */
std::string SampleValidator::getType() const { return "Sample"; }

/**
 * @return A copy of the current state of the object
 */
Kernel::IValidator_sptr SampleValidator::clone() const {
  return boost::make_shared<SampleValidator>(*this);
}

/**
 * Check if the workspace satisfies the validation requirements
 * @param value A pointer to a MatrixWorkspace object
 * @return An empty string if the requirements are satisfied otherwise
 * a string containing a message indicating the problem(s)
 */
std::string
SampleValidator::checkValidity(const MatrixWorkspace_sptr &value) const {
  const auto &shape = value->sample().getShape();
  std::list<std::string> missing;
  if ((m_requires & Shape) && !shape.hasValidShape())
    missing.emplace_back("shape");
  const auto &material = shape.material();
  if ((m_requires & Material) && material.name().empty())
    missing.emplace_back("material");

  if (missing.empty())
    return "";
  else {
    return "The sample is missing the following properties: " +
           join(missing.begin(), missing.end(), ",");
  }
}
}
}
