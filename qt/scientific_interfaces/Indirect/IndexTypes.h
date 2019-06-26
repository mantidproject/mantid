// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_INDEXTYPE_H_
#define MANTIDQTCUSTOMINTERFACESIDA_INDEXTYPE_H_

#include <QMetaType>
#include <ostream>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

template<int Class>
struct IndexType {
  using IntImplementationType = int;
  IntImplementationType value = 0;
  IndexType operator+(IndexType index) const { return IndexType{ value + index.value }; }
  IndexType operator-(IndexType index) const { return IndexType{ value - index.value }; }
  bool operator>(IndexType index) const { return value > index.value; }
  bool operator<(IndexType index) const { return value < index.value; }
  bool operator<=(IndexType index) const { return value <= index.value; }
  bool operator==(IndexType index) const { return value == index.value; }
  bool operator!=(IndexType index) const { return value != index.value; }
  IndexType& operator++() { ++value; return *this; }
  IndexType operator++(int) { auto oldValue = value++; return IndexType{ oldValue }; }
  IndexType& operator+=(const IndexType &index) { value += index.value; return *this; }
  IndexType operator-() const { return IndexType{-value}; }
};

using SpectrumRowIndex = IndexType<0>;
using WorkspaceIndex = IndexType<1>;
using GroupIndex = IndexType<2>;
using DatasetIndex = IndexType<3>;

template<class CollectionIndexType, class CollectionValueType>
class IndexCollectionType {
public:
  using CollectionImplementationType = std::vector<CollectionValueType>;
  const CollectionValueType& operator[](const CollectionIndexType & dataIndex) const { return m_collection[dataIndex.value]; }
  CollectionValueType& operator[](const CollectionIndexType & dataIndex) { return m_collection[dataIndex.value]; }
  CollectionIndexType size() const { return CollectionIndexType{static_cast<CollectionIndexType::IntImplementationType>(m_collection.size())}; }
  CollectionIndexType zero() const { return CollectionIndexType{ 0 }; }
  CollectionIndexType last() const { return CollectionIndexType{static_cast<CollectionIndexType::IntImplementationType>(m_collection.size() - 1)}; }
  bool empty() const { return m_collection.empty(); }
  CollectionValueType& front() { return m_collection.front(); }
  CollectionValueType& back() { return m_collection.back(); }
  const CollectionValueType& front() const { return m_collection.front(); }
  const CollectionValueType& back() const { return m_collection.back(); }
  template< class... Args >
  void emplace_back(Args&&... args) { m_collection.emplace_back(args...); }
  void remove(const CollectionIndexType & dataIndex) { m_collection.erase(m_collection.begin() + dataIndex.value); }
  typename CollectionImplementationType::iterator begin() { return m_collection.begin(); }
  typename CollectionImplementationType::iterator end() { return m_collection.end(); }
  typename CollectionImplementationType::const_iterator begin() const { return m_collection.begin(); }
  typename CollectionImplementationType::const_iterator end() const { return m_collection.end(); }
  void clear() {m_collection.clear();}
private:
  CollectionImplementationType m_collection;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

template<int i>
std::ostream& operator<<(std::ostream& out, const MantidQt::CustomInterfaces::IDA::IndexType<i>& index) {
  out << index.value;
  return out;
}

Q_DECLARE_METATYPE(MantidQt::CustomInterfaces::IDA::SpectrumRowIndex);
Q_DECLARE_METATYPE(MantidQt::CustomInterfaces::IDA::WorkspaceIndex);
Q_DECLARE_METATYPE(MantidQt::CustomInterfaces::IDA::GroupIndex);

#endif /* MANTIDQTCUSTOMINTERFACESIDA_INDEXTYPE_H_ */
