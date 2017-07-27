//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidQtAPI/WorkspaceIcons.h"
#include "MantidQtAPI/pixmaps.h"

namespace MantidQt {
namespace API {
//-----------------------------------------------------------------------------
// Public member functions
//-----------------------------------------------------------------------------
/**
 * Default constructor
 */
WorkspaceIcons::WorkspaceIcons() : m_idToPixmapName() { initInternalLookup(); }

/**
 * @param workspaceID A string giving the ID for a workspace
 * @throws std::runtime_error if no icon can be found
 */
QPixmap WorkspaceIcons::getIcon(const std::string &workspaceID) const {
  // All mappings are simple bar the MDEventWorkspace as each of its templates
  // has different ID.
  if (workspaceID.compare(0, 16, "MDEventWorkspace") == 0)
    return getQPixmap(m_idToPixmapName.value("MDEventWorkspace"));
  else
    return getQPixmap(m_idToPixmapName.value(workspaceID));
}

/**
 * Get the string ID representing the icon
 * @param workspaceID :: a string representing the ID of the workspace
 * @return a string with the QPixmap id for this workspace.
 */
std::string WorkspaceIcons::getIconID(const std::string &workspaceID) const {
  if (workspaceID.compare(0, 16, "MDEventWorkspace") == 0)
    return m_idToPixmapName.value("MDEventWorkspace");
  else
    return m_idToPixmapName.value(workspaceID);
}

//-----------------------------------------------------------------------------
// Private member functions
//-----------------------------------------------------------------------------
/**
 */
void WorkspaceIcons::initInternalLookup() {
  m_idToPixmapName.clear();
  // MatrixWorkspace types
  m_idToPixmapName["EventWorkspace"] = "mantid_matrix_xpm";
  m_idToPixmapName["GroupingWorkspace"] = "mantid_matrix_xpm";
  m_idToPixmapName["MaskWorkspace"] = "mantid_matrix_xpm";
  m_idToPixmapName["OffsetsWorkspace"] = "mantid_matrix_xpm";
  m_idToPixmapName["RebinnedOutput"] = "mantid_matrix_xpm";
  m_idToPixmapName["SpecialWorkspace2D"] = "mantid_matrix_xpm";
  m_idToPixmapName["Workspace2D"] = "mantid_matrix_xpm";
  m_idToPixmapName["WorkspaceSingleValue"] = "mantid_matrix_xpm";

  // Table workspace types
  m_idToPixmapName["TableWorkspace"] = "worksheet_xpm";
  m_idToPixmapName["PeaksWorkspace"] = "worksheet_xpm";

  // Group
  m_idToPixmapName["WorkspaceGroup"] = "mantid_wsgroup_xpm";

  // MD
  m_idToPixmapName["MDHistoWorkspace"] = "mantid_mdws_xpm";
  m_idToPixmapName["MDEventWorkspace"] = "mantid_mdws_xpm";
}
}
}
