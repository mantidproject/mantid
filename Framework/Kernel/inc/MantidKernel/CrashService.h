#ifndef MANTID_KERNEL_CRASHSERVICE_H_
#define MANTID_KERNEL_CRASHSERVICE_H_

#include <string>
#include <Poco/ActiveMethod.h>

#include "MantidKernel/DllConfig.h"

namespace Mantid {
namespace Kernel {

/** CrashReporter : The Crash reporter is responsible for sending crash reports

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

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

class MANTID_KERNEL_DLL CrashServiceImpl {  
    public:
    CrashServiceImpl(std::string application);
    CrashServiceImpl(std::string application, std::string startTime);
    void sendCrashReport();

    protected:
    virtual std::string generateCrashMessage();
    virtual int sendReport(const std::string &message,
                                 const std::string &url);
    
    private:
    int sendCrashAsyncImpl(const std::string &message);
    const std::string m_application;
    const std::string m_startTime;
    /// Async method for sending startup notifications
    Poco::ActiveMethod<int, std::string, CrashServiceImpl> m_crashActiveMethod;
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_CRASHSERVICE_H_ */