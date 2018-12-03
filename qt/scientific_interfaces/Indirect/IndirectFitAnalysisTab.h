// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_IFATAB_H_
#define MANTIDQTCUSTOMINTERFACESIDA_IFATAB_H_

#include "IndirectDataAnalysisTab.h"
#include "IndirectFitDataPresenter.h"
#include "IndirectFitPlotPresenter.h"
#include "IndirectFittingModel.h"
#include "IndirectSpectrumSelectionPresenter.h"
#include "IndirectSpectrumSelectionView.h"

#include "MantidQtWidgets/Common/IndirectFitPropertyBrowser.h"

#include <boost/optional.hpp>

#include <QtCore>

#include <memory>
#include <type_traits>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class DLLExport IndirectFitAnalysisTab : public IndirectDataAnalysisTab {
  Q_OBJECT

public:
  /// Constructor
  IndirectFitAnalysisTab(IndirectFittingModel *model,
                         QWidget *parent = nullptr);

  void setFitDataPresenter(std::unique_ptr<IndirectFitDataPresenter> presenter);
  void setPlotView(IIndirectFitPlotView *view);
  void setSpectrumSelectionView(IndirectSpectrumSelectionView *view);
  void
  setFitPropertyBrowser(MantidWidgets::IndirectFitPropertyBrowser *browser);

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

protected:
  IndirectFittingModel *fittingModel() const;

  void setSampleWSSuffices(const QStringList &suffices);
  void setSampleFBSuffices(const QStringList &suffices);
  void setResolutionWSSuffices(const QStringList &suffices);
  void setResolutionFBSuffices(const QStringList &suffices);

  void run() override;
  void plotResult(const QString &plotType);
  void plotAll(Mantid::API::WorkspaceGroup_sptr workspaces);
  void plotParameter(Mantid::API::WorkspaceGroup_sptr workspace,
                     const std::string &parameter);
  void plotAll(Mantid::API::MatrixWorkspace_sptr workspace);
  void plotParameter(Mantid::API::MatrixWorkspace_sptr workspace,
                     const std::string &parameter);
  void plotSpectrum(Mantid::API::MatrixWorkspace_sptr workspace);
  void plotSpectrum(Mantid::API::MatrixWorkspace_sptr workspace,
                    const std::string &parameterToPlot);

  void setAlgorithmProperties(Mantid::API::IAlgorithm_sptr fitAlgorithm) const;
  void runFitAlgorithm(Mantid::API::IAlgorithm_sptr fitAlgorithm);
  void runSingleFit(Mantid::API::IAlgorithm_sptr fitAlgorithm);
  virtual void setupFit(Mantid::API::IAlgorithm_sptr fitAlgorithm);

  void updatePlotOptions(QComboBox *cbPlotType);
  void enablePlotResult(bool error);
  bool isResultWorkspacePlottable() const;
  bool isResultWorkspacePlottable(
      Mantid::API::WorkspaceGroup_sptr resultWorkspaces) const;

  void setPlotOptions(QComboBox *cbPlotType,
                      const std::vector<std::string> &parameters) const;

  void setPlotOptions(QComboBox *cbPlotType,
                      const QSet<QString> &options) const;

  virtual void setPlotResultEnabled(bool enabled) = 0;
  virtual void setSaveResultEnabled(bool enabled) = 0;

  virtual void setRunIsRunning(bool running) = 0;
	virtual void setFitSingleSpectrumIsFitting(bool fitting) = 0;

signals:
  void functionChanged();
  void parameterChanged(const Mantid::API::IFunction *);
  void customBoolChanged(const QString &key, bool value);
  void updateFitTypes();

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
  void setBrowserWorkspace();
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
      const std::unordered_map<std::string, ParameterValue> &parameters);
  void updateFitBrowserParameterValues();

  virtual void updatePlotOptions() = 0;

  void updateResultOptions();
  void saveResult();

private slots:
  void emitUpdateFitTypes();

private:
  /// Overidden by child class.
  void setup() override;
  void loadSettings(const QSettings &settings) override;
  virtual void setupFitTab() = 0;
  bool validate() override;

  void connectDataAndPlotPresenters();
  void connectSpectrumAndPlotPresenters();
  void connectFitBrowserAndPlotPresenter();
  void connectDataAndSpectrumPresenters();
  void connectDataAndFitBrowserPresenters();

  std::unique_ptr<IndirectFittingModel> m_fittingModel;
  MantidWidgets::IndirectFitPropertyBrowser *m_fitPropertyBrowser;
  std::unique_ptr<IndirectFitDataPresenter> m_dataPresenter;
  std::unique_ptr<IndirectFitPlotPresenter> m_plotPresenter;
  std::unique_ptr<IndirectSpectrumSelectionPresenter> m_spectrumPresenter;

  Mantid::API::IAlgorithm_sptr m_fittingAlgorithm;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_IFATAB_H_ */
