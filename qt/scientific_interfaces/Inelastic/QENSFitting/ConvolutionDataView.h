// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "FitDataView.h"

#include "DllConfig.h"

#include <QTabWidget>

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {

/**
Presenter for a table of convolution fitting data.
*/
class MANTIDQT_INELASTIC_DLL ConvolutionDataView : public FitDataView {
  Q_OBJECT
public:
  ConvolutionDataView(QWidget *parent);
  void addTableEntry(size_t row, FitDataRow const &newRow) override;

protected:
  ConvolutionDataView(const QStringList &headers, QWidget *parent);

protected slots:
  void showAddWorkspaceDialog() override;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
