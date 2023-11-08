// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IndirectDataAnalysisTab.h"

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <memory>

namespace Mantid {
namespace API {
class IFunction;
class CompositeFunction;
} // namespace API
} // namespace Mantid

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INELASTIC_DLL IndirectDataAnalysisIqtFitTab : public IndirectDataAnalysisTab {
  Q_OBJECT

public:
  IndirectDataAnalysisIqtFitTab(QWidget *parent = nullptr);

  std::string getTabName() const override { return "IqtFit"; }

  bool hasResolution() const override { return false; }

private:
  EstimationDataSelector getEstimationDataSelector() const override;

  QString m_tiedParameter;
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
