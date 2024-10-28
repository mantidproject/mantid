// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/InstrumentView/InstrumentRendererClassic.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidQtWidgets/InstrumentView/InstrumentActor.h"
#include "MantidQtWidgets/InstrumentView/OpenGLError.h"

#include <algorithm>

using Mantid::Beamline::ComponentType;

namespace MantidQt::MantidWidgets {
InstrumentRendererClassic::InstrumentRendererClassic(const InstrumentActor &actor) : InstrumentRenderer(actor) {
  m_displayListId = {0, 0};
  m_useDisplayList = {false, false};
}

InstrumentRendererClassic::~InstrumentRendererClassic() {
  for (unsigned int i : m_displayListId) {
    if (i != 0) {
      glDeleteLists(i, 1);
    }
  }
}

void InstrumentRendererClassic::renderInstrument(const std::vector<bool> &visibleComps, bool showGuides, bool picking) {
  if (std::none_of(visibleComps.cbegin(), visibleComps.cend(), [](bool visible) { return visible; }))
    return;
  if (!instrActor().isInitialized()) {
    return;
  }

  OpenGLError::check("InstrumentActor::draw()");
  size_t i = picking ? 1 : 0;
  if (m_useDisplayList[i]) {
    glCallList(m_displayListId[i]);
  } else {
    m_displayListId[i] = glGenLists(1);
    m_useDisplayList[i] = true;
    glNewList(m_displayListId[i],
              GL_COMPILE); // Construct display list for object representation
    draw(visibleComps, showGuides, picking);
    glEndList();
    if (glGetError() == GL_OUT_OF_MEMORY) // Throw an exception
      throw Mantid::Kernel::Exception::OpenGLError("OpenGL: Out of video memory");
    glCallList(m_displayListId[i]);
  }
  OpenGLError::check("InstrumentActor::draw()");
}

void InstrumentRendererClassic::draw(const std::vector<bool> &visibleComps, bool showGuides, bool picking) {
  const auto &compInfo = instrActor().componentInfo();
  std::vector<bool> visited(compInfo.size(), false);

  for (size_t i = compInfo.root(); i != std::numeric_limits<size_t>::max(); --i) {
    drawComponent(i, visibleComps, showGuides, picking, compInfo, visited);
  }
}

void InstrumentRendererClassic::resetDisplayLists() {
  for (size_t i = 0; i < 2; ++i) {
    if (m_displayListId[i] != 0) {
      glDeleteLists(m_displayListId[i], 1);
      m_displayListId[i] = 0;
      m_useDisplayList[i] = false;
    }
  }
}
} // namespace MantidQt::MantidWidgets
