// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectSettingsHelper.h"
#include "MantidQtWidgets/Common/QSettingsHelper.h"

#include <QSettings>
#include <QString>
#include <QVariant>

namespace MantidQt {
namespace CustomInterfaces {
namespace IndirectSettingsHelper {

using namespace MantidQt::MantidWidgets::QSettingsHelper;

static std::string const INDIRECT_SETTINGS_GROUP("Indirect Settings");
static std::string const RESTRICT_DATA_PROPERTY("restrict-input-by-name");
static std::string const ERROR_BARS_PROPERTY("plot-error-bars-external");

bool restrictInputDataByName() {
  return getSetting<bool>(INDIRECT_SETTINGS_GROUP, RESTRICT_DATA_PROPERTY);
}

bool externalPlotErrorBars() {
  return getSetting<bool>(INDIRECT_SETTINGS_GROUP, ERROR_BARS_PROPERTY);
}

void setRestrictInputDataByName(bool restricted) {
  setSetting<bool>(INDIRECT_SETTINGS_GROUP, RESTRICT_DATA_PROPERTY, restricted);
}

void setExternalPlotErrorBars(bool errorBars) {
  setSetting<bool>(INDIRECT_SETTINGS_GROUP, ERROR_BARS_PROPERTY, errorBars);
}

} // namespace IndirectSettingsHelper
} // namespace CustomInterfaces
} // namespace MantidQt
