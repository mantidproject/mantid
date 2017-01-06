#include "MantidAPI/SampleValidator.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/Strings.h"

namespace Mantid {
namespace API {
using Kernel::Strings::join;

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

} // namespace API
} // namespace Mantid
