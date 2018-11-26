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
#include "MantidAPI/MatrixWorkspace.h"

#include "MantidQtWidgets/Common/MantidWidget.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL IndirectFitPlotView : public API::MantidWidget {
  Q_OBJECT
public:
  IndirectFitPlotView(QWidget *parent = nullptr);
  virtual ~IndirectFitPlotView() override;

  virtual std::size_t getSelectedSpectrum() const;
  int getSelectedSpectrumIndex() const;
  int getSelectedDataIndex() const;
  virtual std::size_t dataSelectionSize() const;
  bool isPlotGuessChecked() const;

  void hideMultipleDataSelection();
  void showMultipleDataSelection();

	virtual void setAvailableSpectra(std::size_t minimum, std::size_t maximum);
  void setAvailableSpectra(const std::vector<std::size_t>::const_iterator &from,
                           const std::vector<std::size_t>::const_iterator &to);

  void setMinimumSpectrum(int minimum);
  void setMaximumSpectrum(int maximum);
  void setPlotSpectrum(int spectrum);
  void appendToDataSelection(const std::string &dataName);
  void setNameInDataSelection(const std::string &dataName, std::size_t index);
  void clearDataSelection();

  void plotInTopPreview(const QString &name,
                        Mantid::API::MatrixWorkspace_sptr workspace,
                        std::size_t spectrum, Qt::GlobalColor colour);
  void plotInBottomPreview(const QString &name,
                           Mantid::API::MatrixWorkspace_sptr workspace,
                           std::size_t spectrum, Qt::GlobalColor colour);

	virtual void removeFromTopPreview(const QString &name);
	virtual void removeFromBottomPreview(const QString &name);

  void enableFitSingleSpectrum(bool enable);
	virtual void enablePlotGuess(bool enable);
	virtual void enableSpectrumSelection(bool enable);
	virtual void enableFitRangeSelection(bool enable);

  void setBackgroundLevel(double value);

  void setFitRange(double minimum, double maximum);
	virtual void setFitRangeMinimum(double minimum);
	virtual void setFitRangeMaximum(double maximum);

  void setBackgroundRangeVisible(bool visible);
  void setHWHMRangeVisible(bool visible);

	virtual void displayMessage(const std::string &message) const;

public slots:
  void clearTopPreview();
  void clearBottomPreview();
	virtual void clear();
  void setHWHMRange(double minimum, double maximum);
	virtual void setHWHMMaximum(double minimum);
	virtual void setHWHMMinimum(double maximum);

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
