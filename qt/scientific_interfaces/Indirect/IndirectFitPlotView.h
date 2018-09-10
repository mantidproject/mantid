#ifndef MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITPLOTVIEW_H_
#define MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITPLOTVIEW_H_

#include "ui_IndirectFitPreviewPlot.h"

#include "MantidAPI/MatrixWorkspace.h"

#include "MantidQtWidgets/Common/MantidWidget.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class DLLExport IndirectFitPlotView : public API::MantidWidget {
  Q_OBJECT
public:
  IndirectFitPlotView(QWidget *parent);
  ~IndirectFitPlotView() override;

  std::size_t getSelectedSpectrum() const;
  int getSelectedSpectrumIndex() const;
  int getSelectedDataIndex() const;
  std::size_t dataSelectionSize() const;
  bool isPlotGuessChecked() const;

  void hideMultipleDataSelection();
  void showMultipleDataSelection();

  void setAvailableSpectra(std::size_t minimum, std::size_t maximum);
  void setAvailableSpectra(const std::vector<std::size_t>::const_iterator &from,
                           const std::vector<std::size_t>::const_iterator &to);

  void setMinimumSpectrum(int minimum);
  void setMaximumSpectrum(int maximum);
  void appendToDataSelection(const std::string &dataName);
  void setNameInDataSelection(const std::string &dataName, std::size_t index);
  void clearDataSelection();

  void plotInTopPreview(const QString &name,
                        Mantid::API::MatrixWorkspace_sptr workspace,
                        std::size_t spectrum, Qt::GlobalColor colour);
  void plotInBottomPreview(const QString &name,
                           Mantid::API::MatrixWorkspace_sptr workspace,
                           std::size_t spectrum, Qt::GlobalColor colour);

  void removeFromTopPreview(const QString &name);
  void removeFromBottomPreview(const QString &name);

  void disablePlotGuess();
  void enablePlotGuess();

  void disableSpectrumSelection();
  void enableSpectrumSelection();

  void disableFitRangeSelection();
  void enableFitRangeSelection();

  void setBackgroundLevel(double value);

  void setFitRange(double minimum, double maximum);
  void setFitRangeMinimum(double minimum);
  void setFitRangeMaximum(double maximum);

  void setBackgroundRangeVisible(bool visible);
  void setHWHMRangeVisible(bool visible);

public slots:
  void clearTopPreview();
  void clearBottomPreview();
  void clear();
  void setHWHMRange(double minimum, double maximum);
  void setHWHMMaximum(double minimum);
  void setHWHMMinimum(double maximum);

signals:
  void selectedFitDataChanged(std::size_t);
  void plotCurrentPreview();
  void plotSpectrumChanged(std::size_t);
  void plotGuessChanged(bool);
  void fitSelectedSpectrum();
  void startXChanged(double);
  void endXChanged(double);
  void hwhmMinimumChanged(double);
  void hwhmMaximumChanged(double);
  void hwhmChanged(double, double);
  void backgroundChanged(double);

private slots:
  void emitPlotSpectrumChanged(int);
  void emitPlotSpectrumChanged(const QString &spectrum);
  void emitSelectedFitDataChanged(int);
  void emitPlotGuessChanged(int);

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
