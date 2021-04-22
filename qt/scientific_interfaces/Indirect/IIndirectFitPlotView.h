// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Common/IndexTypes.h"
#include "MantidQtWidgets/Common/MantidWidget.h"

#include <QObject>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
using namespace MantidWidgets;

class MANTIDQT_INDIRECT_DLL IIndirectFitPlotView : public API::MantidWidget {
  Q_OBJECT

public:
  IIndirectFitPlotView(QWidget *parent = nullptr) : API::MantidWidget(parent){};
  virtual ~IIndirectFitPlotView(){};

  virtual void watchADS(bool watch) = 0;

  virtual WorkspaceIndex getSelectedSpectrum() const = 0;
  virtual FitDomainIndex getSelectedSpectrumIndex() const = 0;
  virtual TableDatasetIndex getSelectedDataIndex() const = 0;
  virtual TableDatasetIndex dataSelectionSize() const = 0;
  virtual bool isPlotGuessChecked() const = 0;

  virtual void hideMultipleDataSelection() = 0;
  virtual void showMultipleDataSelection() = 0;

  virtual void setAvailableSpectra(WorkspaceIndex minimum, WorkspaceIndex maximum) = 0;
  virtual void setAvailableSpectra(const std::vector<WorkspaceIndex>::const_iterator &from,
                                   const std::vector<WorkspaceIndex>::const_iterator &to) = 0;

  virtual void setMinimumSpectrum(int minimum) = 0;
  virtual void setMaximumSpectrum(int maximum) = 0;
  virtual void setPlotSpectrum(WorkspaceIndex spectrum) = 0;
  virtual void appendToDataSelection(const std::string &dataName) = 0;
  virtual void setNameInDataSelection(const std::string &dataName, TableDatasetIndex index) = 0;
  virtual void clearDataSelection() = 0;

  virtual void plotInTopPreview(const QString &name, Mantid::API::MatrixWorkspace_sptr workspace,
                                WorkspaceIndex spectrum, Qt::GlobalColor colour) = 0;
  virtual void plotInBottomPreview(const QString &name, Mantid::API::MatrixWorkspace_sptr workspace,
                                   WorkspaceIndex spectrum, Qt::GlobalColor colour) = 0;

  virtual void removeFromTopPreview(const QString &name) = 0;
  virtual void removeFromBottomPreview(const QString &name) = 0;

  virtual void enablePlotGuess(bool enable) = 0;
  virtual void enableSpectrumSelection(bool enable) = 0;
  virtual void enableFitRangeSelection(bool enable) = 0;

  virtual void setFitSingleSpectrumText(QString const &text) = 0;
  virtual void setFitSingleSpectrumEnabled(bool enable) = 0;

  virtual void setBackgroundLevel(double value) = 0;

  virtual void setFitRange(double minimum, double maximum) = 0;
  virtual void setFitRangeMinimum(double minimum) = 0;
  virtual void setFitRangeMaximum(double maximum) = 0;

  virtual void setBackgroundRangeVisible(bool visible) = 0;
  virtual void setHWHMRangeVisible(bool visible) = 0;

  virtual void displayMessage(const std::string &message) const = 0;
  virtual void disableSpectrumPlotSelection() = 0;

  virtual void allowRedraws(bool state) = 0;
  virtual void redrawPlots() = 0;

public slots:
  virtual void clearTopPreview() = 0;
  virtual void clearBottomPreview() = 0;
  virtual void clearPreviews() = 0;
  virtual void setHWHMRange(double minimum, double maximum) = 0;
  virtual void setHWHMMaximum(double minimum) = 0;
  virtual void setHWHMMinimum(double maximum) = 0;

signals:
  void selectedFitDataChanged(TableDatasetIndex /*_t1*/);
  void plotCurrentPreview();
  void plotSpectrumChanged(WorkspaceIndex /*_t1*/);
  void plotGuessChanged(bool /*_t1*/);
  void fitSelectedSpectrum();
  void startXChanged(double /*_t1*/);
  void endXChanged(double /*_t1*/);
  void hwhmMinimumChanged(double /*_t1*/);
  void hwhmMaximumChanged(double /*_t1*/);
  void hwhmChanged(double /*_t1*/, double /*_t2*/);
  void backgroundChanged(double /*_t1*/);
};
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
