#include "MantidQtWidgets/Common/DataProcessorUI/ConstColumnIterator.h"
namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
ConstColumnIterator::ConstColumnIterator(QStringIterator names,
                                         QStringIterator descriptions,
                                         QStringIterator algorithmProperties,
                                         BoolIterator isShown,
                                         QStringIterator prefixes)
    : m_names(names), m_descriptions(descriptions),
      m_algorithmProperties(algorithmProperties), m_isShown(isShown),
      m_prefixes(prefixes) {}

ConstColumnIterator &ConstColumnIterator::operator++() {
  ++m_names;
  ++m_descriptions;
  ++m_algorithmProperties;
  ++m_isShown;
  ++m_prefixes;
  return (*this);
}

ConstColumnIterator ConstColumnIterator::operator++(int) {
  auto result = (*this);
  ++result;
  return result;
}

bool ConstColumnIterator::operator==(const ConstColumnIterator &other) const {
  return m_names == other.m_names;
}

bool ConstColumnIterator::operator!=(const ConstColumnIterator &other) const {
  return !((*this) == other);
}

auto ConstColumnIterator::operator*() const -> reference {
  return reference(*m_names, *m_algorithmProperties, *m_isShown, *m_prefixes,
                   *m_descriptions);
}

ConstColumnIterator &ConstColumnIterator::operator+=(difference_type n) {
  m_names += n;
  m_algorithmProperties += n;
  m_isShown += n;
  m_prefixes += n;
  m_descriptions += n;
  return (*this);
}

ConstColumnIterator &ConstColumnIterator::operator-=(difference_type n) {
  m_names -= n;
  m_algorithmProperties -= n;
  m_isShown -= n;
  m_prefixes -= n;
  m_descriptions -= n;
  return (*this);
}
}
}
}
