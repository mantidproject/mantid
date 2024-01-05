// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"

#include <string>

#include <QStringList>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INELASTIC_DLL IAddWorkspaceDialog {
public:
  virtual std::string workspaceName() const = 0;
  virtual void setWSSuffices(const QStringList &suffices) = 0;
  virtual void setFBSuffices(const QStringList &suffices) = 0;

  virtual void updateSelectedSpectra() = 0;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
