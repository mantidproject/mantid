#ifndef WORKSPACEICONS_H_
#define WORKSPACEICONS_H_

#include <QMap>
#include <QPixmap>

/**
 * Defines a mapping between a workspace ID and a pixmap
 * to use for an icon.
 */
class WorkspaceIcons
{
public:
  WorkspaceIcons();

  /// Returns an icon for the given ID
  QPixmap getIcon(const std::string & workspaceID) const;

private:
  /// Defines the mapping between ID & pixmap name
  void initInternalLookup();

  /// Internal map instance
  QMap<std::string, std::string> m_idToPixmapName;
};

#endif
