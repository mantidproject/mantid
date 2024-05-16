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

class OutputPlotOptionsView;
class IIqtPresenter;

class MANTIDQT_INELASTIC_DLL IIqtView {

public:
  virtual ~IIqtView() = default;
  virtual void subscribePresenter(IIqtPresenter *presenter) = 0;

  virtual OutputPlotOptionsView *getPlotOptions() const = 0;
  virtual void plotInput(Mantid::API::MatrixWorkspace_sptr inputWS, int spectrum) = 0;
  virtual void setPreviewSpectrumMaximum(int value) = 0;
  virtual void updateDisplayedBinParameters() = 0;
  virtual void setRangeSelectorDefault(const Mantid::API::MatrixWorkspace_sptr inputWorkspace,
                                       const QPair<double, double> &range) = 0;
  virtual bool validate() = 0;
  virtual void setSampleFBSuffixes(const QStringList &suffix) = 0;
  virtual void setSampleWSSuffixes(const QStringList &suffix) = 0;
  virtual void setResolutionFBSuffixes(const QStringList &suffix) = 0;
  virtual void setResolutionWSSuffixes(const QStringList &suffix) = 0;
  virtual void setRunEnabled(bool enabled) = 0;
  virtual void setSaveResultEnabled(bool enabled) = 0;
  virtual void setRunText(bool running) = 0;
  virtual void setWatchADS(bool watch) = 0;
  virtual void setup() = 0;
  virtual void showMessageBox(const std::string &message) const = 0;
  virtual std::string getSampleName() const = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt