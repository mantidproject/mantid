#ifndef MANTID_API_RUN_H_
#define MANTID_API_RUN_H_

#include "MantidAPI/DllConfig.h"
#include "MantidGeometry/Instrument/Goniometer.h"
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

      void filterByTime(const Kernel::DateAndTime start, const Kernel::DateAndTime stop);
      void splitByTime(Kernel::TimeSplitterType& splitter, std::vector< Run * > outputs) const;

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
      bool hasProperty(const std::string & name) const { return m_manager.existsProperty(name); }
      // Expose some of the PropertyManager publicly
      /**
       * Remove a named property
       */
      void removeProperty(const std::string &name, bool delproperty=true) { m_manager.removeProperty(name, delproperty); }
      /**
       * Return all of the current properties
       * @returns A vector of the current list of properties
       */
      const std::vector<Kernel::Property*>& getProperties() const { return m_manager.getProperties(); }
      /// Returns a property as a time series property. It will throw if it is not valid
      template<typename T>
      Kernel::TimeSeriesProperty<T> * getTimeSeriesProperty(const std::string & name) const;
      /// Get the value of a property as the given TYPE. Throws if the type is not correct
      template<typename HeldType>
      HeldType getPropertyValueAsType(const std::string & name) const;

      /**
       * Returns the named property
       * @param name :: The name of the property
       * @returns The named property
       */
      Kernel::Property * getProperty(const std::string & name) const
      {
        Kernel::Property *p = m_manager.getProperty(name);
        return p;
      }

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
      /// Save the run to a NeXus file with a given group name
      void saveNexus(::NeXus::File * file, const std::string & group) const;
      /// Load the run from a NeXus file with a given group name
      void loadNexus(::NeXus::File * file, const std::string & group);

    private:
      /// The number of properties that are summed when two workspaces are summed
      static const int ADDABLES;
      /// The names of the properties to sum when two workspaces are summed
      static const std::string ADDABLE[];
      /// The name of the proton charge property
      static const char *PROTON_CHARGE_LOG_NAME;
      /// The name of the histogram bins property
      static const char *HISTOGRAM_BINS_LOG_NAME;

      /// A pointer to a property manager
      Kernel::PropertyManager m_manager;
      
      /// Goniometer for this run
      Mantid::Geometry::Goniometer m_goniometer;

      /// Adds all the time series in from one property manager into another
      void mergeMergables(Mantid::Kernel::PropertyManager & sum, const Mantid::Kernel::PropertyManager & toAdd);

      /// Static reference to the logger class
      static Kernel::Logger &g_log;

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
