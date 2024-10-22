// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include <QIcon>
#include <QMap>

namespace MantidQt {
namespace API {
/**
 * Defines a mapping between a workspace ID and a pixmap
 * to use for an icon.
 */
class EXPORT_OPT_MANTIDQT_COMMON WindowIcons {
public:
  WindowIcons();

  /// Returns an icon for the given ID
  QIcon getIcon(const std::string &windowID) const;
  /// Returns an icon ID for the given window ID
  std::string getIconID(const std::string &windowID) const;

private:
  /// Defines the mapping between ID & pixmap name
  void initInternalLookup();
  /// Build a icon object from an image file
  QIcon makeIconFromFile(const std::string &path) const;

  /// Internal map instance
  QMap<std::string, std::string> m_idToPixmapName;
};
} // namespace API
} // namespace MantidQt
