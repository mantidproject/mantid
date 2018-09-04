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
  std::string singleFitOutputName(std::size_t index,
                                  std::size_t spectrum) const override;

private:
  std::string getResultXAxisUnit() const override;

  std::string m_fitType;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
