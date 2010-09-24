#ifndef MANTID_API_RUN_H_
#define MANTID_API_RUN_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/TimeSplitter.h"
#include <vector>

namespace Mantid
{
  //-------------------------------------------------------------------
  // Forward declarations
  //-------------------------------------------------------------------

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
    class DLLExport Run
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

      void filterByTime(const Kernel::dateAndTime start, const Kernel::dateAndTime stop);
      void splitByTime(Kernel::TimeSplitterType& splitter, std::vector< Run * > outputs) const;

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
      void removeProperty(const std::string &name) { m_manager.removeProperty(name); }
      /**
       * Return all of the current properties
       * @returns A vector of the current list of properties
       */
      const std::vector<Kernel::Property*>& getProperties() const { return m_manager.getProperties(); }
      /**
       * Returns the named property
       * @param name The name of the property
       * @returns The named property
       */
      Kernel::Property * getProperty(const std::string & name) const
      {
        Kernel::Property *p = m_manager.getProperty(name);
        return p;
      }

      /** @name Legacy functions */
      //@{
      /// Set the proton charge
      void setProtonCharge( const double charge);
      /// Get the proton charge
      double getProtonCharge() const;

      double integrateProtonCharge();

      /**
       * Add a log entry
       * @param p A pointer to the property containing the log entry
       */
      void addLogData( Kernel::Property *p ) { addProperty(p); }

      /**
       * Access a single log entry
       * @param name The name of the log entry to retrieve
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
       * @param name The name of the entry to remove
       */
      void removeLogData(const std::string &name) { return removeProperty(name); }
      //@}

    private:
      /// A pointer to a property manager
      Kernel::PropertyManager m_manager;
      /// The name of the proton charge property
      const std::string m_protonChargeName;
    };

    /**
     * Add a property of a specified type (Simply creates a Kernel::Property of that type
     * @param name The name of the type
     * @param value The value of the property
     * @param overwrite If true, a current value is overwritten. (Default: False)
     */
    template<class TYPE>
      void Run::addProperty(const std::string & name, const TYPE & value, bool overwrite)
    {
      addProperty(new Kernel::PropertyWithValue<TYPE>(name, value), overwrite);
    }

    /**
     * Add a property of a specified type (Simply creates a Kernel::Property of that type)
     *  and set its units.
     * @param name The name of the type
     * @param value The value of the property
     * @param units a string giving the units of the property.
     * @param overwrite If true, a current value is overwritten. (Default: False)
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
