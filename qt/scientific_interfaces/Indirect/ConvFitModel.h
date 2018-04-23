#ifndef MANTIDQTCUSTOMINTERFACESIDA_CONVFITMODEL_H_
#define MANTIDQTCUSTOMINTERFACESIDA_CONVFITMODEL_H_

#include "IndirectFittingModel.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class DLLExport ConvFitModel : public IndirectFittingModel {
public:
  ConvFitModel();
  ~ConvFitModel() override;

  boost::optional<double> getInstrumentResolution(std::size_t dataIndex) const;

  void setFitFunction(Mantid::API::IFunction_sptr model,
                      Mantid::API::IFunction_sptr background) override;
  void setTemperature(const boost::optional<double> &temperature);

  void addWorkspace(Mantid::API::MatrixWorkspace_sptr workspace,
                    const Spectra &spectra) override;
  void removeWorkspace(std::size_t index) override;
  void setResolution(Mantid::API::MatrixWorkspace_sptr resolution);
  void setFitTypeString(const std::string &fitType);

  std::unordered_map<std::string, ParameterValue>
  getDefaultParameters(std::size_t index) const override;

  void addSampleLogs();

private:
  Mantid::API::IAlgorithm_sptr sequentialFitAlgorithm() const override;
  Mantid::API::IAlgorithm_sptr simultaneousFitAlgorithm() const override;
  std::string sequentialFitOutputName() const override;
  std::string simultaneousFitOutputName() const override;

  IndirectFitOutput
  createFitOutput(Mantid::API::WorkspaceGroup_sptr resultGroup,
                  Mantid::API::ITableWorkspace_sptr parameterTable,
                  Mantid::API::MatrixWorkspace_sptr resultWorkspace,
                  const std::vector<std::unique_ptr<IndirectFitData>>
                      &m_fittingData) const override;
  void addOutput(IndirectFitOutput *fitOutput,
                 Mantid::API::WorkspaceGroup_sptr resultGroup,
                 Mantid::API::ITableWorkspace_sptr parameterTable,
                 Mantid::API::MatrixWorkspace_sptr resultWorkspace,
                 const std::vector<std::unique_ptr<IndirectFitData>>
                     &m_fittingData) const override;

  std::size_t maximumHistograms() const;

  void extendResolution();

  void setParameterNameChanges(const Mantid::API::IFunction &model,
                               bool backgroundUsed);

  boost::weak_ptr<Mantid::API::MatrixWorkspace> m_resolutionWorkspace;
  std::unordered_map<std::string, std::string> m_parameterNameChanges;
  boost::optional<double> m_temperature;
  std::string m_backgroundString;
  std::string m_fitType;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
