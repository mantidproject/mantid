// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include <string>
#include <vector>

namespace Mantid {
namespace Kernel {
//----------------------------------------------------------------------
// Forward Declaration
//----------------------------------------------------------------------
class IPropertyManager;
class Property;

/** Interface for modifiers to Property's that specify
  if they should be enabled or visible in a GUI.
  They are set on an algorithm via Algorithm::setPropertySettings()

  @author Janik Zikovsky
  @date 2011-08-26
*/
class MANTID_KERNEL_DLL IPropertySettings {
public:
  /// Constructor
  IPropertySettings() = default;

  /// Destructor
  virtual ~IPropertySettings() = default;

  /** Is the property to be shown as "enabled" in the GUI. Default true. */
  virtual bool isEnabled(const IPropertyManager *algo) const {
    UNUSED_ARG(algo);
    return true;
  }

  /** Is the property to be shown in the GUI? Default true. */
  virtual bool isVisible(const IPropertyManager *algo) const {
    UNUSED_ARG(algo);
    return true;
  }

  /** to verify if the properties, this one depends on have changed
      or other special condition occurs which needs the framework to react to */
  virtual bool isConditionChanged(const IPropertyManager *algo, const std::string &changedPropName = "") const {
    UNUSED_ARG(algo);
    UNUSED_ARG(changedPropName);
    return false;
  }

  /** Overload this virtual function in order to modify the
   *  current property based on changes to other properties.
   *
   *  @return: whether or not the current property was changed.
   */
  virtual bool applyChanges(const IPropertyManager *algo, const std::string &currentPropName) {
    UNUSED_ARG(algo);
    UNUSED_ARG(currentPropName);
    return false;
  }

  /// Other properties that this property depends on.
  virtual std::vector<std::string> dependsOn(const std::string & /* unused */) const {
    return std::vector<std::string>();
  }

  //--------------------------------------------------------------------------------------------
  /// Make a copy of the present type of IPropertySettings
  virtual IPropertySettings *clone() const = 0;

protected:
  // non-copyable directly
  IPropertySettings(const IPropertySettings &) = default;
};

} // namespace Kernel
} // namespace Mantid
