#include "SANSConstants.h"
#include <limits>

namespace MantidQt {
namespace CustomInterfaces {
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
  const static QString pythonEmptyKeyword = "None";
  return pythonEmptyKeyword;
}

/**
 * Defines the python keyword for true , ie True
 * @returns the python true keyword
 */
QString SANSConstants::getPythonTrueKeyword() {
  const static QString pythonTrueKeyword = "True";
  return pythonTrueKeyword;
}

/**
 * Defines the python keyword for false , ie False
 * @returns the python false keyword
 */
QString SANSConstants::getPythonFalseKeyword() {
  const static QString pythonFalseKeyword = "False";
  return pythonFalseKeyword;
}

/**
 * Gets the tooltip for h11 for QResolution
 * @returns tooltip text for h1
 */
QString SANSConstants::getQResolutionH1ToolTipText() {
  const QString qResolutionH1ToolTipText =
      "The height of the first aperture in mm.";
  return qResolutionH1ToolTipText;
}

/**
 * Gets the tooltip for h2 for QResolution
 * @returns tooltip text for h2
 */
QString SANSConstants::getQResolutionH2ToolTipText() {
  const QString qResolutionH2ToolTipText =
      "The height of the seoncd aperture in mm.";
  return qResolutionH2ToolTipText;
}

/**
 * Gets the tooltip for a1 for QResolution
 * @returns tooltip text for a1
 */
QString SANSConstants::getQResolutionA1ToolTipText() {
  const QString qResolutionA1ToolTipText =
      "The diameter for the first aperture";
  return qResolutionA1ToolTipText;
}

/**
 * Gets the tooltip for a2 for QResolution
 * @returns tooltip text for a2
 */
QString SANSConstants::getQResolutionA2ToolTipText() {
  const QString qResolutionA2ToolTipText =
      "The diameter for the second aperture";
  return qResolutionA2ToolTipText;
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
int SANSConstants::getDecimals() { return 6; }

/**
 * Get the max integer value
 * @returns the max integer value
 */
int SANSConstants::getMaxIntValue() { return std::numeric_limits<int>::max(); }
} // namespace CustomInterfaces
} // namespace MantidQt
