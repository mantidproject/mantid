#include "EnggDiffUserSettings.h"

namespace MantidQt {
namespace CustomInterfaces {

EnggDiffUserSettings::EnggDiffUserSettings(const std::string &instName)
    : m_instName(instName) {}

std::string EnggDiffUserSettings::getInstName() const { return m_instName; }

} // CustomInterfaces
} // MantidQt
