//+ Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0
#include "MantidQtWidgets/InstrumentView/InstrumentDisplay.h"
#include "MantidQtWidgets/InstrumentView/GLDisplay.h"
#include "MantidQtWidgets/InstrumentView/IStackedLayout.h"
#include "MantidQtWidgets/InstrumentView/ProjectionSurface.h"
#include "MantidQtWidgets/InstrumentView/QtDisplay.h"
#include "MantidQtWidgets/InstrumentView/StackedLayout.h"

class QObject;
class QWidget;

namespace MantidQt::MantidWidgets {

InstrumentDisplay::InstrumentDisplay(QWidget *parent, std::unique_ptr<IGLDisplay> glDisplay,
                                     std::unique_ptr<IQtDisplay> qtDisplay, std::unique_ptr<IStackedLayout> layout)
    : m_glDisplay(std::move(glDisplay)), m_qtDisplay(std::move(qtDisplay)),
      m_instrumentDisplayLayout(std::move(layout)) {
  if (!m_glDisplay)
    m_glDisplay = std::make_unique<GLDisplay>();

  if (!m_qtDisplay)
    m_qtDisplay = std::make_unique<QtDisplay>();

  if (!m_instrumentDisplayLayout)
    m_instrumentDisplayLayout = std::make_unique<StackedLayout>(parent);
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

void InstrumentDisplay::setSurface(ProjectionSurface *surface) {
  ProjectionSurface_sptr sharedSurface(surface);
  if (m_glDisplay) {
    m_glDisplay->setSurface(sharedSurface);
    m_glDisplay->qtUpdate();
  }
  if (getQtDisplay()) {
    m_qtDisplay->setSurface(sharedSurface);
    m_qtDisplay->qtUpdate();
  }
}
} // namespace MantidQt::MantidWidgets