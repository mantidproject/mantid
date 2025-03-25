// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IndexProperty.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidIndexing/SpectrumIndexSet.h"

#include <memory>
#include <type_traits>

namespace {
template <typename T1, typename T2>
void setWorkspaceProperty(Mantid::API::WorkspaceProperty<T1> *wsProp, const T2 &wksp, const std::true_type &) {
  *wsProp = wksp;
}

template <typename T1, typename T2>
void setWorkspaceProperty(Mantid::API::WorkspaceProperty<T1> *wsProp, const T2 &wksp, const std::false_type &) {
  wsProp->setValue(wksp);
}
} // namespace

namespace Mantid {
namespace API {
/** Declare a property which defines the workspace and allowed index types, as
 * well as a property for capturing the indices all at once. This method is
 * only enabled if T is convertible to MatrixWorkspace.
 * The AllowedIndexTypes combination of allowed index types. Default IndexType::WorkspaceIndex
 *
 * @param propertyName Name of property which will be reserved
 * @param wsPropArgs a parameter pack of arguments forwarded to WorkspaceProperty.
 * Can contain PropertyMode, LockMode, and validators.
 * @param doc Property documentation string.
 */
template <typename T, const int AllowedIndexTypes, typename... WSPropArgs, typename>
void Algorithm::declareWorkspaceInputProperties(const std::string &propertyName, const std::string &doc,
                                                WSPropArgs &&...wsPropArgs) {
  auto wsProp = std::make_unique<WorkspaceProperty<T>>(propertyName, "", Kernel::Direction::Input,
                                                       std::forward<WSPropArgs>(wsPropArgs)...);
  const auto &wsPropRef = *wsProp;
  declareProperty(std::move(wsProp), doc);

  auto indexTypePropName = IndexTypeProperty::generatePropertyName(propertyName);
  auto indexTypeProp = std::make_unique<IndexTypeProperty>(indexTypePropName, AllowedIndexTypes);
  const auto &indexTypePropRef = *indexTypeProp;

  declareProperty(std::move(indexTypeProp), "The type of indices in the optional index set; For optimal "
                                            "performance WorkspaceIndex should be preferred;");

  auto indexPropName = IndexProperty::generatePropertyName(propertyName);
  declareProperty(std::make_unique<IndexProperty>(indexPropName, wsPropRef, indexTypePropRef),
                  "An optional set of spectra that will be processed by the "
                  "algorithm; If not set, all spectra will be processed; The "
                  "indices in this list can be workspace indices or possibly "
                  "spectrum numbers, depending on the selection made for the "
                  "index type; Indices are entered as a comma-separated list "
                  "of values, and/or ranges; For example, '4,6,10-20,1000';");

  m_reservedList.emplace_back(propertyName);
  m_reservedList.emplace_back(indexTypePropName);
  m_reservedList.emplace_back(indexPropName);
}

template <typename T1, typename T2, typename WsType>
void Algorithm::doSetInputProperties(const std::string &name, const T1 &wksp, IndexType type, const T2 &list) {
  if (!isCompoundProperty(name))
    throw std::runtime_error("Algorithm::setWorkspaceInputProperties can only be used "
                             "with properties declared using "
                             "declareWorkspaceInputProperties.");

  auto *wsProp = dynamic_cast<WorkspaceProperty<WsType> *>(getPointerToProperty(name));
  auto *indexTypeProp =
      dynamic_cast<IndexTypeProperty *>(getPointerToProperty(IndexTypeProperty::generatePropertyName(name)));
  auto *indexProp = dynamic_cast<IndexProperty *>(getPointerToProperty(IndexProperty::generatePropertyName(name)));

  setWorkspaceProperty<WsType, T1>(wsProp, wksp, std::is_convertible<T1, std::shared_ptr<WsType>>());

  *indexTypeProp = type;

  *indexProp = list;
}

/** Mechanism for setting the index property with a workspace shared pointer.
* This method can only be used if T1 is convertible to a MatrixWorkspace and
* T2 is either std::string or std::vector<int>

@param name Property name
@param wksp Workspace as a pointer
@param type Index type IndexType::WorkspaceIndex or IndexType::SpectrumNum
@param list List of indices to be used.
*/
template <typename T1, typename T2, typename, typename>
void Algorithm::setWorkspaceInputProperties(const std::string &name, const std::shared_ptr<T1> &wksp, IndexType type,
                                            const T2 &list) {
  doSetInputProperties<std::shared_ptr<T1>, T2, T1>(name, wksp, type, list);
}

/** Mechanism for setting the index property with a workspace shared pointer.
* This method can only be used if T1 is convertible to a MatrixWorkspace and
* T2 is either std::string or std::vector<int64_t>

@param name Property name
@param wsName Workspace name as string
@param type Index type IndexType::WorkspaceIndex or IndexType::SpectrumNum
@param list List of indices to be used.
*/
template <typename T1, typename T2, typename, typename>
void Algorithm::setWorkspaceInputProperties(const std::string &name, const std::string &wsName, IndexType type,
                                            const T2 &list) {
  doSetInputProperties<std::string, T2, T1>(name, wsName, type, list);
}

/** Mechanism for retriving the index property. This method can only be used
if T is convertible to a MatrixWorkspace.

@param name Property name
@returns Tuple containing Workspace shared pointer and SpectrumIndexSet
*/
template <typename T, typename>
std::tuple<std::shared_ptr<T>, Indexing::SpectrumIndexSet>
Algorithm::getWorkspaceAndIndices(const std::string &name) const {
  if (!isCompoundProperty(name))
    throw std::runtime_error("Algorithm::getWorkspacesAndIndices can only be used "
                             "with properties declared using "
                             "declareWorkspaceInputProperties.");

  std::shared_ptr<T> ws = getProperty(name);

  // Not able to use the regular getProperty mechanism because types, in this
  // case SpectrumIndexSet, need to be known upfront. Since SpectrumIndexSet is
  // not declared at a level where it can be used in Kernel, this will not work.
  // There is an issue which has been created which may solve this and other
  // problems related to the property mechanism in the future:
  // https://github.com/mantidproject/mantid/issues/20092

  auto indexProp = dynamic_cast<IndexProperty *>(getPointerToProperty(IndexProperty::generatePropertyName(name)));
  Indexing::SpectrumIndexSet indexSet = *indexProp;

  return std::make_tuple(ws, indexSet);
}
} // namespace API
} // namespace Mantid
