// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include <QString>

namespace MantidQt {
namespace API {

/** IGUILauncher : This is the interface for custom GUI launcher.
 */
class EXPORT_OPT_MANTIDQT_COMMON IGUILauncher {
public:
  virtual ~IGUILauncher() = default;
  virtual QString name() const = 0;
  virtual QString category() const = 0;
  virtual void show() const = 0;
};

} // namespace API
} // namespace MantidQt
