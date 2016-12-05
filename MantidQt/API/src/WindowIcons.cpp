//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidQtAPI/WindowIcons.h"
#include "MantidQtAPI/pixmaps.h"

namespace MantidQt {
namespace API {

//-----------------------------------------------------------------------------
// Public member functions
//-----------------------------------------------------------------------------
/**
 * Default constructor
 */
WindowIcons::WindowIcons() : m_idToPixmapName() { initInternalLookup(); }

/**
 * @param windowID A string giving the ID for a window
 * @throws std::runtime_error if no icon can be found
 */
QPixmap WindowIcons::getIcon(const std::string &windowID) const {
  return getQPixmap(m_idToPixmapName.value(windowID));
}

/**
 * Get the string ID representing the icon
 * @param windowID :: a string representing the ID of the window
 * @return a string with the QPixmap id for this window.
 */
std::string WindowIcons::getIconID(const std::string &windowID) const
{
  if(m_idToPixmapName.contains(windowID))
    return m_idToPixmapName.value(windowID);
  else
    return "";
}

//-----------------------------------------------------------------------------
// Private member functions
//-----------------------------------------------------------------------------
/**
 */
void WindowIcons::initInternalLookup() {
  m_idToPixmapName.clear();
  m_idToPixmapName["Matrix"] = "mantid_matrix_xpm";
  m_idToPixmapName["Table"] = "worksheet_xpm";
  m_idToPixmapName["Note"] = "note_xpm";
  m_idToPixmapName["Graph"] = "graph_xpm";
  m_idToPixmapName["3D Graph"] = "trajectory_xpm";
  m_idToPixmapName["Workspace"] = "mantid_matrix_xpm";
}
}
}
