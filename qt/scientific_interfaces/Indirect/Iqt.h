// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_IQT_H_
#define MANTIDQTCUSTOMINTERFACESIDA_IQT_H_

#include "IndirectDataAnalysisTab.h"
#include "ui_Iqt.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
class DLLExport Iqt : public IndirectDataAnalysisTab {
  Q_OBJECT

public:
  Iqt(QWidget *parent = nullptr);

private:
  void run() override;
  void setup() override;
  bool validate() override;
  void loadSettings(const QSettings &settings) override;
  void setBrowserWorkspace() override{};

  bool isErrorsEnabled();

  std::size_t getXMinIndex(Mantid::MantidVec const &firstSpectraYData,
                           std::vector<double>::const_iterator iter);
  double getXMinValue(Mantid::API::MatrixWorkspace_const_sptr workspace,
                      std::size_t const &index);

  void plotResult(QString const &workspaceName);

  void setRunEnabled(bool enabled);
  void setPlotSpectrumEnabled(bool enabled);
  void setTiledPlotEnabled(bool enabled);
  void setSaveResultEnabled(bool enabled);
  void setButtonsEnabled(bool enabled);
  void setRunIsRunning(bool running);
  void setPlotSpectrumIsPlotting(bool plotting);
  void setTiledPlotIsPlotting(bool plotting);

  void setTiledPlotFirstIndex(int value);
  void setTiledPlotLastIndex(int value);
  void setMinMaxOfTiledPlotFirstIndex(int minimum, int maximum);
  void setMinMaxOfTiledPlotLastIndex(int minimum, int maximum);
  void setPlotSpectrumIndexMax(int maximum);
  void setPlotSpectrumIndex(int value);
  int getPlotSpectrumIndex();

private slots:
  void algorithmComplete(bool error);
  void plotInput(const QString &wsname);
  void rsRangeChangedLazy(double min, double max);
  void updateRS(QtProperty *prop, double val);
  void updatePropertyValues(QtProperty *prop, double val);
  void updateDisplayedBinParameters();
  void setTiledPlotFirstPlot(int value);
  void setTiledPlotLastPlot(int value);
  void runClicked();
  void saveClicked();
  void plotClicked();
  void errorsClicked();
  void plotTiled();

private:
  Ui::Iqt m_uiForm;
  QtTreePropertyBrowser *m_iqtTree;
  bool m_iqtResFileType;
  int m_maxTiledPlots = 17;
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACESIDA_IQT_H_ */
