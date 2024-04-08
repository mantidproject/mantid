// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/IndirectDataValidationHelper.h"
#include "DllConfig.h"
#include <typeinfo>

using namespace Mantid::API;
namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INELASTIC_DLL MomentsTabModel {

public:
  MomentsTabModel();
  ~MomentsTabModel() = default;
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
