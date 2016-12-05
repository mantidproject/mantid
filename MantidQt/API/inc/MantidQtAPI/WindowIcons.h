#ifndef MANTIDQT_API_WINDOWICONS_H_
#define MANTIDQT_API_WINDOWICONS_H_

#include "DllOption.h"
#include <QMap>
#include <QPixmap>

namespace MantidQt {
namespace API {
/**
 * Defines a mapping between a workspace ID and a pixmap
 * to use for an icon.
 */
class EXPORT_OPT_MANTIDQT_API WindowIcons {
public:
  WindowIcons();

  /// Returns an icon for the given ID
  QPixmap getIcon(const std::string &windowID) const;
  /// Returns an icon ID for the given window ID
  std::string getIconID(const std::string &windowID) const;

private:
  /// Defines the mapping between ID & pixmap name
  void initInternalLookup();

  /// Internal map instance
  QMap<std::string, std::string> m_idToPixmapName;
};
}
}
#endif // MANTIDQT_API_WINDOWICONS_H_
