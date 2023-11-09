// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "IndirectDataAnalysisTab.h"
#include "ParameterEstimation.h"

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
class MANTIDQT_INELASTIC_DLL IndirectDataAnalysisConvFitTab : public IndirectDataAnalysisTab {
  Q_OBJECT

public:
  IndirectDataAnalysisConvFitTab(QWidget *parent = nullptr);

  std::string getTabName() const override { return "ConvFit"; }

  bool hasResolution() const override { return true; }
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
