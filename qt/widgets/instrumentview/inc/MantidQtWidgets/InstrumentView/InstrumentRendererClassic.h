// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/InstrumentView/InstrumentRenderer.h"
#include <array>

namespace MantidQt::MantidWidgets {
class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW InstrumentRendererClassic : public InstrumentRenderer {
public:
  InstrumentRendererClassic(const InstrumentActor &actor);
  ~InstrumentRendererClassic() override;
  void renderInstrument(const std::vector<bool> &visibleComps, bool showGuides, bool picking = false) override;

protected:
  void draw(const std::vector<bool> &visibleComps, bool showGuides, bool picking) override;
  void resetDisplayLists() override;

private:
  std::array<GLuint, 2> m_displayListId;
  std::array<bool, 2> m_useDisplayList;
};
} // namespace MantidQt::MantidWidgets
