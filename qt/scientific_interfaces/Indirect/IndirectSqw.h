// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_INDIRECTSQW_H_
#define MANTIDQTCUSTOMINTERFACES_INDIRECTSQW_H_

#include "IndirectDataReductionTab.h"

#include "MantidKernel/System.h"
#include "ui_IndirectSqw.h"

namespace MantidQt {
namespace CustomInterfaces {
/** IndirectSqw

  @author Dan Nixon
  @date 23/07/2014
*/
class DLLExport IndirectSqw : public IndirectDataReductionTab {
  Q_OBJECT

public:
  IndirectSqw(IndirectDataReduction *idrUI, QWidget *parent = nullptr);
  ~IndirectSqw() override;

  void setup() override;
  void run() override;
  bool validate() override;

private slots:
  void handleDataReady(QString const &dataName) override;
  void sqwAlgDone(bool error);

  void runClicked();
  void plotSpectrumClicked();
  void plotContourClicked();
  void saveClicked();

  void updateRunButton(bool enabled = true,
                       std::string const &enableOutputButtons = "unchanged",
                       QString const message = "Run",
                       QString const tooltip = "");

private:
  void plotRqwContour(std::string const &sampleName);
  void setDefaultQAndEnergy();
  void setQRange(std::tuple<double, double> const &axisRange);
  void setEnergyRange(std::tuple<double, double> const &axisRange);
  void setFileExtensionsByName(bool filter) override;

  std::size_t getOutWsNumberOfSpectra() const;

  void setPlotSpectrumIndexMax(int maximum);

  void setRunEnabled(bool enabled);
  void setPlotSpectrumEnabled(bool enabled);
  void setPlotContourEnabled(bool enabled);
  void setSaveEnabled(bool enabled);
  void setOutputButtonsEnabled(std::string const &enableOutputButtons);
  void setPlotSpectrumIsPlotting(bool plotting);
  void setPlotContourIsPlotting(bool plotting);

  Ui::IndirectSqw m_uiForm;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_INDIRECTSQW_H_
