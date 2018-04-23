#ifndef MANTIDQT_CUSTOMINTERFACES_ENGGDIFFUSERSETTINGS_H_
#define MANTIDQT_CUSTOMINTERFACES_ENGGDIFFUSERSETTINGS_H_

#include "DllConfig.h"

#include <string>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ENGGDIFFRACTION_DLL EnggDiffUserSettings {
public:
  EnggDiffUserSettings(const std::string &instName);

  std::string getInstName() const;

private:
  std::string m_instName;
};

} // CustomInterfaces
} // MantidQt

#endif // MANTIDQT_CUSTOMINTERFACES_ENGGDIFFUSERSETTINGS_H_
