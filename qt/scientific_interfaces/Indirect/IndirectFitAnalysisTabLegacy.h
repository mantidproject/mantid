// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_IFATABLEGACY_H_
#define MANTIDQTCUSTOMINTERFACESIDA_IFATABLEGACY_H_

#include "IndirectDataAnalysisTab.h"
#include "IndirectFitDataPresenterLegacy.h"
#include "IndirectFitOutputOptionsPresenter.h"
#include "IndirectFitOutputOptionsView.h"
#include "IndirectFitPlotPresenterLegacy.h"
#include "IndirectFittingModelLegacy.h"
#include "IndirectSpectrumSelectionPresenterLegacy.h"
#include "IndirectSpectrumSelectionViewLegacy.h"

#include "MantidQtWidgets/Common/IndirectFitPropertyBrowserLegacy.h"

#include <boost/optional.hpp>

#include <QtCore>

#include <memory>
#include <type_traits>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class DLLExport IndirectFitAnalysisTabLegacy : public IndirectDataAnalysisTab {
  Q_OBJECT

public:
  IndirectFitAnalysisTabLegacy(IndirectFittingModelLegacy *model,
                               QWidget *parent = nullptr);

  void setFitDataPresenter(
      std::unique_ptr<IndirectFitDataPresenterLegacy> presenter);
  void setPlotView(IIndirectFitPlotViewLegacy *view);
  void setSpectrumSelectionView(IndirectSpectrumSelectionViewLegacy *view);
  void setOutputOptionsView(IIndirectFitOutputOptionsView *view);
  void setFitPropertyBrowser(
      MantidWidgets::IndirectFitPropertyBrowserLegacy *browser);

  virtual std::string tabName() const = 0;

  virtual bool hasResolution() const = 0;

  std::size_t getSelectedDataIndex() const;
  std::size_t getSelectedSpectrum() const;
  bool isRangeCurrentlySelected(std::size_t dataIndex,
                                std::size_t spectrum) const;

  QString selectedFitType() const;

  size_t numberOfCustomFunctions(const std::string &functionName) const;

  void setConvolveMembers(bool convolveMembers);

  void updateTies();

  void setCustomSettingEnabled(const QString &customName, bool enabled);

  void setParameterValue(const std::string &functionName,
                         const std::string &parameterName, double value);

  void setDefaultPeakType(const std::string &function);

  void addCheckBoxFunctionGroup(
      const QString &groupName,
      const std::vector<Mantid::API::IFunction_sptr> &functions,
      bool defaultValue = false);

  void addSpinnerFunctionGroup(
      const QString &groupName,
      const std::vector<Mantid::API::IFunction_sptr> &functions,
      int minimum = 0, int maximum = 10, int defaultValue = 0);

  void addComboBoxFunctionGroup(
      const QString &groupName,
      const std::vector<Mantid::API::IFunction_sptr> &functions);
  void clearFitTypeComboBox();

  void setBackgroundOptions(const QStringList &backgrounds);

  bool boolSettingValue(const QString &settingKey) const;

  void setCustomBoolSetting(const QString &settingKey, bool value);

  int intSettingValue(const QString &settingKey) const;

  double doubleSettingValue(const QString &settingKey) const;

  QString enumSettingValue(const QString &settingKey) const;

  void addBoolCustomSetting(const QString &settingKey,
                            const QString &settingName,
                            bool defaultValue = false);

  void addDoubleCustomSetting(const QString &settingKey,
                              const QString &settingName,
                              double defaultValue = 0);

  void addIntCustomSetting(const QString &settingKey,
                           const QString &settingName, int defaultValue = 0);

  void addEnumCustomSetting(const QString &settingKey,
                            const QString &settingName,
                            const QStringList &options);

  void addOptionalDoubleSetting(const QString &settingKey,
                                const QString &settingName,
                                const QString &optionKey,
                                const QString &optionName, bool enabled = false,
                                double defaultValue = 0);

  void setCustomSettingChangesFunction(const QString &settingKey,
                                       bool changesFunction);

public slots:
  void setBrowserWorkspace() override;

