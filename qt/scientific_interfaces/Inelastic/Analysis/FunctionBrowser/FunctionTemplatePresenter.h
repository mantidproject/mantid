// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "ITemplatePresenter.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class FunctionTemplateBrowser;

class MANTIDQT_INELASTIC_DLL FunctionTemplatePresenter : public ITemplatePresenter {
public:
  FunctionTemplatePresenter();

  virtual void init() override;
  virtual void
  updateAvailableFunctions(const std::map<std::string, std::string> &functionInitialisationStrings) override;

  virtual void setFitType(std::string const &name) override;

  virtual void updateMultiDatasetParameters(const Mantid::API::ITableWorkspace &table) override;

  /// Used by IqtTemplatePresenter
  virtual void setNumberOfExponentials(int nExponentials) override;
  virtual void setStretchExponential(bool on) override;
  virtual void setBackground(std::string const &name) override;
  virtual void tieIntensities(bool on) override;
  virtual bool canTieIntensities() const override;

  /// Used by ConvTemplatePresenter
  virtual void setSubType(std::size_t subTypeIndex, int typeIndex) override;
  virtual void setDeltaFunction(bool on) override;
  virtual void setTempCorrection(bool on) override;
  virtual void setBackgroundA0(double value) override;
  virtual void setResolution(const std::vector<std::pair<std::string, size_t>> &fitResolutions) override;
  virtual void setQValues(const std::vector<double> &qValues) override;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt