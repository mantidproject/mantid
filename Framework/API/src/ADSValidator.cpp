#include "MantidAPI/ADSValidator.h"
#include "MantidKernel/TypedValidator.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidAPI/AnalysisDataService.h"

namespace Mantid {
namespace API {

/// Default constructor. Sets up an empty list of valid values.
ADSValidator::ADSValidator(const bool allowMultiSelection,
                           const bool isOptional)
    : TypedValidator<std::string>(), m_AllowMultiSelection(allowMultiSelection),
      m_isOptional(isOptional){};

/// Clone the validator
IValidator_sptr ADSValidator::clone() const {
  return boost::make_shared<ADSValidator>(*this);
}

bool ADSValidator::isMultipleSelectionAllowed() {
  return m_AllowMultiSelection;
}

void ADSValidator::setMultipleSelectionAllowed(
    const bool isMultiSelectionAllowed) {
  m_AllowMultiSelection = isMultiSelectionAllowed;
}

bool ADSValidator::isOptional() const { return m_isOptional; }

void ADSValidator::setOptional(const bool setOptional) {
  m_isOptional = setOptional;
}

/** Checks if the string passed is in the ADS, or if all members are in the ADS
*  @param value :: The value to test
*  @return "" if the value is on the list, or "The workspace is not in the
* workspace list"
*/
std::string ADSValidator::checkValidity(const std::string &value) const {
  if (!m_isOptional && isEmpty(value))
    return "Select a value";
  std::vector<std::string> ws_to_validate;
  ws_to_validate.push_back(value);
  if (m_AllowMultiSelection) {
    StringTokenizer tokenizer(
        value, ",|;\t", StringTokenizer::TOK_TRIM |
                            StringTokenizer::TOK_IGNORE_EMPTY |
                            StringTokenizer::TOK_IGNORE_FINAL_EMPTY_TOKEN);
    ws_to_validate = tokenizer.asVector();
  }
  auto &ads = AnalysisDataService::Instance();
  std::ostringstream os;
  for (std::string wsName : ws_to_validate) {
    if (!ads.doesExist(wsName)) {
      os << "The workspace \"" << value << "\" is not in the workspace list";
    }
  }
  return os.str();
}

/** Returns the current contents of the AnalysisDataService for input
* workspaces.
*  For output workspaces, an empty set is returned
*  @return set of objects in AnalysisDataService
*/
std::vector<std::string> ADSValidator::allowedValues() const {
  // Get the list of workspaces currently in the ADS
  auto vals = AnalysisDataService::Instance().getObjectNames();
  if (isOptional()) // Insert an empty option
  {
    vals.insert("");
  }
  auto values = std::vector<std::string>(vals.begin(), vals.end());
  std::sort(values.begin(), values.end());
  return values;
}

/**
* Is the value considered empty. Specialized string version to use empty
* @param value :: The value to check
* @return True if it is considered empty
*/
bool ADSValidator::isEmpty(const std::string &value) const {
  return value.empty();
}

} // namespace API
} // namespace Mantid
