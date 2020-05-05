// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IndirectFittingModel.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class DLLExport MSDFitModel : public IndirectFittingModel {
public:
  std::string sequentialFitOutputName() const override;
  std::string simultaneousFitOutputName() const override;
  std::string singleFitOutputName(TableDatasetIndex index,
                                  WorkspaceIndex spectrum) const override;

  std::vector<std::string> getSpectrumDependentAttributes() const override;
  void setFitTypeString(const std::string &fitType);

private:
  std::string getResultXAxisUnit() const override;
  std::string m_fitType;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
