#ifndef MANTID_API_LOGPARSER_H_
#define MANTID_API_LOGPARSER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidKernel/Logger.h"
#include <boost/shared_ptr.hpp>
#include <map>
#include <vector>
#include <ctime>
#include <cmath>

#ifdef WIN32
#define isNaN(x) _isnan(x)
#else
#define isNaN(x) isnan(x)
#endif

namespace Mantid
{

namespace Kernel
{
    class Property;
}

namespace API
{
/** @class LogParser LogParser.h API/LogParser.h

LogParser parses the instrument log files to select records corresponding
to 'RUNNING' instrument status. It determines the values of the logged variables
at the beginning and the end of each RUNNING interval and keeps track of changes
within the interval.

@author Roman Tolchenov, Tessella Support Services plc
@date 19/01/2009

Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratories

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

File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>. 
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

//#ifdef _WIN32
//#ifdef IN_MANTID_DATA_HANDLING
//  #define LogParser_DllExport __declspec( dllexport )
//#else
//  #define LogParser_DllExport __declspec( dllimport )
//#endif
//#else
//  #define LogParser_DllExport
//  #define LogParser_DllImport
//#endif

class DLLExport LogParser
{
public:
    /// Create given the icpevent file name
    LogParser(const std::string& eventFName);
    /// Create given the icpevent log property
    LogParser(const Kernel::Property* log);
    /// Destructor
    ~LogParser();

    /// Number of periods
    int nPeriods()const{return m_nOfPeriods;}

    /// Creates a TimeSeriesProperty of either double or string type depending on the log data
    /// Returns a pointer to the created property
    Kernel::Property* createLogProperty(const std::string& logFName, const std::string& name)const;

    /// Ctreates a TimeSeriesProperty<bool> showing times when a particular period was active
    Kernel::Property* createPeriodLog(int period)const;

    /// Ctreates a TimeSeriesProperty<int> with all data periods
    Kernel::Property* createAllPeriodsLog()const;

    /// Ctreates a TimeSeriesProperty<bool> with running status
    Kernel::Property* createRunningLog()const;

private:

    /// TimeSeriesProperty<int> containing data periods. Created by LogParser
    boost::shared_ptr<Kernel::Property> m_periods;

    /// TimeSeriesProperty<bool> containing running status. Created by LogParser
    boost::shared_ptr<Kernel::Property> m_status;

    /// Number of periods
    int m_nOfPeriods;

    /// static reference to the logger class
    static Kernel::Logger& g_log;
};

/// Returns the mean value if the property is TimeSeriesProperty<double>
double timeMean(const Kernel::Property* p);


} // namespace API
} // namespace Mantid

#endif /*MANTID_API_LOGPARSER_H_*/
