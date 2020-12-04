// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "IFitScriptGeneratorView.h"

#include <string>

namespace MantidQt {
namespace MantidWidgets {

using ViewEvent = IFitScriptGeneratorView::Event;

class EXPORT_OPT_MANTIDQT_COMMON IFitScriptGeneratorPresenter {
public:
  virtual ~IFitScriptGeneratorPresenter() = default;

  virtual void notifyPresenter(ViewEvent const &event,
                               std::string const &arg = "") = 0;

  virtual void openFitScriptGenerator() = 0;
};

} // namespace MantidWidgets
} // namespace MantidQt
