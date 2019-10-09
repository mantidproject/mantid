// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MANTIDWIDGETS_IOPTIONSDIALOG_H
#define MANTID_MANTIDWIDGETS_IOPTIONSDIALOG_H

#include "DllOption.h"

namespace MantidQt {
namespace MantidWidgets {
class EXPORT_OPT_MANTIDQT_COMMON IOptionsDialog {
public:
  virtual ~IOptionsDialog() = default;
  virtual void show() = 0;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTID_MANTIDWIDGETS_IOPTIONSDIALOG_H */
