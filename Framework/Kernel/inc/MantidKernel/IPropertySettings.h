// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include <string>

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

  /** The function user have to overload it in their custom code to modify the
   property
      according to the changes to other properties.
   *
   *  Currently it has been tested to modify the property values as function of
   other properties
   *
   *  Allowed property values are obtained from property's allowedValues
   function, and the purpose the
   *  function interfaced here is to modify its output.
   *
   *  allowedValues function on propertyWithValue class obtains its data from a
   validator, so in the case of
   *  simple PropertyWithValue, this function has to replace the validator.
   *  For WorkspaceProperty, which obtains its values from dataservice and
   filters them by validators,
   *  a new validator has to be a new filter      */
  virtual void applyChanges(const IPropertyManager *, Property *const) {}

  //--------------------------------------------------------------------------------------------
  /// Make a copy of the present type of IPropertySettings
  virtual IPropertySettings *clone() const = 0;

protected:
  // non-copyable directly
  IPropertySettings(const IPropertySettings &) = default;
};

} // namespace Kernel
} // namespace Mantid
