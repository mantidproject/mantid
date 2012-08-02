#ifndef MANTID_API_RUN_H_
#define MANTID_API_RUN_H_

#include "MantidAPI/DllConfig.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/Cache.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/TimeSplitter.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Matrix.h"
#include "MantidNexusCPP/NeXusFile.hpp"
#include <vector>

namespace Mantid
{
  namespace API
  {

    /**
       This class stores information regarding an experimental run as a series
       of log entries

       @author Martyn Gigg, Tessella plc
       @date 22/07/2010
       
       Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
       
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
    class MANTID_API_DLL Run
    {
    public:
      /// Default constructor
      Run();
      /// Virtual destructor
      virtual ~Run();
      /// Copy constructor
      Run(const Run& copy);
      /// Assignment operator
      const Run& operator=(const Run& rhs);
      /// Addition
      Run& operator+=(const Run& rhs);

      /// Set the run start and end
      void setStartAndEndTime(const Kernel::DateAndTime & start, const Kernel::DateAndTime & end);
      /// Return the run start time
      const Kernel::DateAndTime startTime() const;
      /// Return the run end time
      const Kernel::DateAndTime endTime() const;

      /// Filter the logs by time
      void filterByTime(const Kernel::DateAndTime start, const Kernel::DateAndTime stop);
      /// Split the logs based on the given intervals
      void splitByTime(Kernel::TimeSplitterType& splitter, std::vector< Run * > outputs) const;
      /// Filter the run by the given boolean log
      void filterByLog(const Kernel::TimeSeriesProperty<bool> & filter);

      /// Return an approximate memory size for the object in bytes
      size_t getMemorySize() const;

      /// Add data to the object in the form of a property
      void addProperty(Kernel::Property *prop, bool overwrite = false);
      /// Add a property of given type
      template<class TYPE>
      void addProperty(const std::string & name, const TYPE & value, bool overwrite = false);

      template<class TYPE>
      void addProperty(const std::string & name, const TYPE & value, const std::string & units,
                       bool overwrite = false);

      /// Does the property exist on the object
      bool hasProperty(const std::string & name) const;
      /// Remove a named property
      void removeProperty(const std::string &name, bool delproperty=true);
      /**
       * Return all of the current properties
       * @returns A vector of the current list of properties
       */
      inline const std::vector<Kernel::Property*>& getProperties() const { return m_manager.getProperties(); }
      /// Returns a property as a time series property. It will throw if it is not valid
      template<typename T>
      Kernel::TimeSeriesProperty<T> * getTimeSeriesProperty(const std::string & name) const;
      /// Get the value of a property as the given TYPE. Throws if the type is not correct
      template<typename HeldType>
      HeldType getPropertyValueAsType(const std::string & name) const;
      /// Returns a property as a single double value from its name
      double getPropertyAsSingleValue(const std::string & name, Kernel::Math::StatisticType statistic = Kernel::Math::Mean) const;
      /// Returns the named property as a pointer
      Kernel::Property * getProperty(const std::string & name) const;

      /// Set the proton charge
      void setProtonCharge( const double charge);
      /// Get the proton charge
      double getProtonCharge() const;
      /// Integrate the proton charge over the whole run time
      double integrateProtonCharge();

      /// Store the given values as a set of histogram bin boundaries
      void storeHistogramBinBoundaries(const std::vector<double> & energyBins);
      /// Returns the bin boundaries for a given value
      std::pair<double, double> histogramBinBoundaries(const double energyValue) const;

      /** @return a reference to the Goniometer object for this run */
      Mantid::Geometry::Goniometer & getGoniometer()
      { return m_goniometer; }

      /** @return a reference to the const Goniometer object for this run */
      const Mantid::Geometry::Goniometer & getGoniometer() const
      { return m_goniometer; }

      // Retrieve the goniometer rotation matrix
      Mantid::Kernel::DblMatrix getGoniometerMatrix();

      /**
       * Add a log entry
       * @param p :: A pointer to the property containing the log entry
       */
      void addLogData( Kernel::Property *p ) { addProperty(p); }
      /**
       * Access a single log entry
       * @param name :: The name of the log entry to retrieve
       * @returns A pointer to a property containing the log entry
       */ 
      Kernel::Property* getLogData(const std::string &name) const { return getProperty(name); }
      /**
       * Access all log entries
       * @returns A list of all of the log entries
       */ 
      const std::vector< Kernel::Property* >& getLogData() const {return getProperties(); } 
      /**
       * Remove a named log entry
       * @param name :: The name of the entry to remove
       * @param delproperty :: If true, delete the log entry
       */
      void removeLogData(const std::string &name, const bool delproperty=true) { return removeProperty(name, delproperty); }
      /**
       * @param name :: The name of the property
       * @param statistic :: Defines how to calculate the single value from series (default=Mean)
       * @return A log as a single value using the given statistic type
       */
      double getLogAsSingleValue(const std::string & name, Kernel::Math::StatisticType statistic = Kernel::Math::Mean) const { return getPropertyAsSingleValue(name, statistic); }
      
      /// Save the run to a NeXus file with a given group name
      void saveNexus(::NeXus::File * file, const std::string & group) const;
      /// Load the run from a NeXus file with a given group name
      void loadNexus(::NeXus::File * file, const std::string & group);

    private:
      /// Adds all the time series in from one property manager into another
      void mergeMergables(Mantid::Kernel::PropertyManager & sum, const Mantid::Kernel::PropertyManager & toAdd);

      /// Static reference to the logger class
      static Kernel::Logger &g_log;

      /// A pointer to a property manager
      Kernel::PropertyManager m_manager;
      /// Goniometer for this run
      Mantid::Geometry::Goniometer m_goniometer;
      /// A set of histograms that can be stored here for future reference
      std::vector<double> m_histoBins;

      /// Cache type for single value logs
      typedef Kernel::Cache<std::pair<std::string,Kernel::Math::StatisticType>, double> SingleValueCache;
      /// Cache for the retrieved single values
      mutable SingleValueCache m_singleValueCache;
    };

    /**
     * Add a property of a specified type (Simply creates a Kernel::Property of that type
     * @param name :: The name of the type
     * @param value :: The value of the property
     * @param overwrite :: If true, a current value is overwritten. (Default: False)
     */
    template<class TYPE>
      void Run::addProperty(const std::string & name, const TYPE & value, bool overwrite)
    {
      addProperty(new Kernel::PropertyWithValue<TYPE>(name, value), overwrite);
    }

    /**
     * Add a property of a specified type (Simply creates a Kernel::Property of that type)
     *  and set its units.
     * @param name :: The name of the type
     * @param value :: The value of the property
     * @param units :: a string giving the units of the property.
     * @param overwrite :: If true, a current value is overwritten. (Default: False)
     */
    template<class TYPE>
      void Run::addProperty(const std::string & name, const TYPE & value, const std::string& units, bool overwrite)
    {
      Kernel::Property * newProp = new Kernel::PropertyWithValue<TYPE>(name, value);
      newProp->setUnits(units);
      addProperty(newProp, overwrite);
    }

  }
}

#endif //MANTIDAPI_RUN_H_
