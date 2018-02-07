#ifndef MANTIDQTCUSTOMINTERFACESIDA_IQTFIT_H_
#define MANTIDQTCUSTOMINTERFACESIDA_IQTFIT_H_

#include "IndirectFitAnalysisTab.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "ui_IqtFit.h"

#include <boost/weak_ptr.hpp>

namespace Mantid {
namespace API {
class IFunction;
class CompositeFunction;
} // namespace API
} // namespace Mantid

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class DLLExport IqtFit : public IndirectFitAnalysisTab {
  Q_OBJECT

public:
  IqtFit(QWidget *parent = nullptr);

  Mantid::API::IFunction_sptr fitFunction() const override;

  Mantid::API::MatrixWorkspace_sptr fitWorkspace() const override;

  bool doPlotGuess() const override;

private:
  void setup() override;
  void run() override;
  bool validate() override;
  void loadSettings(const QSettings &settings) override;

protected:
  QHash<QString, double> createDefaultValues() const override;
  std::string createSingleFitOutputName() const override;
  std::string createSequentialFitOutputName() const override;
  Mantid::API::IAlgorithm_sptr singleFitAlgorithm() const override;
  Mantid::API::IAlgorithm_sptr sequentialFitAlgorithm() const override;
  void setMaxIterations(Mantid::API::IAlgorithm_sptr fitAlgorithm,
                        int maxIterations) const override;
  void enablePlotResult() override;
  void disablePlotResult() override;
  void enableSaveResult() override;
  void disableSaveResult() override;
  void addGuessPlot(Mantid::API::MatrixWorkspace_sptr workspace) override;
  void removeGuessPlot() override;

protected slots:
  void newDataLoaded(const QString wsName);
  void updatePreviewPlots() override;
  void updatePlotRange() override;
  void specMinChanged(int value);
  void specMaxChanged(int value);
  void startXChanged(double startX) override;
  void endXChanged(double endX) override;
  void singleFit();
  void algorithmComplete(bool error) override;
  void updatePlotOptions() override;
  void plotWorkspace();
  void saveResult();
  void fitFunctionChanged();

private:
  void disablePlotGuess() override;
  void enablePlotGuess() override;

  std::string
  createIntensityTie(Mantid::API::IFunction_sptr function) const;
  std::vector<std::string>
  getParameters(Mantid::API::IFunction_sptr function,
                const std::string &shortParameterName) const;
  std::string constructBaseName() const;
  std::string fitTypeString() const;
  Mantid::API::IAlgorithm_sptr iqtFitAlgorithm(const size_t &specMin,
                                               const size_t &specMax) const;
  Mantid::API::IAlgorithm_sptr
  replaceInfinityAndNaN(Mantid::API::MatrixWorkspace_sptr inputWS) const;

  std::unique_ptr<Ui::IqtFit> m_uiForm;
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_IQTFIT_H_ */
