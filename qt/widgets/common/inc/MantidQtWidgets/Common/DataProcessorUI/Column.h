// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidQtWidgets/Common/DllOption.h"
#include <QString>
namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
/** @class Column

A column represents a whitelist element providing easy access to it's name,
algorithm, visibility status, prefix and description.
*/

class EXPORT_OPT_MANTIDQT_COMMON Column {
public:
  Column(QString const &name, QString const &algorithmProperty, bool isShown, QString const &prefix,
         QString const &description, bool isKey);
  QString const &name() const;
  QString const &algorithmProperty() const;
  bool isShown() const;
  bool isKey() const;
  QString const &prefix() const;
  QString const &description() const;

private:
  QString const &m_name;
  QString const &m_algorithmProperty;
  bool m_isShown;
  QString const &m_prefix;
  QString const &m_description;
  bool m_isKey;
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt