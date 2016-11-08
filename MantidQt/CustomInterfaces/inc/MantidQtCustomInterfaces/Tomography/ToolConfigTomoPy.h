#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOOLCONFIGTOMOPY_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOOLCONFIGTOMOPY_H_

#include "MantidQtCustomInterfaces/Tomography/TomoRecToolConfig.h"
#include "MantidQtCustomInterfaces/DllConfig.h"

namespace MantidQt {
namespace CustomInterfaces {

/**
Third party tomographic reconstruction tool configuration class
specialized for TomoPy (Python + C++):
https://www1.aps.anl.gov/Science/Scientific-Software/TomoPy

Copyright &copy; 2014,2015 ISIS Rutherford Appleton Laboratory, NScD
Oak Ridge National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTIDQT_CUSTOMINTERFACES_DLL ToolConfigTomoPy
    : public TomoRecToolConfig {
public:
  ToolConfigTomoPy();

  ToolConfigTomoPy(const std::string &runnable, const std::string &pathOut,
                   const std::string &pathDark, const std::string &pathOpen,
                   const std::string &pathSample);

  ~ToolConfigTomoPy() override {}

  // gives the list of methods (reconstruction algorithms) available
  static const std::vector<std::pair<std::string, std::string>> methods() {
    return g_tomopyMethods;
  }

protected:
  std::string makeCmdLineOptions() const override;

  std::string makeExecutable() const override { return m_runnable; };

private:
  static const std::vector<std::pair<std::string, std::string>> g_tomopyMethods;

  std::string m_pathOut;
  std::string m_pathDark;
  std::string m_pathOpen;
  std::string m_pathSample;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOOLCONFIGTOMOPY_H_
