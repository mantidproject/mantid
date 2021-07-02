// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "IGLDisplay.h"
#include "IQtDisplay.h"

#include <memory>

namespace MantidQt::MantidWidgets {

class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW InstrumentDisplay {
public:
  InstrumentDisplay(std::unique_ptr<IGLDisplay> glDisplay, std::unique_ptr<IQtDisplay> qtDisplay);

  IGLDisplay *getGLDisplay() const;
  IQtDisplay *getQtDisplay() const;

  void installEventFilter(QObject *obj);

private:
  std::unique_ptr<IGLDisplay> m_glDisplay;
  std::unique_ptr<IQtDisplay> m_qtDisplay;
};
} // namespace MantidQt::MantidWidgets