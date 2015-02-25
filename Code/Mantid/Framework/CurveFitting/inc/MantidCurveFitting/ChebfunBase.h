#ifndef MANTID_CURVEFITTING_CHEBFUNBASE_H
#define MANTID_CURVEFITTING_CHEBFUNBASE_H

#include "DllConfig.h"
#include "GSLMatrix.h"

#include <boost/shared_ptr.hpp>
#include <vector>
#include <functional>

namespace Mantid {

namespace API{
  class IFunction;
}

namespace CurveFitting {

typedef std::vector<double> ChebfunVec;
/// Type of the approximated function
typedef std::function<double(double)> ChebfunFunctionType;


/**
 * @brief The ChebfunBase class provides a base for a single chebfun.
 *
 * It keeps the x-points (knots? or just points?) and various weights.
 * A single chebfun base can be shared by multiple chebfuns.
 */
class MANTID_CURVEFITTING_DLL ChebfunBase
{
public:
    ChebfunBase(size_t n, double start, double end, double tolerance = 0.0);
    /// Copy constructor
    ChebfunBase( const ChebfunBase& other );
    /// Get the polynomial order of the chebfun based on this base.
    size_t order() const {return m_n;}
    /// Get the size of the base which is the number of x-points.
    size_t size() const {return m_x.size();}
    /// Start of the interval
    double startX() const {return m_x.front();}
    /// End of the interval
    double endX() const {return m_x.back();}
    /// Get the width of the interval
    double width() const {return endX() - startX();}
    /// Get a reference to the x-points
    const std::vector<double>& xPoints() const {return m_x;}
    /// Get a reference to the integration weights
    const std::vector<double>& integrationWeights() const;
    /// Calculate an integral
    double integrate(const ChebfunVec& p) const;
    /// Calculate expansion coefficients
    ChebfunVec calcA(const ChebfunVec& p)const;
    /// Calculate function values
    ChebfunVec calcP(const ChebfunVec& a)const;
    /// Calculate function values at chebfun x-points
    ChebfunVec fit(ChebfunFunctionType f ) const;
    /// Calculate function values at chebfun x-points
    ChebfunVec fit(const API::IFunction& f ) const;
    /// Test an array of chebfun coefficients for convergence
    bool isConverged(const std::vector<double>& a, double maxA = 0.0);

    /// Evaluate a function
    double eval(double x, const std::vector<double> &p) const;
    /// Evaluate a function
    void evalVector(const std::vector<double> &x, const std::vector<double> &p, std::vector<double> &res) const;
    /// Evaluate a function
    std::vector<double> evalVector(const std::vector<double> &x, const std::vector<double> &p) const;
    /// Evaluate a function for a range of x-values.
    template<class XIter, class ResIter>
    void evalIter(XIter xbegin, XIter xend, const std::vector<double> &p, ResIter res) const;
    /// Calculate the derivative
    void derivative(const std::vector<double>& a, std::vector<double>& aout) const;
    /// Calculate the integral
    boost::shared_ptr<ChebfunBase> integral(const std::vector<double>& a, std::vector<double>& aout) const;
    std::vector<double> ChebfunBase::roots(const std::vector<double>& p) const;

    /// Fit a function until full convergence
    static boost::shared_ptr<ChebfunBase> bestFit(double start, double end, ChebfunFunctionType, ChebfunVec& p, ChebfunVec& a, double maxA = 0.0, double tolerance = 0.0, size_t maxSize = 0 );
    /// Fit a function until full convergence
    static boost::shared_ptr<ChebfunBase> bestFit(double start, double end,const API::IFunction&, ChebfunVec& p, ChebfunVec& a, double maxA = 0.0, double tolerance = 0.0, size_t maxSize = 0 );
    /// Tolerance for comparing doubles
    double tolerance() {return m_tolerance;}

    std::vector<double> linspace(size_t n) const;
    /// Get an interpolating matrix
    GSLMatrix createInterpolatingMatrix(const std::vector<double> &x, bool isZeroOutside = false) const;
    GSLMatrix createConvolutionMatrix(ChebfunFunctionType fun) const;
    std::vector<double> smooth(const std::vector<double> &xvalues, const std::vector<double> &yvalues) const;

private:
    /// Private assingment operator to stress the immutability of ChebfunBase.
    ChebfunBase& operator=( const ChebfunBase& other );
    /// Calculate the x-values based on the (start,end) interval.
    void calcX();
    /// Calculate the integration weights
    void calcIntegrationWeights() const;

    /// Calculate function values at odd-valued indices of chebfun x-points
    ChebfunVec fitOdd(ChebfunFunctionType f, ChebfunVec& p) const;
    /// Calculate function values at odd-valued indices of chebfun x-points
    ChebfunVec fitOdd(const API::IFunction& f, ChebfunVec& p) const;
    /// Test an array of chebfun coefficients for convergence
    static bool hasConverged(const std::vector<double>& a, double maxA, double tolerance);
    template<class FunctionType>
    static boost::shared_ptr<ChebfunBase> bestFitTempl(double start, double end, FunctionType f, ChebfunVec& p, ChebfunVec& a , double maxA, double tolerance, size_t maxSize);

    /// Actual tolerance in comparing doubles
    const double m_tolerance;
    /// Number of points on the x-axis.
    size_t m_n;
    /// Start of the interval
    double m_start;
    /// End of the interval
    double m_end;
    /// The x-points
    std::vector<double> m_x;
    /// The barycentric weights.
    std::vector<double> m_bw;
    /// Integration weights
    mutable std::vector<double> m_integrationWeights;
    /// Maximum tolerance in comparing doubles
    static const double g_tolerance;
    /// Maximum number of (x) points in a base.
    static const size_t g_maxNumberPoints;

};

/**
 * Evaluate a function for a range of x-values.
 * @param xbegin :: Iterator of the start of a range of x-values
 * @param xend :: Iterator of the end of a range of x-values
 * @param p :: The function parameters.
 * @param res :: Iterator to the start of the results container. The size of the container must
 *    not be smaller than distance(xbegin,xend).
 */
template<class XIter, class ResIter>
void ChebfunBase::evalIter(XIter xbegin, XIter xend, const std::vector<double> &p, ResIter res) const
{
    using namespace std::placeholders;
    std::transform( xbegin, xend, res, std::bind(&ChebfunBase::eval, this, _1, p) );
}

typedef boost::shared_ptr<ChebfunBase> ChebfunBase_sptr;

} // CurveFitting
} // Mantid


#endif // MANTID_CURVEFITTING_CHEBFUNBASE_H
