// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/PropertyHistory.h"
#include "MantidKernel/Strings.h"

#include <json/value.h>

namespace Mantid {
namespace API {

/** Constructor.
 *  Sets the property and workspace names but initializes the workspace pointer
 * to null.
 *  @param name :: The name to assign to the property
 *  @param wsName :: The name of the workspace
 *  @param direction :: Whether this is a Direction::Input, Direction::Output
 * or Direction::InOut (Input & Output) workspace
 *  @param validator :: The (optional) validator to use for this property
 *  @throw std::out_of_range if the direction argument is not a member of the
 * Direction enum (i.e. 0-2)
 */
template <typename TYPE>
WorkspaceProperty<TYPE>::WorkspaceProperty(const std::string &name, const std::string &wsName,
                                           const unsigned int direction, const Kernel::IValidator_sptr &validator)
    : Kernel::PropertyWithValue<std::shared_ptr<TYPE>>(name, std::shared_ptr<TYPE>(), validator, direction),
      m_workspaceName(wsName), m_initialWSName(wsName), m_optional(PropertyMode::Mandatory), m_locking(LockMode::Lock) {
}

/** Constructor.
 *  Sets the property and workspace names but initializes the workspace pointer
 * to null.
 *  @param name :: The name to assign to the property
 *  @param wsName :: The name of the workspace
 *  @param direction :: Whether this is a Direction::Input, Direction::Output
 * or Direction::InOut (Input & Output) workspace
 *  @param optional :: If true then the property is optional
 *  @param validator :: The (optional) validator to use for this property
 *  @throw std::out_of_range if the direction argument is not a member of the
 * Direction enum (i.e. 0-2)
 */
template <typename TYPE>
WorkspaceProperty<TYPE>::WorkspaceProperty(const std::string &name, const std::string &wsName,
                                           const unsigned int direction, const PropertyMode::Type optional,
                                           const Kernel::IValidator_sptr &validator)
    : Kernel::PropertyWithValue<std::shared_ptr<TYPE>>(name, std::shared_ptr<TYPE>(), validator, direction),
      m_workspaceName(wsName), m_initialWSName(wsName), m_optional(optional), m_locking(LockMode::Lock) {}

/** Constructor.
 *  Sets the property and workspace names but initializes the workspace pointer
 * to null.
 *  @param name :: The name to assign to the property
 *  @param wsName :: The name of the workspace
 *  @param direction :: Whether this is a Direction::Input, Direction::Output
 * or Direction::InOut (Input & Output) workspace
 *  @param optional :: A boolean indicating whether the property is mandatory
 * or not. Only matters
 *                     for input properties
 *  @param locking :: A boolean indicating whether the workspace should read or
 *                    write-locked when an algorithm begins. Default=true.
 *  @param validator :: The (optional) validator to use for this property
 *  @throw std::out_of_range if the direction argument is not a member of the
 * Direction enum (i.e. 0-2)
 */
template <typename TYPE>
WorkspaceProperty<TYPE>::WorkspaceProperty(const std::string &name, const std::string &wsName,
                                           const unsigned int direction, const PropertyMode::Type optional,
                                           const LockMode::Type locking, const Kernel::IValidator_sptr &validator)
    : Kernel::PropertyWithValue<std::shared_ptr<TYPE>>(name, std::shared_ptr<TYPE>(), validator, direction),
      m_workspaceName(wsName), m_initialWSName(wsName), m_optional(optional), m_locking(locking) {}

/// Copy constructor, the default name stored in the new object is the same as
/// the default name from the original object
template <typename TYPE>
WorkspaceProperty<TYPE>::WorkspaceProperty(const WorkspaceProperty &right)
    : Kernel::PropertyWithValue<std::shared_ptr<TYPE>>(right), m_workspaceName(right.m_workspaceName),
      m_initialWSName(right.m_initialWSName), m_optional(right.m_optional), m_locking(right.m_locking) {}

/// Copy assignment operator. Only copies the value (i.e. the pointer to the
/// workspace)
template <typename TYPE> WorkspaceProperty<TYPE> &WorkspaceProperty<TYPE>::operator=(const WorkspaceProperty &right) {
  if (&right == this)
    return *this;
  Kernel::PropertyWithValue<std::shared_ptr<TYPE>>::operator=(right);
  return *this;
}

/** Bring in the PropertyWithValue assignment operator explicitly (avoids
 * VSC++ warning)
 * @param value :: The value to set to
 * @return assigned PropertyWithValue
 */
template <typename TYPE>
WorkspaceProperty<TYPE> &WorkspaceProperty<TYPE>::operator=(const std::shared_ptr<TYPE> &value) {
  std::string wsName = value->getName();
  if (this->direction() == Kernel::Direction::Input && !wsName.empty()) {
    m_workspaceName = wsName;
  }
  Kernel::PropertyWithValue<std::shared_ptr<TYPE>>::operator=(value);
  return *this;
}

//--------------------------------------------------------------------------------------
/// Add the value of another property
template <typename TYPE> WorkspaceProperty<TYPE> &WorkspaceProperty<TYPE>::operator+=(Kernel::Property const *) {
  throw Kernel::Exception::NotImplementedError("+= operator is not implemented for WorkspaceProperty.");
}

/// 'Virtual copy constructor'
template <typename TYPE> WorkspaceProperty<TYPE> *WorkspaceProperty<TYPE>::clone() const {
  return new WorkspaceProperty<TYPE>(*this);
}

/** Get the name of the workspace
 *  @return The workspace's name
 */
template <typename TYPE> std::string WorkspaceProperty<TYPE>::value() const { return m_workspaceName; }

/**
 * @returns The name of the workspace encode as a Json::Value
 */
template <typename TYPE> Json::Value WorkspaceProperty<TYPE>::valueAsJson() const { return Json::Value(value()); }

/** Returns true if the workspace is in the ADS or there is none.
 * @return true if the string returned by value() is valid
 */
template <typename TYPE> bool WorkspaceProperty<TYPE>::isValueSerializable() const {
  return !m_workspaceName.empty() || !this->m_value;
}

/** Get the value the property was initialised with -its default value
 *  @return The default value
 */
template <typename TYPE> std::string WorkspaceProperty<TYPE>::getDefault() const { return m_initialWSName; }

/** Set the name of the workspace.
 * Also tries to retrieve it from the AnalysisDataService.
 * @param value :: The new name for the workspace
 * @return An empty string indicating success otherwise a string containing the
 * error
 */
template <typename TYPE> std::string WorkspaceProperty<TYPE>::setValue(const std::string &value) {
  m_workspaceName = value;
  if (Kernel::PropertyWithValue<std::shared_ptr<TYPE>>::autoTrim()) {
    boost::trim(m_workspaceName);
  }
  retrieveWorkspaceFromADS();
  return isValid();
}

/**
 * Set the name of the workspace from a Json::Value object
 * Also tries to retrieve it from the AnalysisDataService.
 * @param value :: The new name for the workspace
 * @return An empty string indicating success otherwise a string containing the
 * error
 */
template <typename TYPE> std::string WorkspaceProperty<TYPE>::setValueFromJson(const Json::Value &value) {
  try {
    return setValue(value.asString());
  } catch (std::exception &exc) {
    return exc.what();
  }
}

/** Set a value from a data item
 *  @param value :: A shared pointer to a DataItem. If it is of the correct
 *  type it will set validated, if not the property's value will be cleared.
 *  @return
 */
template <typename TYPE>
std::string WorkspaceProperty<TYPE>::setDataItem(const std::shared_ptr<Kernel::DataItem> &value) {
  std::shared_ptr<TYPE> typed = std::dynamic_pointer_cast<TYPE>(value);
  if (typed) {
    if (typed->getName().empty() && this->direction() == Kernel::Direction::Output) {
      typed->setPythonVariableName(m_workspaceName);
    }
    if (this->direction() == Kernel::Direction::Input) {
      m_workspaceName = typed->getName();
    }
    Kernel::PropertyWithValue<std::shared_ptr<TYPE>>::m_value = typed;
  } else {
    this->clear();
  }
  return isValid();
}

/** Set the property mode of the property e.g. Mandatory or Optional
 *  @param optional :: Property mode Mandatory or Optional
 */
template <typename TYPE> void WorkspaceProperty<TYPE>::setPropertyMode(const PropertyMode::Type &optional) {
  m_optional = optional;
}

/** Checks whether the entered workspace is valid.
 *  To be valid, in addition to satisfying the conditions of any validators,
 *  an output property must not have an empty name and an input one must point
 * to
 *  a workspace of the correct type.
 *  @returns A user level description of the problem or "" if it is valid.
 */
template <typename TYPE> std::string WorkspaceProperty<TYPE>::isValid() const {

  // If an output workspace it must have a name, although it might not exist
  // in the ADS yet
  if (this->direction() == Kernel::Direction::Output) {
    return isValidOutputWs();
  }

  // If an input (or inout) workspace, must point to something, although it
  // doesn't have to have a name
  // unless it's optional
  if (this->direction() == Kernel::Direction::Input || this->direction() == Kernel::Direction::InOut) {
    // Workspace groups will not have a value since they are not of type TYPE
    if (!Kernel::PropertyWithValue<std::shared_ptr<TYPE>>::m_value) {
      Mantid::API::Workspace_sptr wksp;
      // if the workspace name is empty then there is no point asking the ADS
      if (m_workspaceName.empty()) {
        return isOptionalWs();
      }

      try {
        wksp = AnalysisDataService::Instance().retrieve(m_workspaceName);
      } catch (Kernel::Exception::NotFoundError &) {
        return isOptionalWs();
      }

      // At this point we have a valid pointer to a Workspace so we need to
      // test whether it is a group
      if (std::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(wksp)) {
        return isValidGroup(std::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(wksp));
      }

      std::string error = "Workspace " + this->value() + " is not of the correct type";
      return error;
    }
  }
  // Call superclass method to access any attached validators (which do their
  // own logging)
  return Kernel::PropertyWithValue<std::shared_ptr<TYPE>>::isValid();
}

/** Indicates if the object is still pointing to the same workspace
 *  @return true if the value is the same as the initial value or false
 * otherwise
 */
template <typename TYPE> bool WorkspaceProperty<TYPE>::isDefault() const {
  if (m_initialWSName.empty()) {
    return m_workspaceName.empty() && !this->m_value;
  }
  return m_initialWSName == m_workspaceName;
}

/** Is the workspace property optional
 * @return true if the workspace can be blank   */
template <typename TYPE> bool WorkspaceProperty<TYPE>::isOptional() const {
  return (m_optional == PropertyMode::Optional);
}
/** Does the workspace need to be locked before starting an algorithm?
 * @return true (default) if the workspace will be locked */
template <typename TYPE> bool WorkspaceProperty<TYPE>::isLocking() const { return (m_locking == LockMode::Lock); }

/** Returns the current contents of the AnalysisDataService for input
 * workspaces.
 *  For output workspaces, an empty set is returned
 *  @return set of objects in AnalysisDataService
 */
template <typename TYPE> std::vector<std::string> WorkspaceProperty<TYPE>::allowedValues() const {
  if (this->direction() == Kernel::Direction::Input || this->direction() == Kernel::Direction::InOut) {
    // If an input workspace, get the list of workspaces currently in the ADS
    auto vals = AnalysisDataService::Instance().getObjectNames(Mantid::Kernel::DataServiceSort::Sorted);
    if (isOptional()) // Insert an empty option
    {
      vals.emplace_back("");
    }
    // Copy-construct a temporary workspace property to test the validity of
    // each workspace
    WorkspaceProperty<TYPE> tester(*this);

    // Remove any workspace that's not valid for this algorithm
    auto eraseIter = remove_if(vals.begin(), vals.end(),
                               [&tester](const std::string &wsName) { return !tester.setValue(wsName).empty(); });
    // Erase everything past returned iterator afterwards for readability
    vals.erase(eraseIter, vals.end());
    return vals;
  } else {
    // For output workspaces, just return an empty set
    return std::vector<std::string>();
  }
}

/// Create a history record
/// @return A populated PropertyHistory for this class
template <typename TYPE> const Kernel::PropertyHistory WorkspaceProperty<TYPE>::createHistory() const {
  std::string wsName = m_workspaceName;
  bool isdefault = this->isDefault();
  bool pythonVariable = false;

  if ((wsName.empty() || this->hasTemporaryValue()) && this->operator()()) {
    const auto pvName = Kernel::PropertyWithValue<std::shared_ptr<TYPE>>::m_value->getPythonVariableName();
    pythonVariable = !pvName.empty();
    if (pythonVariable) {
      wsName = pvName;
    } else {
      // give the property a temporary name in the history
      std::ostringstream os;
      os << "__TMP" << this->operator()().get();
      wsName = os.str();
    }
    isdefault = false;
  }
  return Kernel::PropertyHistory(this->name(), wsName, this->type(), isdefault, this->direction(), pythonVariable);
}

/** If this is an output workspace, store it into the AnalysisDataService
 *  @return True if the workspace is an output workspace and has been stored
 *  @throw std::runtime_error if unable to store the workspace successfully
 */
template <typename TYPE> bool WorkspaceProperty<TYPE>::store() {
  bool result = false;
  if (!this->operator()() && isOptional())
    return result;
  if (this->direction()) // Output or InOut
  {
    // Check that workspace exists
    if (this->operator()()) {
      // Note use of addOrReplace rather than add
      API::AnalysisDataService::Instance().addOrReplace(m_workspaceName, this->operator()());
    } else {
      throw std::runtime_error("WorkspaceProperty doesn't point to a workspace");
    }
    result = true;
  }
  // always clear the internal pointer after storing
  clear();

  return result;
}

template <typename TYPE> Workspace_sptr WorkspaceProperty<TYPE>::getWorkspace() const { return this->operator()(); }

/** Checks whether the entered workspace group is valid.
 *  To be valid *all* members of the group have to be valid.
 *  @param wsGroup :: the WorkspaceGroup of which to check the validity
 *  @returns A user level description of the problem or "" if it is valid.
 */
template <typename TYPE>
std::string WorkspaceProperty<TYPE>::isValidGroup(const std::shared_ptr<WorkspaceGroup> &wsGroup) const {
  g_log.debug() << " Input WorkspaceGroup found \n";

  std::vector<std::string> wsGroupNames = wsGroup->getNames();
  std::string error;

  // Cycle through each workspace in the group ...
  for (const auto &memberWsName : wsGroupNames) {
    std::shared_ptr<Workspace> memberWs = AnalysisDataService::Instance().retrieve(memberWsName);

    // Table Workspaces are ignored
    if ("TableWorkspace" == memberWs->id()) {
      error = "Workspace " + memberWsName +
              " is of type TableWorkspace and "
              "will therefore be ignored as "
              "part of the GroupedWorkspace.";

      g_log.debug() << error << '\n';
    } else {
      // ... and if it is a workspace of incorrect type, exclude the group by
      // returning an error.
      if (!std::dynamic_pointer_cast<TYPE>(memberWs)) {
        error = "Workspace " + memberWsName + " is not of type " +
                Kernel::PropertyWithValue<std::shared_ptr<TYPE>>::type() + ".";

        g_log.debug() << error << '\n';

        return error;
      }
      // If it is of the correct type, it may still be invalid. Check.
      else {
        Mantid::API::WorkspaceProperty<TYPE> memberWsProperty(*this);
        std::string memberError = memberWsProperty.setValue(memberWsName);
        if (!memberError.empty())
          return memberError; // Since if this member is invalid, then the
                              // whole group is invalid.
      }
    }
  }

  return ""; // Since all members of the group are valid.
}

/** Checks whether the entered output workspace is valid.
 *  To be valid the only thing it needs is a name that is allowed by the ADS,
 * @see AnalysisDataServiceImpl
 *  @returns A user level description of the problem or "" if it is valid.
 */
template <typename TYPE> std::string WorkspaceProperty<TYPE>::isValidOutputWs() const {
  std::string error;
  const std::string workspaceName = this->value();
  if (!workspaceName.empty()) {
    // Will the ADS accept it
    error = AnalysisDataService::Instance().isValid(workspaceName);
  } else {
    if (isOptional())
      error = ""; // Optional ones don't need a name
    else
      error = "Enter a name for the Output workspace";
  }
  return error;
}

/** Checks whether the entered workspace (that by this point we've found is
 * not in the ADS)
 *  is actually an optional workspace and so still valid.
 *  @returns A user level description of the problem or "" if it is valid.
 */
template <typename TYPE> std::string WorkspaceProperty<TYPE>::isOptionalWs() const {
  std::string error;

  if (m_workspaceName.empty()) {
    if (isOptional()) {
      error = "";
    } else {
      error = "Enter a name for the Input/InOut workspace";
    }
  } else {
    error = "Workspace \"" + this->value() + "\" was not found in the Analysis Data Service";
  }

  return error;
}

/// Reset the pointer to the workspace
template <typename TYPE> void WorkspaceProperty<TYPE>::clear() {
  Kernel::PropertyWithValue<std::shared_ptr<TYPE>>::m_value = std::shared_ptr<TYPE>();
}

/** Attempts to retreive the data from the ADS
 *  if the data is not foung the internal pointer is set to null.
 */
template <typename TYPE> void WorkspaceProperty<TYPE>::retrieveWorkspaceFromADS() {
  // Try and get the workspace from the ADS, but don't worry if we can't
  try {
    Kernel::PropertyWithValue<std::shared_ptr<TYPE>>::m_value =
        AnalysisDataService::Instance().retrieveWS<TYPE>(m_workspaceName);
  } catch (Kernel::Exception::NotFoundError &) {
    // Set to null property if not found
    this->clear();
  }
}

} // namespace API
} // namespace Mantid
