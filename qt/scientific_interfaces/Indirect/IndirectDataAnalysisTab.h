// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IndirectDataAnalysis.h"
#include "IndirectTab.h"

#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <memory>

class QSettings;
class QString;

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
class MANTIDQT_INDIRECT_DLL IndirectDataAnalysisTab : public IndirectTab {
  Q_OBJECT

public:
  /// Constructor
  IndirectDataAnalysisTab(QWidget *parent = nullptr);
  virtual ~IndirectDataAnalysisTab() override = default;

  /// Prevent loading of data with incorrect naming
  void filterInputData(bool filter);

protected:
  /// Retrieve the selected spectrum
  int getSelectedSpectrum() const;

private:
  virtual void setFileExtensionsByName(bool filter) = 0;

  int m_selectedSpectrum;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
