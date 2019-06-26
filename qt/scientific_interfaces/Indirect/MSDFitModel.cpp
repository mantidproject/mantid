// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MSDFitModel.h"

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

void MSDFitModel::setFitType(const std::string &fitType) {
  m_fitType = fitType;
}

std::string MSDFitModel::sequentialFitOutputName() const {
  if (isMultiFit())
    return "MultiMSDFit_" + m_fitType + "_Results";
  return createOutputName("%1%_MSDFit_" + m_fitType + "_s%2%", "_to_", DatasetIndex{0});
}

std::string MSDFitModel::simultaneousFitOutputName() const {
  return sequentialFitOutputName();
}

std::string MSDFitModel::singleFitOutputName(DatasetIndex index,
  WorkspaceIndex spectrum) const {
  return createSingleFitOutputName("%1%_MSDFit_" + m_fitType + "_s%2%_Results",
                                   index, spectrum);
}

std::vector<std::string> MSDFitModel::getSpectrumDependentAttributes() const {
  return {};
}

std::string MSDFitModel::getResultXAxisUnit() const { return "Temperature"; }

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
