// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidAPI/IFunction.h"
#include <QObject>

namespace MantidQt {
namespace MantidWidgets {

/** IMuonFitFunctionModel: set function to fit for a muon fit property browser

  Abstract base class to be implemented
*/
class EXPORT_OPT_MANTIDQT_COMMON IMuonFitFunctionModel {
public:
  virtual ~IMuonFitFunctionModel() {}
  virtual void setFunction(const Mantid::API::IFunction_sptr func) = 0;
  virtual void runFit() = 0;
  virtual void runSequentialFit() = 0;
  virtual Mantid::API::IFunction_sptr getFunction() const = 0;
  virtual std::vector<std::string> getWorkspaceNamesToFit() const = 0;
  virtual void setMultiFittingMode(bool enabled) = 0;
  virtual bool isMultiFittingMode() const = 0;
  virtual void doRemoveGuess() = 0;
  virtual void doPlotGuess() = 0;
  virtual bool hasGuess() const = 0;
signals:
  virtual void functionUpdateRequested() = 0;
  virtual void functionUpdateAndFitRequested(bool sequential) = 0;
  virtual void userChangedDatasetIndex(int index) = 0;
  virtual void fitRawDataClicked(bool enabled) = 0;
};

} // namespace MantidWidgets
} // namespace MantidQt
