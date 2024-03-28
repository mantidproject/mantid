// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Common/IndexTypes.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {
using namespace MantidWidgets;

class IFitPlotPresenter;

class MANTIDQT_INELASTIC_DLL IFitPlotView {
public:
  virtual ~IFitPlotView() = default;

  virtual void subscribePresenter(IFitPlotPresenter *presenter) = 0;

  virtual void watchADS(bool watch) = 0;

  virtual WorkspaceIndex getSelectedSpectrum() const = 0;
  virtual WorkspaceID getSelectedDataIndex() const = 0;
  virtual WorkspaceID dataSelectionSize() const = 0;
  virtual bool isPlotGuessChecked() const = 0;

  virtual void setAvailableSpectra(WorkspaceIndex minimum, WorkspaceIndex maximum) = 0;
  virtual void setAvailableSpectra(const std::vector<WorkspaceIndex>::const_iterator &from,
                                   const std::vector<WorkspaceIndex>::const_iterator &to) = 0;

  virtual void setMinimumSpectrum(int minimum) = 0;
  virtual void setMaximumSpectrum(int maximum) = 0;
  virtual void setPlotSpectrum(WorkspaceIndex spectrum) = 0;
  virtual void appendToDataSelection(const std::string &dataName) = 0;
  virtual void setNameInDataSelection(const std::string &dataName, WorkspaceID workspaceID) = 0;
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
  virtual void setFitRangeBounds(std::pair<double, double> const &bounds) = 0;

  virtual void setBackgroundRangeVisible(bool visible) = 0;
  virtual void setHWHMRangeVisible(bool visible) = 0;

  virtual void displayMessage(const std::string &message) const = 0;

  virtual void allowRedraws(bool state) = 0;
  virtual void redrawPlots() = 0;

  virtual void setHWHMMinimum(double minimum) = 0;
  virtual void setHWHMMaximum(double maximum) = 0;
  virtual void setHWHMRange(double minimum, double maximum) = 0;

  virtual void clearPreviews() = 0;
};
} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
