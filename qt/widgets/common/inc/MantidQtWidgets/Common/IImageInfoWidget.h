// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include <QTableWidget>

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_COMMON IImageInfoWidget : public QTableWidget {
  Q_OBJECT
public:
  IImageInfoWidget(QWidget *parent = nullptr) : QTableWidget(0, 0, parent) {}

  virtual void updateTable(const double x, const double y, const double z,
                           bool includeValues = true) = 0;
};

} // namespace MantidWidgets
} // namespace MantidQt
