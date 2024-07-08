// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DataValidationHelper.h"
#include "DllConfig.h"
#include "MantidGeometry/IComponent.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/QtTreePropertyBrowser"
#include <typeinfo>

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INELASTIC_DLL SqwModel {

public:
  SqwModel();
  ~SqwModel() = default;
  void setupRebinAlgorithm(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner);
  void setupSofQWAlgorithm(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner);
  void setupAddSampleLogAlgorithm(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner);
  void setInputWorkspace(const std::string &workspace);
  void setQMin(double qMin);
  void setQWidth(double qWidth);
  void setQMax(double qMax);
  void setEMin(double eMin);
  void setEWidth(double eWidth);
  void setEMax(double eMax);
  void setEFixed(const double eFixed);
  void setRebinInEnergy(bool scale);
  std::string getEFixedFromInstrument(std::string const &instrumentName, std::string analyser,
                                      std::string const &reflection);
  std::string getOutputWorkspace();
  MatrixWorkspace_sptr getRqwWorkspace();
  UserInputValidator validate(std::tuple<double, double> const qRange, std::tuple<double, double> const eRange);
  MatrixWorkspace_sptr loadInstrumentWorkspace(const std::string &instrumentName, const std::string &analyser,
                                               const std::string &reflection);

private:
  std::string m_inputWorkspace;
  std::string m_baseName;
  double m_eFixed;
  double m_qLow;
  double m_qWidth;
  double m_qHigh;
  double m_eLow;
  double m_eWidth;
  double m_eHigh;
  bool m_rebinInEnergy;
};
} // namespace CustomInterfaces
} // namespace MantidQt