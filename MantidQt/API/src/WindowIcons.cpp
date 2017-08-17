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
QIcon WindowIcons::getIcon(const std::string &windowID) const {
  auto value = m_idToPixmapName.value(windowID);
  if (QString::fromStdString(value).endsWith(".png")) {
    return makeIconFromFile(value);
  } else {
    return getQPixmap(value);
  }
}

/**
 * Get the string ID representing the icon
 * @param windowID :: a string representing the ID of the window
 * @return a string with the QPixmap id for this window.
 */
std::string WindowIcons::getIconID(const std::string &windowID) const {
  if (m_idToPixmapName.contains(windowID))
    return m_idToPixmapName.value(windowID);
  else
    return "";
}

//-----------------------------------------------------------------------------
// Private member functions
//-----------------------------------------------------------------------------

/**
 * Initilise the internal lookup map
 */
void WindowIcons::initInternalLookup() {
  m_idToPixmapName.clear();
  m_idToPixmapName["Matrix"] = "matrix_xpm";
  m_idToPixmapName["MantidMatrix"] = "mantid_matrix_xpm";
  m_idToPixmapName["Table"] = "worksheet_xpm";
  m_idToPixmapName["Note"] = "note_xpm";
  m_idToPixmapName["MultiLayer"] = "graph_xpm";
  m_idToPixmapName["Graph3D"] = "trajectory_xpm";
  m_idToPixmapName["Graph"] = "graph_xpm";
  m_idToPixmapName["Workspace"] = "mantid_matrix_xpm";
  m_idToPixmapName["SliceViewer"] =
      ":/SliceViewer/icons/SliceViewerWindow_icon.png";
  m_idToPixmapName["VSIWindow"] =
      ":/VatesSimpleGuiViewWidgets/icons/pvIcon.png";
}

/**
 * Make a QIcon object froma file path
 *
 * This is used in the case that a pixmap does not exist and a file path to an
 * image was provided instead.
 *
 * @param path :: path to the image to use to create the icon
 * @return icon object made from the path
 */
QIcon WindowIcons::makeIconFromFile(const std::string &path) const {
  QIcon icon;
  icon.addFile(QString::fromStdString(path), QSize(), QIcon::Normal,
               QIcon::Off);
  return icon;
}
}
}
