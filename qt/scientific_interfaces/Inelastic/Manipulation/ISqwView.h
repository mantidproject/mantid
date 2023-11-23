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
class ISqwPresenter;

class MANTIDQT_INELASTIC_DLL ISqwView {

public:
  virtual void subscribePresenter(ISqwPresenter *presenter) = 0;

  virtual IndirectPlotOptionsView *getPlotOptions() = 0;
  virtual void setFBSuffixes(QStringList suffix) = 0;
  virtual void setWSSuffixes(QStringList suffix) = 0;
  virtual std::tuple<double, double> getQRangeFromPlot() = 0;
  virtual std::tuple<double, double> getERangeFromPlot() = 0;
  virtual std::string getDataName() = 0;
  virtual void plotRqwContour(Mantid::API::MatrixWorkspace_sptr rqwWorkspace) = 0;
  virtual void setDefaultQAndEnergy() = 0;
  virtual void setSaveEnabled(bool enabled) = 0;
  virtual bool validate() = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt