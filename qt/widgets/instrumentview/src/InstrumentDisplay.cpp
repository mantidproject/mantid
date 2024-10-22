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

#include <cassert>

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

ProjectionSurface_sptr InstrumentDisplay::getSurface() const {
  assert(m_glDisplay);
  return m_glDisplay->getSurface();
}

void InstrumentDisplay::setSurface(ProjectionSurface_sptr surface) {
  assert(m_glDisplay);
  m_glDisplay->setSurface(surface);
  m_glDisplay->qtUpdate();

  assert(m_qtDisplay);
  m_qtDisplay->setSurface(surface);
  m_qtDisplay->qtUpdate();
}

void InstrumentDisplay::updateView(bool picking) {
  assert(m_glDisplay);
  if (currentWidget() == dynamic_cast<QWidget *>(m_glDisplay.get())) {
    m_glDisplay->updateView(picking);
  } else {
    m_qtDisplay->updateView(picking);
  }
}

} // namespace MantidQt::MantidWidgets
