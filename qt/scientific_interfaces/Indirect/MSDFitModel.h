// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_MSDFITMODEL_H_
#define MANTIDQTCUSTOMINTERFACESIDA_MSDFITMODEL_H_

#include "IndirectFittingModel.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class DLLExport MSDFitModel : public IndirectFittingModel {
public:
  void setFitType(const std::string &fitType);

  std::string sequentialFitOutputName() const override;
  std::string simultaneousFitOutputName() const override;
  std::string singleFitOutputName(DatasetIndex index,
                                  WorkspaceIndex spectrum) const override;

  std::vector<std::string> getSpectrumDependentAttributes() const override;

private:
  std::string getResultXAxisUnit() const override;

  std::string m_fitType;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
