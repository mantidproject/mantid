// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IndirectDataAnalysisTab.h"

#include "MantidAPI/IFunction.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
class IDAFunctionParameterEstimation;

class MANTIDQT_INELASTIC_DLL IndirectDataAnalysisMSDFitTab : public IndirectDataAnalysisTab {
  Q_OBJECT

public:
  IndirectDataAnalysisMSDFitTab(QWidget *parent = nullptr);

  std::string getTabName() const override { return "MSDFit"; }

  bool hasResolution() const override { return false; }
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
