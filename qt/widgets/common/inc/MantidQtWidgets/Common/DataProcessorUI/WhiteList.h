// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/DataProcessorUI/Column.h"
#include "MantidQtWidgets/Common/DataProcessorUI/ConstColumnIterator.h"
#include "MantidQtWidgets/Common/DllOption.h"

#include <vector>

#include <QString>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

/** @class WhiteList

A whitelist is a ordered collection of algorithm properties,
the values of which can be set from the DataProcessorWidget's
processing table.

Each entry in the whitelist also contains meta-data such as a description
and visability status which are used when displaying the processing table.
*/

class EXPORT_OPT_MANTIDQT_COMMON WhiteList {
public:
  using const_iterator = ConstColumnIterator;

  void addElement(const QString &colName, const QString &algProperty, const QString &description,
                  bool showValue = false, const QString &prefix = "", bool isKey = false);
  int indexFromName(const QString &colName) const;
  QString name(int index) const;
  QString algorithmProperty(int index) const;
  QString description(int index) const;
  QString prefix(int index) const;
  bool isShown(int index) const;
  bool isKey(int index) const;
  std::size_t size() const;
  const_iterator cbegin() const;
  const_iterator begin() const;
  const_iterator cend() const;
  const_iterator end() const;
  std::vector<QString> const &names() const;
  bool hasKeyColumns() const;

private:
  std::vector<QString> m_names;
  std::vector<QString> m_algorithmProperties;
  std::vector<bool> m_isShown;
  std::vector<QString> m_prefixes;
  std::vector<QString> m_descriptions;
  std::vector<bool> m_isKey;
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt