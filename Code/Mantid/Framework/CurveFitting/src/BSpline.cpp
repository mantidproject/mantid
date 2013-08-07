/*WIKI*

This function creates spline using the set of points and interpolates the input between them.

First and second derivatives from the spline can be calculated by using the derivative1D function.

BSpline function takes a set of attributes and a set of parameters. The first attrbiute is 'n' which has integer type and sets the number of interpolation points.
The parameter names have the form 'yi' where 'y' is letter 'y' and 'i' is the parameter's index starting from 0 and have the type double. Likewise, the attribute names have the form 'xi'.

 *WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/BSpline.h"
#include "MantidCurveFitting/GSLVector.h"
#include "MantidAPI/FunctionFactory.h"

#include <boost/lexical_cast.hpp>
#include <iostream>

namespace Mantid
{
  namespace CurveFitting
  {
    using namespace Kernel;
    using namespace API;

    DECLARE_FUNCTION(BSpline)

    /**
     * Constructor
     */
    BSpline::BSpline():m_bsplineWorkspace(NULL)
    {

      declareAttribute( "Order", Attribute( 3 ));
      declareAttribute( "NBreak", Attribute( 10 ));
      declareAttribute( "Uniform", Attribute( true ));

      declareAttribute( "StartX", Attribute(0.0) );
      declareAttribute( "EndX", Attribute(1.0) );
      declareAttribute( "BreakPoints", Attribute( std::vector<double>(10) ));

      resetGSLObjects();
      resetParameters();
    }

    /**
     * Destructor
     */
    BSpline::~BSpline()
    {
        gsl_bspline_free( m_bsplineWorkspace );
    }

    /** Execute the function
     *
     * @param out :: The array to store the calculated y values
     * @param xValues :: The array of x values to interpolate
     * @param nData :: The size of the arrays
     */
    void BSpline::function1D(double* out, const double* xValues, const size_t nData) const
    {
        size_t np = nParams();
        GSLVector B(np);
        double startX = getAttribute("StartX").asDouble();
        double endX = getAttribute("EndX").asDouble();
        for(size_t i = 0; i < nData; ++i)
        {
            double x = xValues[i];
            if ( x < startX || x > endX )
            {
                out[i] = 0.0;
            }
            else
            {
                int ret = gsl_bspline_eval(x, B.gsl(), m_bsplineWorkspace);
                double val = 0.0;
                for(size_t j = 0; j < np; ++j)
                {
                    val += getParameter(j) * B.get(j);
                }
                out[i] = val;
            }
        }
    }


    /** Calculate the derivatives for a set of points on the spline
     *
     * @param out :: The array to store the derivatives in
     * @param xValues :: The array of x values we wish to know the derivatives of
     * @param nData :: The size of the arrays
     * @param order :: The order of the derivatives o calculate
     */
    void BSpline::derivative1D(double* out, const double* xValues, size_t nData, const size_t order) const
    {

    }

    /** Set an attribute for the function
     *
     * @param attName :: The name of the attribute to set
     * @param att :: The attribute to set
     */
    void BSpline::setAttribute(const std::string& attName, const API::IFunction::Attribute& att)
    {
        bool isUniform = attName == "Uniform" && att.asBool();

        storeAttributeValue(attName, att);

        if ( attName == "BreakPoints" || isUniform || attName == "StartX" || attName == "EndX"  )
        {
            resetKnots();
        }
        else if ( attName == "NBreak" )
        {
            resetGSLObjects();
            resetParameters();
        }
        else if ( attName == "Order" )
        {
            resetGSLObjects();
            resetParameters();
        }

    }

    /**
     * Process all attributes at the same time re-setting the internal objects only once.
     * @param attributes :: The attributes to set.
     */
    void BSpline::setAttributes(const std::map<std::string, IFunction::Attribute> &attributes)
    {
        // will b-spline be uniform or not
        bool isUniform = false;
        auto att = attributes.find("Uniform");
        if ( att != attributes.end() )
        {
            isUniform = att->second.asBool();
            storeAttributeValue("Uniform",att->second);
        }
        else
        {
            isUniform = getAttribute("Uniform").asBool();
        }

        // Order is set in any case
        att = attributes.find("Order");
        if ( att != attributes.end() )
        {
            storeAttributeValue("Order",att->second);
        }

        // if b-spline is uniform it is defined by NBreak, StartX, EndX
        if ( isUniform )
        {
            if ( attributes.find("BreakPoints") != attributes.end() )
            {
                g_log.warning() << "BSpline: BreakPoints attribute is ignored because Uniform is set to true.";
            }
            att = attributes.find("NBreak");
            if ( att != attributes.end() ) storeAttributeValue("NBreak",att->second);
            att = attributes.find("StartX");
            if ( att != attributes.end() ) storeAttributeValue("StartX",att->second);
            att = attributes.find("EndX");
            if ( att != attributes.end() ) storeAttributeValue("EndX",att->second);
        }
        else
        {
            att = attributes.find("BreakPoints");
            if ( att != attributes.end() ) storeAttributeValue("BreakPoints",att->second);
        }

        resetGSLObjects();
        resetParameters();
    }

    /**
     * Initialize the GSL objects.
     */
    void BSpline::resetGSLObjects()
    {
        if ( m_bsplineWorkspace != NULL )
        {
            gsl_bspline_free( m_bsplineWorkspace );
        }
        int order = getAttribute("Order").asInt();
        int nbreak = getAttribute("NBreak").asInt();
        m_bsplineWorkspace = gsl_bspline_alloc ( static_cast<size_t>(order), static_cast<size_t>(nbreak) );
    }

    /**
     * Reset fitting parameters after changes to some attributes.
     */
    void BSpline::resetParameters()
    {
        if ( nParams() > 0 )
        {
            clearAllParameters();
        }
        size_t np = gsl_bspline_ncoeffs( m_bsplineWorkspace );
        for(size_t i = 0; i < np; ++i)
        {
            std::string pname = "A" + boost::lexical_cast<std::string>( i );
            declareParameter( pname );
        }

        resetKnots();
    }

    /**
     * Recalculate the B-spline knots
     */
    void BSpline::resetKnots()
    {
        bool isUniform = getAttribute("Uniform").asBool();

        std::vector<double> breakPoints;
        if ( isUniform )
        {
            // create uniform knots in the interval [StartX, EndX]
            double startX = getAttribute("StartX").asDouble();
            double endX = getAttribute("EndX").asDouble();
            gsl_bspline_knots_uniform( startX, endX, m_bsplineWorkspace );
            getGSLBreakPoints( breakPoints );
            storeAttributeValue( "BreakPoints", Attribute(breakPoints) );
        }
        else
        {
            // set the break points from BreakPoints vector attribute, update other attributes
            breakPoints = getAttribute( "BreakPoints" ).asVector();
            GSLVector bp = breakPoints;
            gsl_bspline_knots( bp.gsl(), m_bsplineWorkspace );
            storeAttributeValue( "StartX", Attribute(breakPoints.front()) );
            storeAttributeValue( "EndX", Attribute(breakPoints.back()) );
            storeAttributeValue( "NBreak", Attribute( static_cast<int>(breakPoints.size()) ) );
        }
    }

    /**
     * Copy break points from GSL internal objects
     * @param bp :: A vector to accept the break points.
     */
    void BSpline::getGSLBreakPoints(std::vector<double> &bp) const
    {
        size_t n = gsl_bspline_nbreak( m_bsplineWorkspace );
        bp.resize( n );
        for(size_t i = 0; i < n; ++i)
        {
            bp[i] = gsl_bspline_breakpoint( i, m_bsplineWorkspace );
        }
    }

  } // namespace CurveFitting
} // namespace Mantid
