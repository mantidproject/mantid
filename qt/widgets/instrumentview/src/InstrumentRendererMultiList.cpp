// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/InstrumentView/InstrumentRendererMultiList.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidQtWidgets/InstrumentView/InstrumentActor.h"
#include "MantidQtWidgets/InstrumentView/OpenGLError.h"

#include <algorithm>

using Mantid::Beamline::ComponentType;

namespace MantidQt::MantidWidgets {
InstrumentRendererMultiList::InstrumentRendererMultiList(const InstrumentActor &actor) : InstrumentRenderer(actor) {
  const auto &componentInfo = actor.componentInfo();

  m_useNonPickingDisplayList = false;
  m_usePickingDisplayList = false;

  m_pickingDisplayListId.resize(componentInfo.size());
  m_nonPickingDisplayListId.resize(componentInfo.size());
  std::fill(m_nonPickingDisplayListId.begin(), m_nonPickingDisplayListId.end(), 0);
  std::fill(m_pickingDisplayListId.begin(), m_pickingDisplayListId.end(), 0);
}

InstrumentRendererMultiList::~InstrumentRendererMultiList() {
  for (unsigned int i : m_nonPickingDisplayListId) {
    if (i != 0) {
      glDeleteLists(i, 1);
    }
  }
  for (unsigned int i : m_pickingDisplayListId) {
    if (i != 0) {
      glDeleteLists(i, 1);
    }
  }
}

void InstrumentRendererMultiList::renderInstrument(const std::vector<bool> &visibleComps, bool showGuides,
                                                   bool picking) {
  if (std::none_of(visibleComps.cbegin(), visibleComps.cend(), [](bool visible) { return visible; }))
    return;

  if (!instrActor().isInitialized()) {
    return;
  }

  OpenGLError::check("InstrumentActor::draw()");
  const auto &displayListIds = picking ? m_pickingDisplayListId : m_nonPickingDisplayListId;
  auto &useDisplayList = picking ? m_usePickingDisplayList : m_useNonPickingDisplayList;
  if (useDisplayList) {
    for (size_t componentIndex = 0; componentIndex < instrActor().componentInfo().size(); ++componentIndex) {
      glCallList(displayListIds[componentIndex]);
    }
  } else {
    useDisplayList = true;
    draw(visibleComps, showGuides, picking);
  }
  OpenGLError::check("InstrumentActor::draw()");
}

void InstrumentRendererMultiList::draw(const std::vector<bool> &visibleComps, bool showGuides, bool picking) {
  const auto &compInfo = instrActor().componentInfo();
  std::vector<bool> visited(compInfo.size(), false);

  auto &displayListIds = picking ? m_pickingDisplayListId : m_nonPickingDisplayListId;
  for (size_t componentIndex = compInfo.root(); componentIndex != std::numeric_limits<size_t>::max();
       --componentIndex) {
    displayListIds[componentIndex] = glGenLists(1);
    glNewList(displayListIds[componentIndex],
              GL_COMPILE); // Construct display list for object representation
    drawComponent(componentIndex, visibleComps, showGuides, picking, compInfo, visited);
    glEndList();
    if (glGetError() == GL_OUT_OF_MEMORY)
      throw Mantid::Kernel::Exception::OpenGLError("OpenGL: Out of video memory");
    glCallList(displayListIds[componentIndex]);
  }
}

void InstrumentRendererMultiList::resetDisplayLists() {
  invalidateAndDeleteDisplayList(m_pickingDisplayListId, m_usePickingDisplayList);
  invalidateAndDeleteDisplayList(m_nonPickingDisplayListId, m_useNonPickingDisplayList);
}

} // namespace MantidQt::MantidWidgets
