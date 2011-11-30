#include "MantidAlgorithms/WeightingStrategy.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

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


  } // namespace Mantid
} // namespace Algorithms