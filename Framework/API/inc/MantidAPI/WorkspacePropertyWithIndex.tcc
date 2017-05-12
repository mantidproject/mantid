#ifndef MANTIDAPI_WORKSPACEPROPERTYWITHINDEX_TCC
#define MANTIDAPI_WORKSPACEPROPERTYWITHINDEX_TCC

#include "MantidAPI/IndexTypeProperty.h"
#include "MantidAPI/WorkspacePropertyWithIndex.h"
#include <MantidAPI/MatrixWorkspace.h>
#include <MantidIndexing/IndexInfo.h>
#include <MantidKernel/Property.h>
#include <MantidKernel/make_unique.h>

#include "MantidAPI/WorkspaceProperty.tcc"

using namespace Mantid::Kernel;
using namespace Mantid::Indexing;

namespace Mantid {
namespace API {

template <typename TYPE>
WorkspacePropertyWithIndex<TYPE>::WorkspacePropertyWithIndex(
    const std::string &name, int indexType, const std::string &wsName,
    IValidator_sptr validator)
    : WorkspaceProperty<TYPE>(name, wsName, Direction::Input),
      m_indexListProp(
          make_unique<ArrayProperty<int>>("indices", Direction::Output)),
      m_indexTypeProp(make_unique<IndexTypeProperty>(indexType)) {
  checkWorkspace();
}

template <typename TYPE>
WorkspacePropertyWithIndex<TYPE>::WorkspacePropertyWithIndex(
    const WorkspacePropertyWithIndex<TYPE> &right)
    : WorkspaceProperty<TYPE>(right),
      m_indexListProp(make_unique<ArrayProperty<int>>(*right.m_indexListProp)),
      m_indexTypeProp(make_unique<IndexTypeProperty>(*right.m_indexTypeProp)) {
  checkWorkspace();
}

template <typename TYPE>
std::string WorkspacePropertyWithIndex<TYPE>::isValid() const {
  std::string error = WorkspaceProperty<TYPE>::isValid() + " " +
                      m_indexListProp->isValid() + " " +
                      m_indexTypeProp->isValid() + " ";

  if (std::all_of(error.cbegin(), error.cend(),
                  [](auto c) { return c == ' '; })) {
    error = "";

    try {
      getIndices();
    } catch (std::out_of_range &) {
      error += m_indexTypeProp->value() + "s provided out of range.";
    } catch (std::logic_error &) {
      error += "Invalid " + m_indexTypeProp->value() +
               "s is invalid. May contain duplicate indices.";
    }
  }

  return error;
}

template <typename TYPE>
bool WorkspacePropertyWithIndex<TYPE>::
operator==(const WorkspacePropertyWithIndex<TYPE> &rhs) {
  return WorkspaceProperty<TYPE>::operator==(rhs) &&
         m_indexListProp == rhs.m_indexListProp &&
         m_indexTypeProp == rhs.m_indexTypeProp;
}

template <typename TYPE>
WorkspacePropertyWithIndex<TYPE> &WorkspacePropertyWithIndex<TYPE>::
operator=(const std::tuple<boost::shared_ptr<TYPE>, API::IndexType,
                           std::vector<int>> &rhs) {
  boost::shared_ptr<TYPE> ws;
  API::IndexType type;
  std::vector<int> list;

  std::tie(ws, type, list) = rhs;

  *this = ws;
  *this->m_indexTypeProp.get() = type;
  *this->m_indexListProp.get() = list;

  return *this;
}

template <typename TYPE>
WorkspacePropertyWithIndex<TYPE> &WorkspacePropertyWithIndex<TYPE>::
operator=(const std::tuple<boost::shared_ptr<TYPE>, API::IndexType, std::string>
              &rhs) {
  boost::shared_ptr<TYPE> ws;
  API::IndexType type;
  std::string list;

  std::tie(ws, type, list) = rhs;

  *this = ws;
  *this->m_indexTypeProp.get() = type;
  m_indexListProp->setValue(list);

  return *this;
}

template <typename TYPE>
WorkspacePropertyWithIndex<TYPE> &WorkspacePropertyWithIndex<TYPE>::
operator=(const WorkspacePropertyWithIndex<TYPE> &rhs) {
  if (&rhs == this)
    return *this;
  API::WorkspaceProperty<TYPE>::operator=(rhs);
  *m_indexListProp = *rhs.m_indexListProp;
  *m_indexTypeProp = rhs.m_indexTypeProp->selectedType();

  return *this;
}

template <typename TYPE>
WorkspacePropertyWithIndex<TYPE> &WorkspacePropertyWithIndex<TYPE>::
operator+=(WorkspacePropertyWithIndex const *rhs) {
  throw Kernel::Exception::NotImplementedError(
      "+= operator is not implemented for WorkspacePropertyWithIndex.");
  return *this;
}

template <typename TYPE>
WorkspacePropertyWithIndex<TYPE> *
WorkspacePropertyWithIndex<TYPE>::clone() const {
  return new WorkspacePropertyWithIndex<TYPE>(*this);
}

template <typename TYPE>
WorkspacePropertyWithIndex<TYPE>::
operator const std::tuple<boost::shared_ptr<TYPE>, SpectrumIndexSet>() const {
  return std::make_pair(boost::dynamic_pointer_cast<TYPE>(getWorkspace()),
                        getIndices());
}

template <typename TYPE>
const SpectrumIndexSet WorkspacePropertyWithIndex<TYPE>::getIndices() const {
  // TODO handle DetectorID->SpectrumNumber conversion
  // This will need to be an extra IndexType
  const auto &list = this->m_indexListProp->operator()();

  const auto &indexInfo =
      boost::dynamic_pointer_cast<API::MatrixWorkspace>(getWorkspace())
          ->indexInfo();

  // If no indices provided, then assume all
  if (list.size() == 0)
    return indexInfo.makeIndexSet();

  switch (m_indexTypeProp->selectedType()) {
  case IndexType::SpectrumNumber:
    return indexInfo.makeIndexSet(std::vector<Mantid::Indexing::SpectrumNumber>(
        list.begin(), list.end()));
    break;
  case IndexType::WorkspaceIndex:
    return indexInfo.makeIndexSet(
        std::vector<Mantid::Indexing::GlobalSpectrumIndex>(list.begin(),
                                                           list.end()));
    break;
  }

  return Mantid::Indexing::SpectrumIndexSet(0);
}
} // namespace API
} // namespace Mantid

#endif // MANTIDAPI_WORKSPACEPROPERTYWITHINDEX_TCC