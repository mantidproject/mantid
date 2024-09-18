// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "IFitScriptGeneratorView.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Common/FittingGlobals.h"
#include "MantidQtWidgets/Common/FittingMode.h"
#include "MantidQtWidgets/Common/FunctionModelSpectra.h"

#include <string>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {

using ViewEvent = IFitScriptGeneratorView::Event;

class EXPORT_OPT_MANTIDQT_COMMON IFitScriptGeneratorPresenter {
public:
  virtual ~IFitScriptGeneratorPresenter() = default;

  virtual void notifyPresenter(ViewEvent const &event, [[maybe_unused]] std::string const &arg1 = "",
                               [[maybe_unused]] std::string const &arg2 = "") = 0;
  virtual void notifyPresenter(ViewEvent const &event, std::vector<std::string> const &vec) = 0;
  virtual void notifyPresenter(ViewEvent const &event, FittingMode fittingMode) = 0;
  virtual void handleAddDomainAccepted(std::vector<Mantid::API::MatrixWorkspace_const_sptr> const &workspaces,
                                       FunctionModelSpectra const &workspaceIndices) = 0;

  virtual void openFitScriptGenerator() = 0;

  virtual void setGlobalTies(std::vector<MantidQt::MantidWidgets::GlobalTie> const &globalTies) = 0;
  virtual void setGlobalParameters(std::vector<MantidQt::MantidWidgets::GlobalParameter> const &globalParameters) = 0;
};

} // namespace MantidWidgets
} // namespace MantidQt
