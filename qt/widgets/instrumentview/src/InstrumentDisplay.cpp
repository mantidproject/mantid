//+ Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0
#include "MantidQtWidgets/InstrumentView/InstrumentDisplay.h"

namespace MantidQt::MantidWidgets {

InstrumentDisplay::InstrumentDisplay(std::unique_ptr<IGLDisplay> glDisplay, std::unique_ptr<IQtDisplay> qtDisplay)
    : m_glDisplay(std::move(glDisplay)), m_qtDisplay(std::move(qtDisplay)) {}

IGLDisplay *InstrumentDisplay::getGLDisplay() const { return m_glDisplay.get(); }

IQtDisplay *InstrumentDisplay::getQtDisplay() const { return m_qtDisplay.get(); }
} // namespace MantidQt::MantidWidgets