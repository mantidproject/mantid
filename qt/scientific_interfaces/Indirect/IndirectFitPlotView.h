// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITPLOTVIEW_H_
#define MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITPLOTVIEW_H_

#include "ui_IndirectFitPreviewPlot.h"

#include "DllConfig.h"
#include "IIndirectFitPlotView.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL IndirectFitPlotView : public IIndirectFitPlotView {
  Q_OBJECT

public:
  IndirectFitPlotView(QWidget *parent = nullptr);
  virtual ~IndirectFitPlotView() override;

  WorkspaceIndex getSelectedSpectrum() const override;
  SpectrumRowIndex getSelectedSpectrumIndex() const override;
  DatasetIndex getSelectedDataIndex() const override;
  DatasetIndex dataSelectionSize() const override;
  bool isPlotGuessChecked() const override;

  void hideMultipleDataSelection() override;
  void showMultipleDataSelection() override;

  void setAvailableSpectra(WorkspaceIndex minimum, WorkspaceIndex maximum) override;
  void setAvailableSpectra(
      const std::vector<WorkspaceIndex>::const_iterator &from,
      const std::vector<WorkspaceIndex>::const_iterator &to) override;

  void setMinimumSpectrum(int minimum) override;
  void setMaximumSpectrum(int maximum) override;
  void setPlotSpectrum(WorkspaceIndex spectrum) override;
  void appendToDataSelection(const std::string &dataName) override;
  void setNameInDataSelection(const std::string &dataName,
                              DatasetIndex index) override;
  void clearDataSelection() override;

  void plotInTopPreview(const QString &name,
                        Mantid::API::MatrixWorkspace_sptr workspace,
                        WorkspaceIndex spectrum, Qt::GlobalColor colour) override;
  void plotInBottomPreview(const QString &name,
                           Mantid::API::MatrixWorkspace_sptr workspace,
                           WorkspaceIndex spectrum,
                           Qt::GlobalColor colour) override;

  void removeFromTopPreview(const QString &name) override;
  void removeFromBottomPreview(const QString &name) override;

  void enablePlotGuess(bool enable) override;
  void enableSpectrumSelection(bool enable) override;
  void enableFitRangeSelection(bool enable) override;

  void setFitSingleSpectrumText(QString const &text) override;
  void setFitSingleSpectrumEnabled(bool enable) override;

  void setBackgroundLevel(double value) override;

  void setFitRange(double minimum, double maximum) override;
  void setFitRangeMinimum(double minimum) override;
  void setFitRangeMaximum(double maximum) override;

  void setBackgroundRangeVisible(bool visible) override;
  void setHWHMRangeVisible(bool visible) override;

  void displayMessage(const std::string &message) const override;

public slots:
  void clearTopPreview() override;
  void clearBottomPreview() override;
  void clear() override;
  void setHWHMRange(double minimum, double maximum) override;
  void setHWHMMaximum(double minimum) override;
  void setHWHMMinimum(double maximum) override;

private slots:
  void emitPlotSpectrumChanged(int /*spectrum*/);
  void emitPlotSpectrumChanged(const QString &spectrum);
  void emitSelectedFitDataChanged(int /*index*/);
  void emitPlotGuessChanged(int /*doPlotGuess*/);

private:
  std::string getSpectrumText() const;

  void addFitRangeSelector();
  void addBackgroundRangeSelector();
  void addHWHMRangeSelector();

  std::unique_ptr<Ui::IndirectFitPreviewPlot> m_plotForm;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
