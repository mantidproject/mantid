//+ Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0
#include "MantidQtWidgets/InstrumentView/InstrumentDisplay.h"
#include "MantidQtWidgets/InstrumentView/GLDisplay.h"
#include "MantidQtWidgets/InstrumentView/QtDisplay.h"

#include <QStackedLayout>

namespace MantidQt::MantidWidgets {

InstrumentDisplay::InstrumentDisplay(std::unique_ptr<IGLDisplay> glDisplay, std::unique_ptr<IQtDisplay> qtDisplay,
                                     QWidget *parent)
    : m_glDisplay(std::move(glDisplay)), m_qtDisplay(std::move(qtDisplay)) {
  if (!m_glDisplay)
    m_glDisplay = std::make_unique<GLDisplay>();

  if (!m_qtDisplay)
    m_qtDisplay = std::make_unique<QtDisplay>();

  m_instrumentDisplayLayout = new QStackedLayout(parent);
  m_instrumentDisplayLayout->addWidget(getGLDisplay());
  m_instrumentDisplayLayout->addWidget(getQtDisplay());
}

int InstrumentDisplay::currentIndex() const { return m_instrumentDisplayLayout->currentIndex(); }

QWidget *InstrumentDisplay::currentWidget() const { return m_instrumentDisplayLayout->currentWidget(); }

void InstrumentDisplay::setCurrentIndex(int val) const { m_instrumentDisplayLayout->setCurrentIndex(val); }

IGLDisplay *InstrumentDisplay::getGLDisplay() const { return m_glDisplay.get(); }

IQtDisplay *InstrumentDisplay::getQtDisplay() const { return m_qtDisplay.get(); }

void InstrumentDisplay::installEventFilter(QObject *obj) {
  m_glDisplay->qtInstallEventFilter(obj);
  m_qtDisplay->qtInstallEventFilter(obj);
}
} // namespace MantidQt::MantidWidgets