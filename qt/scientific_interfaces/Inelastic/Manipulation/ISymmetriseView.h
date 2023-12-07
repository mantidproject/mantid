// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <QStringList>

#include <memory>
#include <string>
#include <tuple>

namespace MantidQt {
namespace CustomInterfaces {

class IndirectPlotOptionsView;
class ISymmetrisePresenter;

class MANTIDQT_INELASTIC_DLL ISymmetriseView {

public:
  virtual void subscribePresenter(ISymmetrisePresenter *presenter) = 0;

  virtual void setDefaults() = 0;
  virtual IndirectPlotOptionsView *getPlotOptions() const = 0;
  virtual void setFBSuffixes(QStringList const &suffix) = 0;
  virtual void setWSSuffixes(QStringList const &suffix) = 0;

  virtual double getElow() const = 0;
  virtual double getEhigh() const = 0;
  virtual double getPreviewSpec() = 0;
  virtual std::string getDataName() const = 0;

  virtual void plotNewData(std::string const &workspaceName) = 0;
  virtual void updateMiniPlots() = 0;
  virtual void replotNewSpectrum(double value) = 0;
  virtual void updateRangeSelectors(std::string const &propName, double value) = 0;
  virtual bool verifyERange(std::string const &workspaceName) = 0;
  virtual void setRawPlotWatchADS(bool watchADS) = 0;

  virtual void previewAlgDone() = 0;
  virtual void enableSave(bool save) = 0;
  virtual void enableRun(bool run) = 0;
  virtual bool validate() = 0;
  virtual void showMessageBox(std::string const &message) const = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt