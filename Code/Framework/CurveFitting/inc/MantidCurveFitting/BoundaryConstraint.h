#ifndef MANTID_CURVEFITTING_BOUNDARYCONSTRAINT_H_
#define MANTID_CURVEFITTING_BOUNDARYCONSTRAINT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IConstraint.h"

namespace Mantid
{
  namespace CurveFitting
  {
    //----------------------------------------------------------------------
    // Forward Declaration
    //----------------------------------------------------------------------

    /**
    A boundary constraint is designed to be used to set either
    upper or lower (or both) boundaries on a single parameter.

    @author Anders Markvardsen, ISIS, RAL
    @date 13/11/2009

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
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport BoundaryConstraint : public API::IConstraint
    {
    public:
      /// Default constructor 
      BoundaryConstraint() : 
        m_activeParameterIndex(-1),
        m_penaltyFactor(1000.0),
        m_parameterName(""),
        m_hasLowerBound( false), 
        m_hasUpperBound( false)
      {
      }

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
      BoundaryConstraint(API::IFitFunction* fun, const std::string paramName, const double lowerBound, const double upperBound);

      /// Destructor
      virtual ~BoundaryConstraint() {}

      /// Initialize the constraint from an expression
      void initialize(API::IFitFunction* fun, const API::Expression& expr);

      /// implement IConstraint virtual functions
      void setPenaltyFactor(const double& c); 
      double getPenaltyFactor()const { return m_penaltyFactor;}

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

      /// Get parameter name
      std::string getParameterName()const{return m_parameterName;}

      /// overwrite IConstraint base class methods
      virtual double check();
      virtual double checkDeriv();
      virtual void setParamToSatisfyConstraint();
      virtual std::string asString()const;

    private:

      /// index of parameter in list of the active parameters passed by function. This number is
      /// negative if not set
      int m_activeParameterIndex;

      /// instantiate m_activeParameterIndex if not already instantiated
      //void instantiateParameterIndex(API::IFitFunction* fn);

      //int determineParameterIndex(API::IFitFunction* fn);


      double m_penaltyFactor;

      /// name of parameter you want to constraint
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
