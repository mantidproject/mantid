#ifndef MANTID_API_XMLLOGFILE_H_
#define MANTID_API_XMLLOGFILE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllExport.h"
#include "MantidKernel/Interpolation.h"
#include <string>

namespace Mantid
{	

  namespace Kernel {
    template <typename TYPE>
	  class TimeSeriesProperty;
  }

  namespace Geometry {
    class IComponent;
  }

  namespace API
  {
    /** @class XMLlogfile XMLlogfile.h API/XMLlogfile.h

    Class intended to be used by the API LoadInstrument and 
    LoadRaw algorithms to link up parameters defined in instrument 
    definition files with data in ISIS logfiles.

    @author Anders Markvardsen, ISIS, RAL
    @date 12/1/2009

    Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class EXPORT_OPT_MANTID_API XMLlogfile
    {
    public:
      /// Default constructor
      XMLlogfile(const std::string& logfileID, const std::string& value, const boost::shared_ptr<Kernel::Interpolation>& interpolation,
        const std::string& formula, const std::string& formulaUnit, const std::string& resultUnit, const std::string& paramName, 
        const std::string& type, const std::string& tie, 
        const std::vector<std::string>& constraint, std::string& penaltyFactor, 
        const std::string& fitFunc, const std::string& extractSingleValueAs, 
        const std::string& eq, const Geometry::IComponent* comp);

      /// Destructor
      ~XMLlogfile() {}


      /// log file XML attributes from instrument definition file
      const std::string m_logfileID;  ///< logfile id
      const std::string m_value;  ///< rather then extracting value from logfile, specify a value directly
      const std::string m_paramName;  ///< parameter name
      const std::string m_type;       ///< type of the data, e.g. int, double or string
      const std::string m_tie;                ///< specific to fitting parameter specify any tie
      const std::vector<std::string> m_constraint;  ///< specific to fitting parameter specify lower and upper bound in that order
      std::string m_penaltyFactor;                  ///< specific to fitting parameter specify penalty factor
      const std::string m_fittingFunction;    ///< specific to fitting parameter specify fitting function
      const std::string m_formula;            ///< specific to fitting parameter. Specify formula to use for setting this parameter
      const std::string m_formulaUnit;       ///< unit for formula (i.e. for Centre in formula)
      const std::string m_resultUnit;       ///< expected result (output) unit from evaluating the formula
      boost::shared_ptr<Kernel::Interpolation> m_interpolation; ///< specific to fitting parameter 
      const std::string m_extractSingleValueAs;  ///< describes the way to extract a single value from the log file( average, first number, etc)
      const std::string m_eq;         ///< muParser equation to calculate the parameter value from the log value
      const Geometry::IComponent* m_component;  ///< the component address

      ///Returns parameter value as generated using possibly equation expression etc
      double createParamValue(Mantid::Kernel::TimeSeriesProperty<double>* logData);

    private:


      ///static reference to the logger class
      static Kernel::Logger& g_log;

    };

  } // namespace API
} // namespace Mantid

#endif /*MANTID_API_XMLLOGFILE_H_*/

