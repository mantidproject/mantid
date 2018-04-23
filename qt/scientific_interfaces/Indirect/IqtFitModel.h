#ifndef MANTIDQTCUSTOMINTERFACESIDA_IQTFITMODEL_H_
#define MANTIDQTCUSTOMINTERFACESIDA_IQTFITMODEL_H_

#include "IndirectFittingModel.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class DLLExport IqtFitModel : public IndirectFittingModel {
public:
  void setFitTypeString(const std::string &fitType);
  void addWorkspace(Mantid::API::MatrixWorkspace_sptr workspace,
                    const Spectra &spectra) override;

  std::unordered_map<std::string, ParameterValue>
  getDefaultParameters(std::size_t index) const override;

  std::string createIntensityTie() const;

private:
  Mantid::API::IAlgorithm_sptr sequentialFitAlgorithm() const override;
  Mantid::API::IAlgorithm_sptr simultaneousFitAlgorithm() const override;
  std::string sequentialFitOutputName() const override;
  std::string simultaneousFitOutputName() const override;

  std::string m_fitType;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
