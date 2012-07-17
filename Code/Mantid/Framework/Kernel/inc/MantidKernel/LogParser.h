#ifndef MANTID_KERNEL_LOGPARSER_H_
#define MANTID_KERNEL_LOGPARSER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/Logger.h"

#include <boost/shared_ptr.hpp>

#include <map>
#include <sstream>

namespace Mantid
{

  namespace Kernel
  {

    //-------------------------------------------------------------------------
    // Forward declarations
    //-------------------------------------------------------------------------
    class DateAndTime;
    class Property;
    template<typename T> class TimeSeriesProperty;

    /**
    LogParser parses the instrument log files to select records corresponding
    to 'RUNNING' instrument status. It determines the values of the logged variables
    at the beginning and the end of each RUNNING interval and keeps track of changes
    within the interval.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 19/01/2009

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class MANTID_KERNEL_DLL LogParser
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

      /// Creates a TimeSeriesProperty<bool> showing times when a particular period was active
      Kernel::TimeSeriesProperty<bool>* createPeriodLog(int period) const;

      /// Creates a log value for the current period.
      Kernel::Property* createCurrentPeriodLog(const int& period) const;

      /// Creates a TimeSeriesProperty<int> with all data periods
      Kernel::Property* createAllPeriodsLog() const;

      /// Creates a TimeSeriesProperty<bool> with running status
      Kernel::TimeSeriesProperty<bool>* createRunningLog() const;

      /// This method returns data periods property and useful for loading log data for multi period files
      const boost::shared_ptr<Kernel::Property> getPeriodsProperty() const {return m_periods;}

    private:

      /// Available commands.
      enum commands {NONE = 0,BEGIN,END,CHANGE_PERIOD};

      /// Typedef for a map of string commands to an enum of strongly typed commands.
      typedef std::map<std::string, commands> CommandMap;

      /// TimeSeriesProperty<int> containing data periods. Created by LogParser
      boost::shared_ptr<Kernel::Property> m_periods;

      /// TimeSeriesProperty<bool> containing running status. Created by LogParser
      boost::shared_ptr<Kernel::TimeSeriesProperty<bool> > m_status;

      /// Number of periods
      int m_nOfPeriods;

      /// static reference to the logger class
      static Kernel::Logger& g_log;

      /// Creates a map of all available commands.
      CommandMap createCommandMap() const;

      /// Try to parse period data.
      void tryParsePeriod(const std::string& com, const DateAndTime& time, std::istringstream& idata, Kernel::TimeSeriesProperty<int>* const periods);
    };

    /// Returns the mean value if the property is TimeSeriesProperty<double>
    MANTID_KERNEL_DLL double timeMean(const Kernel::Property* p);

    /// Extract a line from input stream, discarding any EOL characters encountered
    MANTID_KERNEL_DLL std::istream& extractToEOL(std::istream& is, std::string& str);



  } // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_LOGPARSER_H_*/
