// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "IFitScriptGeneratorView.h"
#include "MantidQtWidgets/Common/FittingMode.h"

#include <string>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {

using ViewEvent = IFitScriptGeneratorView::Event;

struct GlobalParameter;
struct GlobalTie;

class EXPORT_OPT_MANTIDQT_COMMON IFitScriptGeneratorPresenter {
public:
  virtual ~IFitScriptGeneratorPresenter() = default;

  virtual void notifyPresenter(ViewEvent const &event,
                               std::string const &arg1 = "",
                               std::string const &arg2 = "") = 0;
  virtual void notifyPresenter(ViewEvent const &event,
                               std::vector<std::string> const &vec) = 0;
  virtual void notifyPresenter(ViewEvent const &event,
                               FittingMode fittingMode) = 0;

  virtual void openFitScriptGenerator() = 0;

  virtual void setGlobalTies(std::vector<GlobalTie> const &globalTies) = 0;
  virtual void
  setGlobalParameters(std::vector<GlobalParameter> const &globalParameters) = 0;
};

} // namespace MantidWidgets
} // namespace MantidQt
