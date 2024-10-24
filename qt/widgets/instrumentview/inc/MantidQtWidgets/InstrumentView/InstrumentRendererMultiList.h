// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/InstrumentView/InstrumentRenderer.h"

namespace MantidQt::MantidWidgets {
/*
This class will use a separate OpenGL display list for drawing each instrument component. By doing
this we reduce the chance that we run into a specific memory allocation bug in the Mesa
graphics library on Linux, which was fixed in Mesa version 24.1. The original method was to put
all the OpenGL commands in one big display list.
*/
class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW InstrumentRendererMultiList : public InstrumentRenderer {
public:
  InstrumentRendererMultiList(const InstrumentActor &actor);
  ~InstrumentRendererMultiList() override;
  void renderInstrument(const std::vector<bool> &visibleComps, bool showGuides, bool picking = false) override;

protected:
  void draw(const std::vector<bool> &visibleComps, bool showGuides, bool picking) override;
  void resetDisplayLists() override;

private:
  std::vector<GLuint> m_nonPickingDisplayListId;
  std::vector<GLuint> m_pickingDisplayListId;
  bool m_usePickingDisplayList;
  bool m_useNonPickingDisplayList;
};
} // namespace MantidQt::MantidWidgets
