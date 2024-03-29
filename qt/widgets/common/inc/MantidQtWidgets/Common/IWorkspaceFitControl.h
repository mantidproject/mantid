// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include <QString>

namespace MantidQt {
namespace MantidWidgets {

/** IWorkspaceFitControl: set data to fit for a fit property browser

  Abstract base class to be implemented
*/
class EXPORT_OPT_MANTIDQT_COMMON IWorkspaceFitControl {
public:
  virtual ~IWorkspaceFitControl() = default;
  virtual void setWorkspaceName(const QString &wsName) = 0;
  virtual void setStartX(double start) = 0;
  virtual void setEndX(double end) = 0;
  virtual void setWorkspaceIndex(int i) = 0;
  virtual void allowSequentialFits(bool allow) = 0;
  virtual bool rawData() const = 0;
};

} // namespace MantidWidgets
} // namespace MantidQt
