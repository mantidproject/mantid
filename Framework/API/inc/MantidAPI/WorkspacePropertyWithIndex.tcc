#include "MantidAPI/IndexTypeProperty.h"
#include "MantidAPI/WorkspacePropertyWithIndex.h"
#include <MantidAPI/MatrixWorkspace.h>
#include <MantidIndexing/GlobalSpectrumIndex.h>
#include <MantidIndexing/IndexInfo.h>
#include <MantidKernel/Property.h>
#include <MantidKernel/make_unique.h>

using namespace Mantid::Kernel;
using namespace Mantid::Indexing;

namespace Mantid {
namespace API {

template <typename TYPE>
WorkspacePropertyWithIndex<TYPE>::WorkspacePropertyWithIndex(
    const int indexType, const std::string &wsName, IValidator_sptr validator)
    : WorkspaceProperty<TYPE>("InputWorkspaceWithIndex", wsName,
                              Direction::Input),
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
    } catch (...) {
      error += "Invalid " + m_indexTypeProp->value() + "s provided";
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
operator=(const WorkspacePropertyWithIndex<TYPE> &rhs) {
  if (&rhs == this)
    return *this;
  API::WorkspaceProperty<TYPE>::operator=(rhs);

  // Copy Array Property
  this->m_indexListProp.reset(new ArrayProperty<int>(*rhs.m_indexListProp));
  this->m_indexTypeProp.reset(
      new IndexTypeProperty(rhs.m_indexTypeProp->allowedTypes()));
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
operator const std::pair<boost::shared_ptr<TYPE>, SpectrumIndexSet>() const {
  auto wksp = getWorkspace();
  SpectrumIndexSet indices = this->getIndices();
  return std::make_pair(boost::dynamic_pointer_cast<TYPE>(wksp), indices);
}

template <typename TYPE>
const SpectrumIndexSet WorkspacePropertyWithIndex<TYPE>::getIndices() const {
  // TODO handle DetectorID->SpectrumNumber conversion
  // This will need to be an extra IndexType

  auto wksp = WorkspaceProperty<TYPE>::getWorkspace();
  auto matWs = boost::dynamic_pointer_cast<MatrixWorkspace>(wksp);

  const auto &indexInfo = matWs->indexInfo();
  std::vector<int> list = this->m_indexListProp->operator()();

  //If no indices provided, then assume all
  if (list.size() == 0) 
	  return indexInfo.makeIndexSet();

  SpectrumIndexSet indexSet(list.size());

  switch (m_indexTypeProp->selectedType()) {
  case IndexType::SpectrumNumber:
    indexSet =
        indexInfo.makeIndexSet(std::vector<Mantid::Indexing::SpectrumNumber>(
            list.begin(), list.end()));
    break;
  case IndexType::WorkspaceIndex:
    indexSet = indexInfo.makeIndexSet(
        std::vector<Mantid::Indexing::GlobalSpectrumIndex>(list.begin(),
                                                           list.end()));
    break;
  }

  return indexSet;
}
} // namespace API
} // namespace Mantid
