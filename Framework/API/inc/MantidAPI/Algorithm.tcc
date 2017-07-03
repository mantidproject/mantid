#include "MantidAPI/Algorithm.h"
// ArrayProperty Implementation
#include "MantidAPI/IndexProperty.h"
#include "MantidAPI/WorkspaceProperty.h"
#include <MantidKernel/ArrayProperty.tcc>

namespace Mantid {
namespace API {
/** Declare a property which defines the workspace and allowed index types, as
* well as a property for capturing the indices all at once. This method is
* only enabled if T is convertible to MatrixWorkspace.
@param propertyName Name of property which will be reserved
@param allowedIndexTypes combination of allowed index types. Default
IndexType::WorkspaceIndex
@param optional Determines if workspace property is optional. Default
PropertyMode::Type::Mandatory
@param lock Determines whether or not the workspace is locked. Default
LockMode::Type::Lock
@param doc Property documentation string.
*/
template <typename T, typename>
void Algorithm::declareIndexProperty(const std::string &propertyName,
                                     const int allowedIndexTypes,
                                     PropertyMode::Type optional,
                                     LockMode::Type lock,
                                     const std::string &doc) {
  declareProperty(
      Kernel::make_unique<WorkspaceProperty<T>>(
          propertyName, "", Kernel::Direction::Input, optional, lock),
      doc);

  declareProperty(Kernel::make_unique<IndexTypeProperty>(
      propertyName + "IndexType", allowedIndexTypes));

  auto *wsProp =
      dynamic_cast<WorkspaceProperty<T> *>(getPointerToProperty(propertyName));
  auto *indexTypeProp = dynamic_cast<IndexTypeProperty *>(
      getPointerToProperty(propertyName + "IndexType"));

  declareProperty(Kernel::make_unique<IndexProperty>(propertyName + "IndexSet",
                                                     *wsProp, *indexTypeProp));

  m_reservedList.push_back(propertyName);
  m_reservedList.push_back(propertyName + "IndexType");
  m_reservedList.push_back(propertyName + "IndexSet");
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
void Algorithm::setIndexProperty(const std::string &name,
                                 const boost::shared_ptr<T1> &wksp,
                                 IndexType type, const T2 &list) {
  if (!isCompoundProperty(name))
    throw std::runtime_error("Algorithm::setIndexProperty can only be used "
                             "with properties declared using "
                             "declareIndexProperty.");
  auto *wsProp =
      dynamic_cast<WorkspaceProperty<T1> *>(getPointerToProperty(name));
  auto *indexTypeProp = dynamic_cast<IndexTypeProperty *>(
      getPointerToProperty(name + "IndexType"));
  auto *indexProp =
      dynamic_cast<IndexProperty *>(getPointerToProperty(name + "IndexSet"));

  *wsProp = wksp;

  *indexTypeProp = type;

  *indexProp = list;
}

/** Mechanism for setting the index property with a workspace shared pointer.
* This method can only be used if T1 is convertible to a MatrixWorkspace and
* T2 is either std::string or std::vector<int>

@param name Property name
@param wsName Workspace as a pointer
@param type Index type IndexType::WorkspaceIndex or IndexType::SpectrumNum
@param list List of indices to be used.
*/
template <typename T1, typename T2, typename, typename>
void Algorithm::setIndexProperty(const std::string &name,
                                 const std::string &wsName, IndexType type,
                                 const T2 &list) {
  if (!isCompoundProperty(name))
    throw std::runtime_error("Algorithm::setIndexProperty can only be used "
                             "with properties declared using "
                             "declareIndexProperty.");
  auto *wsProp =
      dynamic_cast<WorkspaceProperty<T1> *>(getPointerToProperty(name));
  auto *indexTypeProp = dynamic_cast<IndexTypeProperty *>(
      getPointerToProperty(name + "IndexType"));
  auto *indexProp =
      dynamic_cast<IndexProperty *>(getPointerToProperty(name + "IndexSet"));

  wsProp->setValue(wsName);

  *indexTypeProp = type;

  *indexProp = list;
}

/** Mechanism for retriving the index property. This method can only be used
if T is convertible to a MatrixWorkspace.

@param name Property name
@returns Tuple containing Workspace shared pointer and SpectrumIndexSet
*/
template <typename T, typename>
std::tuple<boost::shared_ptr<T>, Indexing::SpectrumIndexSet>
Algorithm::getIndexProperty(const std::string &name) const {
  if (!isCompoundProperty(name))
    throw std::runtime_error("Algorithm::getIndexProperty can only be used "
                             "with properties declared using "
                             "declareIndexProperty.");

  auto *wsProp =
      dynamic_cast<WorkspaceProperty<T> *>(getPointerToProperty(name));
  auto *indexProp =
      dynamic_cast<IndexProperty *>(getPointerToProperty(name + "IndexSet"));

  return std::make_tuple(boost::dynamic_pointer_cast<T>(wsProp->getWorkspace()),
                         indexProp->getIndices());
}
} // namespace API
} // namespace Mantid