protected:
  IndirectFittingModelLegacy *fittingModel() const;

  void run() override;

  void setSampleWSSuffixes(const QStringList &suffices);
  void setSampleFBSuffixes(const QStringList &suffices);
  void setResolutionWSSuffixes(const QStringList &suffices);
  void setResolutionFBSuffixes(const QStringList &suffices);

  void setAlgorithmProperties(Mantid::API::IAlgorithm_sptr fitAlgorithm) const;
  void runFitAlgorithm(Mantid::API::IAlgorithm_sptr fitAlgorithm);
  void runSingleFit(Mantid::API::IAlgorithm_sptr fitAlgorithm);
  virtual void setupFit(Mantid::API::IAlgorithm_sptr fitAlgorithm);

  virtual void setRunIsRunning(bool running) = 0;
  virtual void setRunEnabled(bool enable) = 0;
  void setEditResultVisible(bool visible);

signals:
  void functionChanged();
  void parameterChanged(const Mantid::API::IFunction * /*_t1*/);
  void customBoolChanged(const QString &key, bool value);
  void updateAvailableFitTypes();

protected slots:

  void setModelFitFunction();
  void setModelStartX(double startX);
  void setModelEndX(double startX);
  void setDataTableStartX(double startX);
  void setDataTableEndX(double endX);
  void setDataTableExclude(const std::string &exclude);
  void setBrowserStartX(double startX);
  void setBrowserEndX(double endX);
  void updateBrowserFittingRange();
  void setBrowserWorkspace(std::size_t dataIndex);
  void setBrowserWorkspaceIndex(std::size_t spectrum);
  void setBrowserWorkspaceIndex(int spectrum);
  void tableStartXChanged(double startX, std::size_t dataIndex,
                          std::size_t spectrum);
  void tableEndXChanged(double endX, std::size_t dataIndex,
                        std::size_t spectrum);
  void tableExcludeChanged(const std::string &exclude, std::size_t dataIndex,
                           std::size_t spectrum);

  void updateFitOutput(bool error);
  void updateSingleFitOutput(bool error);
  void fitAlgorithmComplete(bool error);

  void singleFit();
  void singleFit(std::size_t dataIndex, std::size_t spectrum);
  void executeFit();

  void updateAttributeValues();
  void updateAttributeValues(Mantid::API::IFunction_sptr function,
                             std::vector<std::string> const &attributeNames);
  void updateAttributeValues(
      Mantid::API::IFunction_sptr function,
      std::vector<std::string> const &attributeNames,
      std::unordered_map<std::string, Mantid::API::IFunction::Attribute> const
          &attributes);
  void updateFitBrowserAttributeValues();
  std::unordered_map<std::string, Mantid::API::IFunction::Attribute>
  getAttributes(Mantid::API::IFunction_sptr const &function,
                std::vector<std::string> const &attributeNames);
  void updateParameterValues();
  void updateParameterValues(
      const std::unordered_map<std::string, ParameterValueLegacy> &parameters);
  void updateFitBrowserParameterValues();

  void updateResultOptions();

private slots:
  void updatePlotGuess();
  void plotSelectedSpectra();

private:
  /// Overidden by child class.
  void setup() override;
  void loadSettings(const QSettings &settings) override;
  virtual void setupFitTab() = 0;
  bool validate() override;

  void setFileExtensionsByName(bool filter) override;
  void setSampleSuffixes(std::string const &tab, bool filter);
  void setResolutionSuffixes(std::string const &tab, bool filter);

  void connectDataAndPlotPresenters();
  void connectSpectrumAndPlotPresenters();
  void connectFitBrowserAndPlotPresenter();
  void connectDataAndSpectrumPresenters();
  void connectDataAndFitBrowserPresenters();

  void plotSelectedSpectra(std::vector<SpectrumToPlot> const &spectra);
  void plotSpectrum(std::string const &workspaceName, std::size_t const &index);

  std::string getOutputBasename() const;
  Mantid::API::WorkspaceGroup_sptr getResultWorkspace() const;
  std::vector<std::string> getFitParameterNames() const;

  void enableFitButtons(bool enable);
  void enableOutputOptions(bool enable);
  void setPDFWorkspace(std::string const &workspaceName);

  std::unique_ptr<IndirectFittingModelLegacy> m_fittingModel;
  MantidWidgets::IndirectFitPropertyBrowserLegacy *m_fitPropertyBrowser;
  std::unique_ptr<IndirectFitDataPresenterLegacy> m_dataPresenter;
  std::unique_ptr<IndirectFitPlotPresenterLegacy> m_plotPresenter;
  std::unique_ptr<IndirectSpectrumSelectionPresenterLegacy> m_spectrumPresenter;
  std::unique_ptr<IndirectFitOutputOptionsPresenter> m_outOptionsPresenter;

  Mantid::API::IAlgorithm_sptr m_fittingAlgorithm;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_IFATABLEGACY_H_ */
