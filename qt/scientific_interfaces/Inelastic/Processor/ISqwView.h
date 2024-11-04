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

class IRunView;
class IOutputPlotOptionsView;
class ISqwPresenter;

class MANTIDQT_INELASTIC_DLL ISqwView {

public:
  virtual ~ISqwView() = default;
  virtual void subscribePresenter(ISqwPresenter *presenter) = 0;

  virtual IRunView *getRunView() const = 0;
  virtual IOutputPlotOptionsView *getPlotOptions() const = 0;

  virtual void setFBSuffixes(QStringList const &suffix) = 0;
  virtual void setWSSuffixes(QStringList const &suffix) = 0;
  virtual void setLoadHistory(bool doLoadHistory) = 0;
  virtual std::tuple<double, double> getQRangeFromPlot() const = 0;
  virtual std::tuple<double, double> getERangeFromPlot() const = 0;
  virtual std::string getDataName() const = 0;
  virtual void plotRqwContour(Mantid::API::MatrixWorkspace_sptr rqwWorkspace) = 0;
  virtual void setDefaultQAndEnergy() = 0;
  virtual bool validate() = 0;
  virtual void showMessageBox(std::string const &message) const = 0;
  virtual void setEnableOutputOptions(bool const enable) = 0;
};
} // namespace CustomInterfaces
} // namespace MantidQt
