#ifndef MANTIDQT_API_WORKSPACEICONS_H_
#define MANTIDQT_API_WORKSPACEICONS_H_

#include "DllOption.h"
#include <QMap>
#include <QPixmap>

namespace MantidQt {
namespace API {
/**
 * Defines a mapping between a workspace ID and a pixmap
 * to use for an icon.
 */
class EXPORT_OPT_MANTIDQT_COMMON WorkspaceIcons {
public:
  WorkspaceIcons();

  /// Returns an icon for the given ID
  QPixmap getIcon(const std::string &workspaceID) const;
  /// Returns an icon ID for the given workspace ID
  std::string getIconID(const std::string &workspaceID) const;

private:
  /// Defines the mapping between ID & pixmap name
  void initInternalLookup();

  /// Internal map instance
  QMap<std::string, std::string> m_idToPixmapName;
};
} // namespace API
} // namespace MantidQt
#endif // MANTIDQT_API_WORKSPACEICONS_H_
