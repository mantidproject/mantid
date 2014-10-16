#ifndef _MANTIDSCRIPTREPOSITORY_PROXYOSX_H_
#define _MANTIDSCRIPTREPOSITORY_PROXYOSX_H_

#include <boost/tuple/tuple.hpp>
#include "MantidKernel/System.h"
#include "MantidKernel/Logger.h"
#include "MantidScriptRepository/ProxyInfo.h"

namespace Mantid
{
  namespace ScriptRepository
  {

    /** ProxyOSX : Utility for getting network proxy information off the OSX OS.

     Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    DLLExport class ProxyOSX
    {
    private:
      /// Logger object
      Mantid::Kernel::Logger m_logger;
    public:
      /// Constructor
      ProxyOSX();
      /// Look for http network proxy settings
      ProxyInfo getHttpProxy(const std::string& targetURLString);
    };

  }
}

#endif /* _MANTIDSCRIPTREPOSITORY_PROXYOSX_H_ */
