//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/Instrument/XMLlogfile.h"
#include "MantidGeometry/IComponent.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/LogParser.h"
#include <muParser.h>
#include <ctime>
#include <fstream>

namespace Mantid
{
  namespace Geometry
  {

    using namespace Kernel;

    Logger& XMLlogfile::g_log = Logger::get("XMLlogfile");
    double XMLlogfile::angleConvertConst = 1.0;

    /** Constructor
    *  @param logfileID The logfile id -- the part of the file name which identifies the log 
    *  @param value Rather then extracting value from logfile, specify a value directly
    *  @param paramName The name of the parameter which will be created based on the log values
    *  @param type The type
    *  @param extractSingleValueAs Describes the way to extract a single value from the log file( average, first number, etc)
    *  @param eq muParser equation to calculate the parameter value from the log value
    *  @param comp The pointer to the instrument component
    *  @param interpolation The pointer to the interpolation class
    *  @param formula The string formula to apply
    *  @param formulaUnit The unit that the formul requires the input vaule in
    *  @param resultUnit The unit of the result of the formula
    *  @param tie What to tie the value to
    *  @param constraint The constraint associated with this parameter
    *  @param penaltyFactor The level of penalty associated with the constraint
    *  @param fitFunc What fit function this applies to
    */
    XMLlogfile::XMLlogfile(const std::string& logfileID, const std::string& value, const boost::shared_ptr<Kernel::Interpolation>& interpolation, 
      const std::string& formula, const std::string& formulaUnit, const std::string& resultUnit, const std::string& paramName, 
      const std::string& type, const std::string& tie, 
      const std::vector<std::string>& constraint, std::string& penaltyFactor, 
      const std::string& fitFunc, const std::string& extractSingleValueAs, 
      const std::string& eq, const Geometry::IComponent* comp)
      : m_logfileID(logfileID), m_value(value), m_paramName(paramName), m_type(type), m_tie(tie),
      m_constraint(constraint), m_penaltyFactor(penaltyFactor), m_fittingFunction(fitFunc),
      m_formula(formula), m_formulaUnit(formulaUnit), m_resultUnit(resultUnit), m_interpolation(interpolation),
      m_extractSingleValueAs(extractSingleValueAs), m_eq(eq), m_component(comp)
    {
    }

    /** Returns the parameter value.
     * This interprets the XML parameter specification in order to do one of these things:
     *  - Calculate an equation result, if specified
     *  - Interpolate the value, if desired.
     *  - Just extract the value (perhaps the man or just the n-th position) and return that.
    *
    *  @param logData Data in logfile
    *  @return parameter value
    *
    *  @throw InstrumentDefinitionError Thrown if issues with the content of XML instrument definition file
    */
    double XMLlogfile::createParamValue(TimeSeriesProperty<double>* logData)
    {
      // If this parameter is a <look-up-table> or <formula> return 0.0. Such parameter types are 
      // associated with 'fitting' parameters. In some sense this method should never be called
      // for such parameters since they return values from other attributes and only during (or just
      // before) 'fitting'. But include the statement below for safety.

      if ( !m_formula.empty() || m_interpolation->containData() )
        return 0.0;

      // also this method should not be called when parameter is of 'string' type. Display
      // an error and return 0.0

      if ( m_type == "string" )
      {
        g_log.error() << "XMLlogfile::createParamValue has been called with a 'string' parameters.\n"
          << "Return meaningless zere value.";
        return 0.0;
      }

      double extractedValue = 0.0; 

      // Get value either as directly specified by user using the 'value' attribute or through
      // a logfile as specified using the 'logfile-id' attribute. Note if both specified 'logfile-id'
      // takes precedence over the 'value' attribute

      if ( !m_logfileID.empty() )
      {
        // get value from time series

        if ( m_extractSingleValueAs.compare("mean" ) == 0 )
        {
          extractedValue = timeMean(logData);
        }
        // Looking for string: "position n", where n is an integer
        else if ( m_extractSingleValueAs.find("position") == 0 && m_extractSingleValueAs.size() >= 10 )
        {
          std::stringstream extractPosition(m_extractSingleValueAs);
          std::string dummy;
          int position;
          extractPosition >> dummy >> position;

          extractedValue = logData->nthValue(position);
        }
        else
        {
          throw Kernel::Exception::InstrumentDefinitionError(std::string("extract-single-value-as attribute for <parameter>")
            + " element (eq=" + m_eq + ") in instrument definition file is not recognised.");
        }
      }
      else 
      {
        try
        {
          extractedValue = boost::lexical_cast<double>(m_value);
        }
        catch (boost::bad_lexical_cast &)
        {
          throw Kernel::Exception::InstrumentDefinitionError(std::string("<parameter> with name ")
            + m_paramName + " much be set to a number,\n" + 
            "unless it is meant to be a 'string' parameter, see http://www.mantidproject.org/InstrumentDefinitionFile .\n");
        }
      }

      // Check if m_eq is specified if yes evaluate this equation

      if ( m_eq.empty() )
        return extractedValue;

      size_t found;
      std::string equationStr = m_eq;
      found = equationStr.find("value");
      if ( found==std::string::npos )
      {
        throw Kernel::Exception::InstrumentDefinitionError(std::string("Equation attribute for <parameter>")
          + " element (eq=" + m_eq + ") in instrument definition file must contain the string: \"value\"."
          + ". \"value\" is replaced by a value from the logfile.");
      }

      std::stringstream readDouble;
      readDouble << extractedValue;
      std::string extractedValueStr = readDouble.str();
      equationStr.replace(found, 5, extractedValueStr);

      // check if more than one 'value' in m_eq

      while ( equationStr.find("value") != std::string::npos )
      {
        found = equationStr.find("value");
        equationStr.replace(found, 5, extractedValueStr);
      }

      try
      {
        mu::Parser p;
        p.SetExpr(equationStr);
        return p.Eval();
      }
      catch (mu::Parser::exception_type &e)
      {
        throw Kernel::Exception::InstrumentDefinitionError(std::string("Equation attribute for <parameter>")
          + " element (eq=" + m_eq + ") in instrument definition file cannot be parsed."
          + ". Muparser error message is: " + e.GetMsg());
      }



    }



  } // namespace Geometry
} // namespace Mantid
