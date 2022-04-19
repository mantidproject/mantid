// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "IndirectDataValidationHelper.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/QtTreePropertyBrowser"
#include <typeinfo>

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INDIRECT_DLL IndirectMomentsModel {

public:
  IndirectMomentsModel();
  ~IndirectMomentsModel() = default;
  IAlgorithm_sptr setupAlgorithm();
  void setInputWorkspace(const std::string &workspace);
  void setEMin(double eMin);
  void setEMax(double eMax);
  void setScale(bool scale);
  void setScaleValue(double scaleValue);
  std::string getOutputWorkspace();

private:
  std::string m_inputWorkspace;
  std::string m_outputWorkspaceName;
  double m_eMin;
  double m_eMax;
  double m_scaleValue;
  bool m_scale;
};
} // namespace CustomInterfaces
} // namespace MantidQt
