// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/QtTreePropertyBrowser"
#include "MantidQtWidgets/Spectroscopy/DataValidationHelper.h"
#include <typeinfo>

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_INELASTIC_DLL IIqtModel {

public:
  virtual ~IIqtModel() = default;
  virtual void setupTransformToIqt(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner,
                                   std::string const &outputWorkspace) = 0;
  virtual void setSampleWorkspace(std::string const &sampleWorkspace) = 0;
  virtual void setResWorkspace(std::string const &resWorkspace) = 0;
  virtual void setNIterations(std::string const &nIterations) = 0;
  virtual void setEnergyMin(double energyMin) = 0;
  virtual void setEnergyMax(double energyMax) = 0;
  virtual void setNumBins(double numBins) = 0;
  virtual void setCalculateErrors(bool calculateErrors) = 0;
  virtual void setEnforceNormalization(bool enforceNormalization) = 0;
  virtual double EMin() const = 0;
  virtual double EMax() const = 0;
};

class MANTIDQT_INELASTIC_DLL IqtModel : public IIqtModel {

public:
  IqtModel();
  ~IqtModel() = default;
  void setupTransformToIqt(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner,
                           std::string const &outputWorkspace) override;
  void setSampleWorkspace(std::string const &sampleWorkspace) override;
  void setResWorkspace(std::string const &resWorkspace) override;
  void setNIterations(std::string const &nIterations) override;
  void setEnergyMin(double energyMin) override;
  void setEnergyMax(double energyMax) override;
  void setNumBins(double numBins) override;
  void setCalculateErrors(bool calculateErrors) override;
  void setEnforceNormalization(bool enforceNormalization) override;
  double EMin() const override;
  double EMax() const override;

private:
  std::string m_sampleWorkspace;
  std::string m_resWorkspace;
  std::string m_nIterations;
  double m_energyMin;
  double m_energyMax;
  double m_numBins;
  bool m_calculateErrors;
  bool m_enforceNormalization;
};
} // namespace CustomInterfaces
} // namespace MantidQt