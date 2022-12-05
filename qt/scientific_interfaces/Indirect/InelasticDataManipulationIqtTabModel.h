// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "IndirectDataValidationHelper.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/QtTreePropertyBrowser"
#include <typeinfo>

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INDIRECT_DLL InelasticDataManipulationIqtTabModel {

public:
  InelasticDataManipulationIqtTabModel();
  ~InelasticDataManipulationIqtTabModel() = default;
  void setupTransformToIqt(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner, std::string const &outputWorkspace);
  void setSampleWorkspace(std::string const &sampleWorkspace);
  void setResWorkspace(std::string const &resWorkspace);
  void setNIterations(std::string const &nIterations);
  void setEnergyMin(double energyMin);
  void setEnergyMax(double energyMax);
  void setNumBins(double numBins);
  void setCalculateErrors(bool calculateErrors);

private:
  std::string m_sampleWorkspace;
  std::string m_resWorkspace;
  std::string m_nIterations;
  double m_energyMin;
  double m_energyMax;
  double m_numBins;
  bool m_calculateErrors;
};
} // namespace CustomInterfaces
} // namespace MantidQt