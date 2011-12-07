#include "MantidAlgorithms/WeightingStrategy.h"
#include "MantidKernel/System.h"

#include <stdexcept>
#include <cmath>

namespace Mantid
{
  namespace Algorithms
  {

    //----------------------------------------------------------------------------
    // Weighting Implementations
    //----------------------------------------------------------------------------

    /**
    Constructor
    @param cutOff : radius cutoff
    */
    WeightingStrategy::WeightingStrategy(const double cutOff) : m_cutOff(cutOff)
    {
    };

    /// Constructor
    WeightingStrategy::WeightingStrategy() : m_cutOff(0)
    {
    };

    /// Destructor
    WeightingStrategy::~WeightingStrategy()
    {
    }

    //----------------------------------------------------------------------------
    // Flat Weighting Implementations
    //----------------------------------------------------------------------------

    /// Constructor
    FlatWeighting::FlatWeighting()
    {
    }

    /// Destructor
    FlatWeighting::~FlatWeighting()
    {
    }

    /**
    Calculate the weight at distance from epicenter. Always returns 1 for this implementation
    @param adjX : The number of Y (vertical) adjacent pixels to average together
    @param ix : current index x
    @param adjY : The number of X (vertical) adjacent pixels to average together
    @param iy : current index y
    @return weight which is always 1
    */
    double FlatWeighting::weightAt(const double&,const double&, const double&, const double&)
    {
      return 1;
    }

    /**
    Calculate the weight at distance from epicenter. Always returns 1
    @param distance : absolute distance from epicenter
    @return 1
    */
    double FlatWeighting::weightAt(const double&)
    { 
      return 1;
    }

    //----------------------------------------------------------------------------
    // Linear Weighting Implementations
    //----------------------------------------------------------------------------

    /**
    Constructor
    @cutOff : cutoff radius
    */
    LinearWeighting::LinearWeighting(const double cutOff) : WeightingStrategy(cutOff)
    {
    }

    /// Destructor
    LinearWeighting::~LinearWeighting()
    {
    }

    /**
    Calculate the weight at distance from epicenter. Uses linear scaling based on distance from epicenter.
    @param distance : absolute distance from epicenter
    @return weighting
    */
    double LinearWeighting::weightAt(const double& distance)
    {
      return 1 - (distance/m_cutOff);
    }

    /**
    Calculate the weight at distance from epicenter using linear scaling.
    @param adjX : The number of Y (vertical) adjacent pixels to average together
    @param ix : current index x
    @param adjY : The number of X (vertical) adjacent pixels to average together
    @param iy : current index y
    @return weight calculated
    */
    double LinearWeighting::weightAt(const double& adjX,const double& ix, const double& adjY, const double& iy)
    {
      return 1 - (std::sqrt(ix*ix + iy*iy)/std::sqrt(adjX*adjX + adjY*adjY));
    }


    //----------------------------------------------------------------------------
    // Parabolic Weighting Implementations
    //----------------------------------------------------------------------------

    /// Constructor
    ParabolicWeighting::ParabolicWeighting() : WeightingStrategy()
    {
    }

    /// Destructor
    ParabolicWeighting::~ParabolicWeighting()
    {
    }

    /**
    Implementation doesn't make sense on this type.
    @param distance : absolute distance from epicenter
    @return weighting
    @throws runtime_error if used.
    */
    double ParabolicWeighting::weightAt(const double&)
    {
      throw std::runtime_error("Parabolic weighting cannot be calculated based on a radius cut-off alone.");
    }

    /**
    Calculate the weight at distance from epicenter using parabolic scaling
    @param adjX : The number of Y (vertical) adjacent pixels to average together
    @param ix : current index x
    @param adjY : The number of X (vertical) adjacent pixels to average together
    @param iy : current index y
    @return weight calculated
    */
    double ParabolicWeighting::weightAt(const double& adjX,const double& ix, const double& adjY, const double& iy)
    {
      return static_cast<double>(adjX - std::abs(ix) + adjY - std::abs(iy) + 1);
    }

    //----------------------------------------------------------------------------
    // Null Weighting Implementations
    //----------------------------------------------------------------------------

    /// Constructor
    NullWeighting::NullWeighting() : WeightingStrategy()
    {
    }

    /// Destructor
    NullWeighting::~NullWeighting()
    {
    }

    /**
    Calculate the weight at distance from epicenter. Always throws.
    @param distance : absolute distance from epicenter
    @throw runtime_error if used
    */
    double NullWeighting::weightAt(const double&)
    {
      throw std::runtime_error("NullWeighting strategy cannot be used to evaluate weights.");
    }

    /**
    Calculate the weight at distance from epicenter. Always throws.
    @param adjX : The number of Y (vertical) adjacent pixels to average together
    @param ix : current index x
    @param adjY : The number of X (vertical) adjacent pixels to average together
    @param iy : current index y
    @throws runtime_error if called
    */
    double NullWeighting::weightAt(const double&,const double&, const double&, const double&)
    {
      throw std::runtime_error("NullWeighting strategy cannot be used to evaluate weights.");
    }

    //-------------------------------------------------------------------------
    // Gaussian Weighting Implementations
    //-------------------------------------------------------------------------

    GaussianWeighting1D::GaussianWeighting1D(double cutOff, double sigma) : WeightingStrategy(cutOff)
    {
      if(cutOff < 0)
      {
        throw std::invalid_argument("GassianWeighting expects unsigned cutOff input");
      }
      if(sigma < 0)
      {
        throw std::invalid_argument("GassianWeighting expects unsigned standard deviation input");
      }

      init(sigma);
    }

    GaussianWeighting1D::GaussianWeighting1D(double sigma) : WeightingStrategy(0)
    {
      if(sigma < 0)
      {
        throw std::invalid_argument("GassianWeighting expects unsigned standard deviation input");
      }

      init(sigma);
    }

    void GaussianWeighting1D::init(const double sigma)
    {
      m_coeff = 1/((std::sqrt(2 *  M_PI)) * sigma);
      m_twiceSigmaSquared = 2 * sigma * sigma;
    }

    GaussianWeighting1D::~GaussianWeighting1D()
    {
    }

    double GaussianWeighting1D::weightAt(const double& distance)
    {
      double normalisedDistance = distance/m_cutOff;
      return calculateGaussian(normalisedDistance*normalisedDistance);
    }

    double GaussianWeighting1D::weightAt(const double& adjX,const double& ix, const double& adjY, const double& iy)
    {
      double normalisedDistanceSq = (ix*ix + iy*iy) / (adjX*adjX + adjY*adjY);
      return calculateGaussian(normalisedDistanceSq);
    }

    double GaussianWeighting1D::calculateGaussian(const double normalisedDistanceSq)
    {
      return m_coeff * std::exp(-normalisedDistanceSq / m_twiceSigmaSquared);
    }


  } // namespace Mantid
} // namespace Algorithms