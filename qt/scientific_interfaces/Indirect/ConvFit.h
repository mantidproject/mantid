#ifndef MANTIDQTCUSTOMINTERFACESIDA_CONVFIT_H_
#define MANTIDQTCUSTOMINTERFACESIDA_CONVFIT_H_

#include "IndirectFitAnalysisTab.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "ui_ConvFit.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
class DLLExport ConvFit : public IndirectFitAnalysisTab {
  Q_OBJECT

public:
  ConvFit(QWidget *parent = nullptr);

  Mantid::API::IFunction_sptr fitFunction() const override;

  bool canPlotGuess() const override;

  bool doPlotGuess() const override;

protected:
  int minimumSpectrum() const override;
  int maximumSpectrum() const override;

  QHash<QString, double> createDefaultValues() const override;
  std::string createSingleFitOutputName() const override;
  std::string createSequentialFitOutputName() const override;
  std::string constructBaseName() const;
  Mantid::API::IAlgorithm_sptr singleFitAlgorithm() const override;
  Mantid::API::IAlgorithm_sptr sequentialFitAlgorithm() const override;
  QHash<QString, QString>
  functionNameChanges(Mantid::API::IFunction_sptr model) const override;
  void enablePlotResult() override;
  void disablePlotResult() override;
  void enableSaveResult() override;
  void disableSaveResult() override;
  void enablePlotPreview() override;
  void disablePlotPreview() override;
  void addGuessPlot(Mantid::API::MatrixWorkspace_sptr workspace) override;
  void removeGuessPlot() override;

private:
  void setup() override;
  void runFitAlgorithm(Mantid::API::IAlgorithm_sptr fitAlgorithm) override;
  bool validate() override;
  void loadSettings(const QSettings &settings) override;

protected slots:
  void algorithmComplete(bool error) override;
  void newDataLoaded(const QString &wsName);
  void extendResolutionWorkspace();
  void updatePreviewPlots() override;
  void updatePlotRange() override;
  void singleFit();
  void specMinChanged(int value);
  void specMaxChanged(int value);
  void startXChanged(double startX) override;
  void endXChanged(double endX) override;
  void backgLevel(double);
  void hwhmMinChanged(double);
  void hwhmMaxChanged(double);
  void updateHWHMFromResolution();
  void saveClicked();
  void plotClicked();
  void updatePlotOptions() override;
  void fitFunctionChanged();
  void parameterUpdated(const Mantid::API::IFunction *function);
  void sampleLogsAdded();

private:
  void fwhmChanged(double fwhm);

  void disablePlotGuess() override;
  void enablePlotGuess() override;

  double getInstrumentResolution(
      Mantid::API::MatrixWorkspace_sptr workspaceName) const;

  Mantid::API::CompositeFunction_sptr
  addTemperatureCorrection(Mantid::API::IFunction_sptr model) const;
  Mantid::API::CompositeFunction_sptr
  addTemperatureCorrection(Mantid::API::CompositeFunction_sptr model) const;
  Mantid::API::CompositeFunction_sptr
  applyTemperatureCorrection(Mantid::API::IFunction_sptr function,
                             Mantid::API::IFunction_sptr correction) const;
  Mantid::API::IFunction_sptr createTemperatureCorrection() const;
  Mantid::API::IFunction_sptr createResolutionFunction() const;

  Mantid::API::IAlgorithm_sptr
  cloneAlgorithm(Mantid::API::MatrixWorkspace_sptr inputWS,
                 const std::string &outputWS) const;
  Mantid::API::IAlgorithm_sptr
  appendAlgorithm(Mantid::API::MatrixWorkspace_sptr leftWS,
                  Mantid::API::MatrixWorkspace_sptr rightWS, int numHistograms,
                  const std::string &outputWSName) const;
  Mantid::API::IAlgorithm_sptr
  loadParameterFileAlgorithm(Mantid::API::MatrixWorkspace_sptr workspace,
                             const std::string &filename) const;

  void
  addFunctionNameChanges(Mantid::API::IFunction_sptr model,
                         const QString &modelPrefix, const QString &newPrefix,
                         QHash<QString, QString> &functionNameChanges) const;
  void addFunctionNameChanges(
      Mantid::API::CompositeFunction_sptr model, const QString &prefixPrefix,
      const QString &prefixSuffix, size_t fromIndex, size_t toIndex, int offset,
      QHash<QString, QString> &functionNameChanges) const;

  std::string fitTypeString() const;
  QString backgroundString(const QString &backgroundType) const;
  void addFunctionGroupToComboBox(
      const QString &groupName,
      const std::vector<Mantid::API::IFunction_sptr> &functions);
  void addSampleLogsToWorkspace(const std::string &workspaceName,
                                const std::string &logName,
                                const std::string &logText,
                                const std::string &logType);
  Mantid::API::IAlgorithm_sptr sequentialFit(const int &specMin,
                                             const int &specMax) const;
  QString backgroundType() const;

  std::unique_ptr<Ui::ConvFit> m_uiForm;
  bool m_confitResFileType;
  bool m_usedTemperature;
  double m_temperature;

  // ShortHand Naming for fit functions
  QHash<QString, std::string> m_fitStrings;
  QHash<QString, std::string> m_fitTypeToFunction;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_CONVFIT_H_ */
