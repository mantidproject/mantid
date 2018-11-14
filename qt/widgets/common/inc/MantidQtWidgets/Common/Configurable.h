// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_MANTIDWIDGETS_CONFIGURABLE_H
#define MANTIDQT_MANTIDWIDGETS_CONFIGURABLE_H

#include "MantidQtWidgets/Common/DllOption.h"

class QSettings;

namespace MantidQt {
namespace MantidWidgets {

/**
 * Defines an interface for an object to load and store
 * any configuration settings that should persist between objects. A widget
 * should inherit from this class and define the loadSettings and saveSettings
 * member functions. These functions are expected to be called by the client
 * along with a QSettings instance, opened at the correct group, which will
 * either give access or receive the values.
 */
class EXPORT_OPT_MANTIDQT_COMMON Configurable {
public:
  virtual ~Configurable() = default;
  virtual void readSettings(const QSettings &) = 0;
  virtual void writeSettings(QSettings &) const = 0;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDQT_MANTIDWIDGETS_CONFIGURABLE_H
