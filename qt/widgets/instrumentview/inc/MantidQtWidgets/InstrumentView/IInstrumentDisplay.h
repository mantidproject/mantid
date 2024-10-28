// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/InstrumentView/IGLDisplay.h"
#include "MantidQtWidgets/InstrumentView/IQtDisplay.h"
#include "MantidQtWidgets/InstrumentView/IStackedLayout.h"
#include "MantidQtWidgets/InstrumentView/ProjectionSurface.h"

#include <memory>

// Qt Forward Declarations
class QStackedLayout;

namespace MantidQt::MantidWidgets {

class IInstrumentDisplay {
public:
  virtual ~IInstrumentDisplay() = default;

  virtual int currentIndex() const = 0;
  virtual QWidget *currentWidget() const = 0;
  virtual void setCurrentIndex(int val) const = 0;

  virtual IGLDisplay *getGLDisplay() const = 0;
  virtual IQtDisplay *getQtDisplay() const = 0;

  virtual void installEventFilter(QObject *obj) = 0;

  virtual ProjectionSurface_sptr getSurface() const = 0;
  virtual void setSurface(ProjectionSurface_sptr surface) = 0;

  virtual void updateView(bool picking) = 0;
};
} // namespace MantidQt::MantidWidgets
