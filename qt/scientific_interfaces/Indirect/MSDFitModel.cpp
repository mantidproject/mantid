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
    return "MultiMSDFit_" + m_fitType + "_Result";
  return createOutputName("%1%_MSDFit_" + m_fitType + "_s%2%", "_to_", 0);
}

std::string MSDFitModel::simultaneousFitOutputName() const {
  return sequentialFitOutputName();
}

std::string MSDFitModel::singleFitOutputName(std::size_t index,
                                             std::size_t spectrum) const {
  return createSingleFitOutputName("%1%_MSDFit_" + m_fitType + "_s%2%", index,
                                   spectrum);
}

std::string MSDFitModel::getResultXAxisUnit() const { return ""; }

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
