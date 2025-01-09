// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <utility>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidKernel/DataService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/IPropertySettings.h"
#include "MantidKernel/SingletonHolder.h"

namespace Mantid {
namespace API {

/** Show a property as enabled when the workspace pointed to by another
 * is of a given type

  @author Janik Zikovsky
  @date 2011-09-21
*/
template <typename T> class DLLExport EnabledWhenWorkspaceIsType : public Kernel::IPropertySettings {
public:
  //--------------------------------------------------------------------------------------------
  /** Constructor
   * @param otherPropName :: Name of the OTHER property that we will check.
   * @param enabledSetting :: Set Enabled on this property to this value when
   * the workspace is of type T. Default true.
   */
  EnabledWhenWorkspaceIsType(std::string otherPropName, bool enabledSetting = true)
      : IPropertySettings(), m_otherPropName(std::move(otherPropName)), m_enabledSetting(enabledSetting) {}

  //--------------------------------------------------------------------------------------------
  /** Does the validator fulfill the criterion based on the
   * other property values?
   * @return true if fulfilled or if any problem was found (missing property,
   * e.g.).
   */
  virtual bool checkCriterion(const Kernel::IPropertyManager *algo) const {
    // Find the property
    if (!algo)
      return true;
    Mantid::Kernel::Property *prop = nullptr;
    try {
      prop = algo->getPointerToProperty(m_otherPropName);
    } catch (Mantid::Kernel::Exception::NotFoundError &) {
      return true; // Property not found. Ignore
    }
    if (!prop)
      return true;

    // Value of the other property
    std::string propValue = prop->value();
    if (propValue.empty())
      return true;

    Workspace_sptr ws;
    try {
      ws = Mantid::API::AnalysisDataService::Instance().retrieve(propValue);
    } catch (...) {
      return true;
    }
    // Does it cast to the desired type?
    std::shared_ptr<T> castWS = std::dynamic_pointer_cast<T>(ws);
    if (castWS)
      return m_enabledSetting;
    else
      return !m_enabledSetting;
  }

  //--------------------------------------------------------------------------------------------
  /// Return true/false based on whether the other property satisfies the
  /// criterion
  bool isEnabled(const Kernel::IPropertyManager *algo) const override { return checkCriterion(algo); }

  //--------------------------------------------------------------------------------------------
  /// Return true always
  bool isVisible(const Kernel::IPropertyManager *) const override { return true; }

  //--------------------------------------------------------------------------------------------
  /// Make a copy of the present type of validator
  IPropertySettings *clone() const override {
    return new EnabledWhenWorkspaceIsType<T>(m_otherPropName, m_enabledSetting);
  }

protected:
  /// Name of the OTHER property that we will check.
  std::string m_otherPropName;
  /// Set Enabled to this.
  bool m_enabledSetting;
};

} // namespace API
} // namespace Mantid
