#ifndef MANTID_API_XMLLOGFILE_H_
#define MANTID_API_XMLLOGFILE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"


namespace Mantid
{	

  namespace Kernel {
    template <typename TYPE>
	  class TimeSeriesProperty;
  }

  namespace Geometry {
    class Component;
  }

  namespace API
  {
    /** @class XMLlogfile XMLlogfile.h API/XMLlogfile.h

    Class intended to be used by the API LoadInstrument and 
    LoadRaw algorithms to link up parameters defined in instrument 
    definition files with data in ISIS logfiles.

    @author Anders Markvardsen, ISIS, RAL
    @date 12/1/2009

    Copyright &copy; 2007-9 STFC Rutherford Appleton Laboratories

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>    
    */
    class DLLExport XMLlogfile
    {
    public:
      /// Default constructor
      XMLlogfile(std::string& logfileID, std::string& value, std::string& paramName, std::string& type, 
                 bool fixed, std::string& extractSingleValueAs, std::string& eq, Geometry::Component* comp);

      /// Destructor
      ~XMLlogfile() {}


      /// log file XML attributes from instrument definition file
      const std::string m_logfileID;  ///< logfile id
      const std::string m_value;  ///< rather then extracting value from logfile, specify a value directly
      std::string m_paramName;  ///< parameter name
      const std::string m_type;       ///< type of the data
      std::string m_tie;                /// specific to fitting parameter specify any tie
      std::string m_fittingFunction;    /// specific to fitting parameter specify fitting function
      const std::string m_extractSingleValueAs;  ///< describes the way to extract a single value from the log file( average, first number, etc)
      const std::string m_eq;         ///< muParser equation to calculate the parameter value from the log value
      const Geometry::Component* m_component;  ///< the component address

      ///Returns parameter value as generated using possibly equation expression etc
      double createParamValue(Mantid::Kernel::TimeSeriesProperty<double>* logData);

    private:


      ///static reference to the logger class
      static Kernel::Logger& g_log;

    };

  } // namespace API
} // namespace Mantid

#endif /*MANTID_API_XMLLOGFILE_H_*/

