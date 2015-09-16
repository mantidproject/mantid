#include "MantidQtCustomInterfaces/SANSConstants.h"
#include <limits>

namespace MantidQt
{
namespace CustomInterfaces
{
SANSConstants::SANSConstants() {}

SANSConstants::~SANSConstants() {}

/**
 * Defines the python keyword for a succssful operation
 * @returns the keyword for success
 */
QString SANSConstants::getPythonSuccessKeyword() {
  const static QString pythonSuccessKeyword = "pythonExecutionWasSuccessful";
  return pythonSuccessKeyword;
}

/**
 * Defines the python keyword for an empty object , ie None
 * @returns the python keyword for an empty object
 */
QString SANSConstants::getPythonEmptyKeyword() {
  const static QString pythonSuccessKeyword = "None";
  return pythonSuccessKeyword;
}

/**
 * Defines the python keyword for true , ie True
 * @returns the python true keyword
 */
QString SANSConstants::getPythonTrueKeyword() {
  const static QString pythonSuccessKeyword = "True";
  return pythonSuccessKeyword;
}

/**
 * Gets the max double value
 * @returns the max double
 */
double SANSConstants::getMaxDoubleValue() {
    return std::numeric_limits<double>::max();
}

/**
 * Get the number of decimals
 * @returns the number of decimals
 */
int SANSConstants::getDecimals() {
  return 8;
}

/**
 * Get the max integer value
 * @returns the max integer value
 */
int SANSConstants::getMaxIntValue() {
  return std::numeric_limits<int>::max();
}

}
}