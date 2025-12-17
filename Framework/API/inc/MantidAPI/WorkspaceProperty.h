// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/PropertyWithValue.h"

#include <string>

namespace Mantid {
namespace API {
// -------------------------------------------------------------------------
// Forward decaration
// -------------------------------------------------------------------------
class Workspace;
class MatrixWorkspace;
class WorkspaceGroup;
class IEventWorkspace;
class IMDEventWorkspace;
class IMDHistoWorkspace;
class IMDWorkspace;
class IPeaksWorkspace;
class ITableWorkspace;

/// Enumeration for locking behaviour
struct LockMode {
  enum Type { Lock, NoLock };
};

/** A property class for workspaces. Inherits from PropertyWithValue, with the
value being
a pointer to the workspace type given to the WorkspaceProperty constructor. This
kind
of property also holds the name of the workspace (as used by the
AnalysisDataService)
and an indication of whether it is an input or output to an algorithm (or both).

The pointers to the workspaces are fetched from the ADS when the properties are
validated
(i.e. when the PropertyManager::validateProperties() method calls isValid() ).
Pointers to output workspaces are also fetched, if they exist, and can then be
used within
an algorithm. (An example of when this might be useful is if the user wants to
write the
output into the same workspace as is used for input - this avoids creating a new
workspace
and the overwriting the old one at the end.)

@author Russell Taylor, Tessella Support Services plc
@date 10/12/2007
*/
template <typename TYPE = MatrixWorkspace>
class WorkspaceProperty : public Kernel::PropertyWithValue<std::shared_ptr<TYPE>>, public IWorkspaceProperty {
public:
  explicit WorkspaceProperty(
      const std::string &name, const std::string &wsName, const unsigned int direction,
      const Kernel::IValidator_sptr &validator = Kernel::IValidator_sptr(new Kernel::NullValidator));

  explicit WorkspaceProperty(
      const std::string &name, const std::string &wsName, const unsigned int direction,
      const PropertyMode::Type optional,
      const Kernel::IValidator_sptr &validator = Kernel::IValidator_sptr(new Kernel::NullValidator));

  explicit WorkspaceProperty(
      const std::string &name, const std::string &wsName, const unsigned int direction,
      const PropertyMode::Type optional, const LockMode::Type locking,
      const Kernel::IValidator_sptr &validator = Kernel::IValidator_sptr(new Kernel::NullValidator));

  WorkspaceProperty(const WorkspaceProperty &right);

  WorkspaceProperty &operator=(const WorkspaceProperty &right);

  WorkspaceProperty &operator=(const std::shared_ptr<TYPE> &value) override;

  WorkspaceProperty &operator+=(Kernel::Property const *) override;

  WorkspaceProperty<TYPE> *clone() const override;

  std::string value() const override;

  Json::Value valueAsJson() const override;

  bool isValueSerializable() const override;

  std::string getDefault() const override;

  std::string setValue(const std::string &value) override;

  std::string setValueFromJson(const Json::Value &value) override;

  std::string setDataItem(const std::shared_ptr<Kernel::DataItem> &value) override;

  void setPropertyMode(const PropertyMode::Type &optional) override;

  std::string isValid() const override;

  bool isDefault() const override;

  bool isOptional() const override;
  bool isLocking() const override;

  std::vector<std::string> allowedValues() const override;

  const Kernel::PropertyHistory createHistory() const override;

  bool store() override;

  Workspace_sptr getWorkspace() const override;

private:
  std::string isValidGroup(const std::shared_ptr<WorkspaceGroup> &wsGroup) const;

  std::string isValidOutputWs() const;

  std::string isOptionalWs() const;

  void clear() override;

  void retrieveWorkspaceFromADS();

  /// The name of the workspace (as used by the AnalysisDataService)
  std::string m_workspaceName;
  /// The name of the workspace that the this this object was created for
  std::string m_initialWSName;
  /// A flag indicating whether the property should be considered optional. Only
  /// matters for input workspaces
  PropertyMode::Type m_optional;
  /** A flag indicating whether the workspace should be read or write-locked
   * when an algorithm begins. Default=true. */
  LockMode::Type m_locking;

  /// for access to logging streams
  static Kernel::Logger g_log;
};

template <typename TYPE> Kernel::Logger WorkspaceProperty<TYPE>::g_log("WorkspaceProperty");

#ifndef API_EXPORTS
// Extern declarations to ensure typeinfo visibility for dynamic_cast on macOS/Clang
extern template class MANTID_API_DLL WorkspaceProperty<Workspace>;
extern template class MANTID_API_DLL WorkspaceProperty<IEventWorkspace>;
extern template class MANTID_API_DLL WorkspaceProperty<IMDEventWorkspace>;
extern template class MANTID_API_DLL WorkspaceProperty<IMDHistoWorkspace>;
extern template class MANTID_API_DLL WorkspaceProperty<IMDWorkspace>;
extern template class MANTID_API_DLL WorkspaceProperty<MatrixWorkspace>;
extern template class MANTID_API_DLL WorkspaceProperty<IPeaksWorkspace>;
extern template class MANTID_API_DLL WorkspaceProperty<ITableWorkspace>;
extern template class MANTID_API_DLL WorkspaceProperty<WorkspaceGroup>;
#endif

} // namespace API
} // namespace Mantid
