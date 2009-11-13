#ifndef MANTID_CURVEFITTING_BOUNDARYCONSTRAINT_H_
#define MANTID_CURVEFITTING_BOUNDARYCONSTRAINT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IConstraint.h"
//#include <string>

namespace Mantid
{
  namespace CurveFitting
  {
    //----------------------------------------------------------------------
    // Forward Declaration
    //----------------------------------------------------------------------
    class Kernel::Logger;

    /**
    A boundary constraint is designed to be used to set either
    upper or lower (or both) boundaries on a single parameter.

    @author Anders Markvardsen, ISIS, RAL
    @date 13/11/2009

    Copyright &copy; 2007-9 STFC Rutherford Appleton Laboratory

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
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport BoundaryConstraint : public API::IConstraint
    {
    public:
      /// Constructor with no boundary arguments
      BoundaryConstraint(const std::string& paramName) : 
        m_activeParameterIndex(-1),
        m_penaltyFactor(1000.0),
        m_parameterName(paramName),
        m_hasLowerBound( false), 
        m_hasUpperBound( false)
      {
      }

      /// Constructor with boundary arguments
      BoundaryConstraint(const std::string paramName, const double lowerBound, const double upperBound) : 
        m_activeParameterIndex(-1),
        m_penaltyFactor(1000.0),
        m_parameterName(paramName),
        m_hasLowerBound( true), 
        m_hasUpperBound( true),    
        m_lowerBound(lowerBound), 
        m_upperBound(upperBound)
      {
      }

      /// Destructor
      virtual ~BoundaryConstraint() {}

      /// Set panelty factor. The larger the number to thigter the constraint. This number
      /// must be set to a number larger than zero
      void setPenaltyFactor(const double& c); 

      /// Check to see if constraint is valid with respect to a given fitting function, this
      /// means for now that the parameter name which have been specified for the constraint
      /// is also one of the active parameters of the fitting function
      bool isValid(API::IFunction* fn);

      /// Return if it has a lower bound
      bool        hasLower() const { return m_hasLowerBound; }
      /// Return if it has a lower bound
      bool        hasUpper() const { return m_hasUpperBound; }
      /// Return the lower bound value
      const double&    lower()    const { return m_lowerBound; }
      /// Return the upper bound value
      const double&    upper()    const { return m_upperBound; }

      /// Set lower bound value
      void setLower( const double& value ) { m_hasLowerBound = true; m_lowerBound = value; }
      /// Set upper bound value
      void setUpper( const double& value ) { m_hasUpperBound = true; m_upperBound = value; }
      /// Clear lower bound value
      void clearLower()  { m_hasLowerBound = false; m_lowerBound = double(); }
      /// Clear upper bound value
      void clearUpper()  { m_hasUpperBound = false; m_upperBound = double(); }

      /// Set both bounds (lower and upper) at the same time
      void setBounds( const double& lower, const double& upper) 
      {
        setLower( lower ); 
        setUpper( upper ); 
      }

      /// Clear both bounds (lower and upper) at the same time
      void clearBounds() 
      {
        clearLower(); 
        clearUpper(); 
      }

      /// overwrite IConstraint base class methods
      virtual double check(API::IFunction* fn);
      virtual boost::shared_ptr<std::vector<double> > checkDeriv(API::IFunction* fn);

    private:

      /// index of parameter in list of the active parameters passed by function. This number is
      /// negative if not set
      int m_activeParameterIndex;

      /// instantiate m_activeParameterIndex if not already instantiated
      void instantiateParameterIndex(API::IFunction* fn);


      double m_penaltyFactor;

      // name of parameter you want to constraint
      std::string m_parameterName;

      /// has a lower bound set true/false
      bool  m_hasLowerBound;
      /// has a upper bound set true/false
      bool  m_hasUpperBound;
      /// the lower bound
      double m_lowerBound;
      /// the upper bound
      double m_upperBound;

      /// Static reference to the logger class
      static Kernel::Logger& g_log;
    };

  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_BOUNDARYCONSTRAINT_H_*/
