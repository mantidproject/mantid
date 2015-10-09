//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAPI/AlgorithmHasProperty.h"
#include "MantidAPI/IAlgorithm.h"

namespace Mantid {
namespace API {
AlgorithmHasProperty::AlgorithmHasProperty(const std::string &propName)
    : m_propName(propName) {}

AlgorithmHasProperty::~AlgorithmHasProperty() {}

std::string AlgorithmHasProperty::checkValidity(
    const boost::shared_ptr<IAlgorithm> &value) const {
  std::string message("");
  if (value->existsProperty(m_propName)) {
    Kernel::Property *p = value->getProperty(m_propName);
    if (!p->isValid().empty()) {
      message = "Algorithm object contains the required property \"" +
                m_propName + "\" but it has an invalid value: " + p->value();
    }
  } else {
    message = "Algorithm object does not have the required property \"" +
              m_propName + "\"";
  }

  return message;
}

} // namespace Mantid
} // namespace API
