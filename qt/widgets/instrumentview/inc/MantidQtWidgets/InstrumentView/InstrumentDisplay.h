// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "IGLDisplay.h"
#include "IInstrumentDisplay.h"
#include "IQtDisplay.h"
#include "IStackedLayout.h"

#include <memory>
// Qt forward declarations
class QWidget;

namespace MantidQt::MantidWidgets {

class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW InstrumentDisplay : public IInstrumentDisplay {
public:
  InstrumentDisplay(QWidget *parent, std::unique_ptr<IGLDisplay> glDisplay = nullptr,
                    std::unique_ptr<IQtDisplay> qtDisplay = nullptr, std::unique_ptr<IStackedLayout> layout = nullptr);
  int currentIndex() const override;
  QWidget *currentWidget() const override;
  void setCurrentIndex(int val) const override;

  IGLDisplay *getGLDisplay() const override;
  IQtDisplay *getQtDisplay() const override;

  void installEventFilter(QObject *obj) override;

  ProjectionSurface_sptr getSurface() const override;
  void setSurface(ProjectionSurface_sptr surface) override;

  void updateView(bool picking) override;

private:
  std::unique_ptr<IGLDisplay> m_glDisplay;
  std::unique_ptr<IQtDisplay> m_qtDisplay;

  /// Stacked layout managing m_glDisplay and m_qtDisplay
  std::unique_ptr<IStackedLayout> m_instrumentDisplayLayout;
};
} // namespace MantidQt::MantidWidgets
