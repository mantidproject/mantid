#ifndef MANTID_DATAHANDLING_PROCESSDASNEXUSLOG_H_
#define MANTID_DATAHANDLING_PROCESSDASNEXUSLOG_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"

namespace Mantid
{
namespace DataHandling
{

  /** ProcessDasNexusLog : TODO: DESCRIPTION
    
    @date 2012-01-23

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class DLLExport ProcessDasNexusLog : public API::Algorithm, public API::DeprecatedAlgorithm
  {
  public:
    ProcessDasNexusLog();
    virtual ~ProcessDasNexusLog();
    

    virtual const std::string name() const {return "ProcessDasNexusLog"; };
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Very specialized algorithm to fix certain SNS DAS logs that cannot be used directly.";}

    virtual int version() const {return 1; };
    virtual const std::string category() const {return "DataHandling"; };

  private:
    void init();
    void exec();

    void convertToAbsoluteTime(API::MatrixWorkspace_sptr ws, std::string logname, std::vector<Kernel::DateAndTime>& abstimeve,
        std::vector<double>& orderedtofs);

    void writeLogtoFile(API::MatrixWorkspace_sptr ws, std::string logname, size_t numentriesoutput, std::string outputfilename);

    void addLog(API::MatrixWorkspace_sptr ws, std::vector<Kernel::DateAndTime> timevec,
          double unifylogvalue, std::string logname, std::vector<Kernel::DateAndTime> pulsetimes,
          std::vector<double> orderedtofs, bool);

    void checkLog(API::MatrixWorkspace_sptr ws, std::string logname);

    void calDistributions(std::vector<Kernel::DateAndTime>, double dts);

    void exportErrorLog(API::MatrixWorkspace_sptr ws, std::vector<Kernel::DateAndTime> abstimevec,
        std::vector<Kernel::DateAndTime> pulsetimes, std::vector<double>orderedtofs, double dts);
  };


} // namespace DataHandling
} // namespace Mantid

#endif  /* MANTID_DATAHANDLING_PROCESSDASNEXUSLOG_H_ */
