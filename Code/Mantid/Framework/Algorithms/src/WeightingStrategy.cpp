#include "MantidAlgorithms/WeightingStrategy.h"
#include "MantidKernel/System.h"

#include <stdexcept>
#include <cmath>

namespace Mantid {
namespace Algorithms {

//----------------------------------------------------------------------------
// Weighting Implementations
//----------------------------------------------------------------------------

/**
Constructor
@param cutOff : radius cutoff
*/
WeightingStrategy::WeightingStrategy(const double cutOff) : m_cutOff(cutOff){};

/// Constructor
WeightingStrategy::WeightingStrategy() : m_cutOff(0){};

/// Destructor
WeightingStrategy::~WeightingStrategy() {}

//----------------------------------------------------------------------------
// Flat Weighting Implementations
//----------------------------------------------------------------------------

/// Constructor
FlatWeighting::FlatWeighting() {}

/// Destructor
FlatWeighting::~FlatWeighting() {}

/**
Calculate the weight at distance from epicenter. Always returns 1 for this
implementation
*/
double FlatWeighting::weightAt(const double &, const double &, const double &,
                               const double &) {
  return 1;
}

/**
Calculate the weight at distance from epicenter. Always returns 1
@return 1
*/
double FlatWeighting::weightAt(const Mantid::Kernel::V3D &) { return 1; }

//----------------------------------------------------------------------------
// Linear Weighting Implementations
//----------------------------------------------------------------------------

/**
Constructor
@param cutOff : cutoff radius
*/
LinearWeighting::LinearWeighting(const double cutOff)
    : WeightingStrategy(cutOff) {}

/// Destructor
LinearWeighting::~LinearWeighting() {}

/**
Calculate the weight at distance from epicenter. Uses linear scaling based on
distance from epicenter.
@param distance : absolute distance from epicenter
@return weighting
*/
double LinearWeighting::weightAt(const Mantid::Kernel::V3D &distance) {
  return 1 - (distance.norm() / m_cutOff);
}

/**
Calculate the weight at distance from epicenter using linear scaling.
@param adjX : The number of Y (vertical) adjacent pixels to average together
@param ix : current index x
@param adjY : The number of X (vertical) adjacent pixels to average together
@param iy : current index y
@return weight calculated
*/
double LinearWeighting::weightAt(const double &adjX, const double &ix,
                                 const double &adjY, const double &iy) {
  return 1 -
         (std::sqrt(ix * ix + iy * iy) / std::sqrt(adjX * adjX + adjY * adjY));
}

//----------------------------------------------------------------------------
// Parabolic Weighting Implementations
//----------------------------------------------------------------------------

/** Constructor
@param cutOff : distance cutOff
*/
ParabolicWeighting::ParabolicWeighting(const double cutOff)
    : WeightingStrategy(cutOff) {}

/// Destructor
ParabolicWeighting::~ParabolicWeighting() {}

/**
Implementation doesn't make sense on this type.
@param distance :
@return weighting
*/
double ParabolicWeighting::weightAt(const Mantid::Kernel::V3D &distance) {
  return static_cast<double>(m_cutOff - std::abs(distance.X()) + m_cutOff -
                             std::abs(distance.Y()) + 1);
}

/**
Calculate the weight at distance from epicenter using parabolic scaling
@param adjX : The number of Y (vertical) adjacent pixels to average together
@param ix : current index x
@param adjY : The number of X (vertical) adjacent pixels to average together
@param iy : current index y
@return weight calculated
*/
double ParabolicWeighting::weightAt(const double &adjX, const double &ix,
                                    const double &adjY, const double &iy) {
  return static_cast<double>(adjX - std::abs(ix) + adjY - std::abs(iy) + 1);
}

//----------------------------------------------------------------------------
// Null Weighting Implementations
//----------------------------------------------------------------------------

/// Constructor
NullWeighting::NullWeighting() : WeightingStrategy() {}

/// Destructor
NullWeighting::~NullWeighting() {}

/**
Calculate the weight at distance from epicenter. Always throws.
@throw runtime_error if used
*/
double NullWeighting::weightAt(const Mantid::Kernel::V3D &) {
  throw std::runtime_error(
      "NullWeighting strategy cannot be used to evaluate weights.");
}

/**
Calculate the weight at distance from epicenter. Always throws.
@throws runtime_error if called
*/
double NullWeighting::weightAt(const double &, const double &, const double &,
                               const double &) {
  throw std::runtime_error(
      "NullWeighting strategy cannot be used to evaluate weights.");
}

//-------------------------------------------------------------------------
// Gaussian 2D Weighting Implementations
//-------------------------------------------------------------------------

/**
Constructor
@param cutOff : radius cut-off.
@param sigma : gaussian sigma value.
*/
GaussianWeightingnD::GaussianWeightingnD(double cutOff, double sigma)
    : WeightingStrategy(cutOff) {
  if (cutOff < 0) {
    throw std::invalid_argument(
        "GassianWeighting 1D expects unsigned cutOff input");
  }
  if (sigma < 0) {
    throw std::invalid_argument(
        "GassianWeighting 1D expects unsigned standard deviation input");
  }

  m_twiceSigmaSquared = 2 * sigma * sigma;
}

/// Destructor
GaussianWeightingnD::~GaussianWeightingnD() {}

/**
Calculate the weight at distance from epicenter. Uses linear scaling based on
distance from epicenter.
@param distance : absolute distance from epicenter
@return weighting
*/
double GaussianWeightingnD::weightAt(const Mantid::Kernel::V3D &distance) {
  /*
  distance.norm() = r
  r/R provides normalisation
  */
  double normalisedDistance =
      distance.norm() / m_cutOff; // Ensures 1 at the edges and zero in the
                                  // center no matter what the units are
  return calculateGaussian(normalisedDistance * normalisedDistance);
}

/**
Calculate the weight for rectangular detectors.
@param adjX : The number of Y (vertical) adjacent pixels to average together
@param ix : current index x
@param adjY : The number of X (vertical) adjacent pixels to average together
@param iy : current index y
@return weight calculated
*/
double GaussianWeightingnD::weightAt(const double &adjX, const double &ix,
                                     const double &adjY, const double &iy) {
  double normalisedDistanceSq =
      (ix * ix + iy * iy) / (adjX * adjX + adjY * adjY);
  return calculateGaussian(normalisedDistanceSq);
}

/**
calculateGaussian method so that same gaussian calculation can be run by
different consuming methods.
@param normalisedDistanceSq : r^2/cutOff^2
@return exp(-(r^2/cutOff^2)/(2*sigma^2))
*/
inline double
GaussianWeightingnD::calculateGaussian(const double normalisedDistanceSq) {
  return std::exp(-normalisedDistanceSq / m_twiceSigmaSquared);
}

} // namespace Mantid
} // namespace Algorithms
