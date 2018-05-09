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

  Mantid::API::MatrixWorkspace_sptr fitWorkspace() const override;

  bool doPlotGuess() const override;

private:
  void setup() override;
  bool validate() override;
  void loadSettings(const QSettings &settings) override;

protected:
  int minimumSpectrum() const override;
  int maximumSpectrum() const override;

  QHash<QString, double> createDefaultValues() const override;
  std::string createSingleFitOutputName() const override;
  std::string createSequentialFitOutputName() const override;
  Mantid::API::IAlgorithm_sptr singleFitAlgorithm() const override;
  Mantid::API::IAlgorithm_sptr sequentialFitAlgorithm() const override;
  void enablePlotResult() override;
  void disablePlotResult() override;
  void enableSaveResult() override;
  void disableSaveResult() override;
  void enablePlotPreview() override;
  void disablePlotPreview() override;
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
  void backgroundSelectorChanged(double val);
  void singleFit();
  void algorithmComplete(bool error) override;
  void updatePlotOptions() override;
  void plotWorkspace();
  void saveResult();
  void fitFunctionChanged();
  void parameterUpdated(const Mantid::API::IFunction *function);
  void customBoolUpdated(const QString &key, bool value);

private:
  void disablePlotGuess() override;
  void enablePlotGuess() override;

  void updateIntensityTie();
  void updateIntensityTie(const QString &intensityTie);
  std::string createIntensityTie(Mantid::API::IFunction_sptr function) const;
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
  QString m_tiedParameter;
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_IQTFIT_H_ */
