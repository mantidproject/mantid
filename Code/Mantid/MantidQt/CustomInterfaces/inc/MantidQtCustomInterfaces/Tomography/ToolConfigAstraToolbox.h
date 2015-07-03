#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOOLCONFIGATRATOOLBOX_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOOLCONFIGATRATOOLBOX_H_

#include <string>

#include "MantidKernel/System.h"
#include "MantidQtCustomInterfaces/Tomography/TomoRecToolConfig.h"

namespace MantidQt {
namespace CustomInterfaces {

/**

Configuration of a third party tomographic reconstruction tool
specialized for the Astra Toolbox tomographic reconstruction tool
(C++, CUDA): Astra Toolbox <http://sourceforge.net/p/astra-toolbox/wiki/Home/

Copyright &copy; 2014,205 ISIS Rutherford Appleton Laboratory, NScD
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
class DLLExport ToolConfigAstraToolbox : public TomoRecToolConfig {
public:
  ToolConfigAstraToolbox() { }

  ToolConfigAstraToolbox(const std::string &runnable, double centerRot,
                         double angleMin, double angleMax,
                         const std::string &pathOut,
                         const std::string &pathDark,
                         const std::string &pathOpen,
                         const std::string &pathSample);

  ~ToolConfigAstraToolbox() { }

protected:
  virtual std::string makeCmdLineOptions() const;

  virtual std::string makeExecutable() const { return m_runnable; };

private:
  double m_centerRot;
  double m_angleMin;
  double m_angleMax;
  std::string m_pathOut;
  std::string m_pathDark;
  std::string m_pathOpen;
  std::string m_pathSample;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOOLCONFIGATRATOOLBOX_H_
