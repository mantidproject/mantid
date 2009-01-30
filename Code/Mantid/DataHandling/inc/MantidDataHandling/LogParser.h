#ifndef MANTID_API_LOGPARSER_H_
#define MANTID_API_LOGPARSER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Logger.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <map>
#include <vector>
#include <ctime>

using namespace boost::posix_time;
using namespace boost::gregorian;


namespace Mantid
{

namespace Kernel
{
    class Property;
}

namespace DataHandling
{
/** @class LogParser LogParser.h DataHandling/LogParser.h

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

#ifdef _WIN32
#ifdef IN_MANTID_DATA_HANDLING
  #define LogParser_DllExport __declspec( dllexport )
#else
  #define LogParser_DllExport __declspec( dllimport )
#endif
#else
  #define LogParser_DllExport
  #define LogParser_DllImport
#endif

class LogParser_DllExport LogParser
{
public:
    /// Default constructor
    LogParser():m_nOfPeriods(1),m_unknown(true){}
    /// 
    LogParser(const std::string& eventFName);
    /// Destructor
    ~LogParser() {}

    /// Returns the period if the instrument was running at the moment tim or 0 otherwise.
    int period(ptime tim)const;

    /// Number of periods
    int nPeriods()const{return m_nOfPeriods;}

    /// Returns time intervals for a period
    std::vector<time_period> getTimes(int p)const;

    /// Creates a TimeSeriesProperty of either double or string type depending on the log data
    /// Returns a pointer to the created property
    Kernel::Property* createLogProperty(const std::string& logFName, const std::string& name, int period = 1);

private:

    /// Time intervals when the instrument was running and the corresponding data period.
    std::map<time_period,int> m_periods;

    /// Number of periods
    int m_nOfPeriods;

    /// Flag set if running time are unknown (icpevent file was not found)
    bool m_unknown;

    /// static reference to the logger class
    static Kernel::Logger& g_log;
};

/// Returns the mean value if the property is TimeSeriesProperty<double>
double LogParser_DllExport timeMean(const Kernel::Property* p);

/// Returnd the first value in the time series (if nimeric).
/// Throws runtime_error if empty
double LogParser_DllExport firstValue(const Kernel::Property* p);

/// Returnd the second value in the time series (if nimeric).
/// Throws runtime_error if empty or has only single value
double LogParser_DllExport secondValue(const Kernel::Property* p);

/// Returnd the last value in the time series (if nimeric).
/// Throws runtime_error if empty
double LogParser_DllExport lastValue(const Kernel::Property* p);

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_API_LOGPARSER_H_*/
