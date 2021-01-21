// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAlgorithms/DllConfig.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace Algorithms {

/** WeightingStrategy :

  Abstract weighting strategy, which can be applied to calculate individual
  weights for each pixel based upon disance from epicenter. Generated for use
  with SmoothNeighbours.

  @date 2011-11-30
*/
class MANTID_ALGORITHMS_DLL WeightingStrategy {
public:
  /// Constructor
  WeightingStrategy(const double cutOff);
  /// Constructor
  WeightingStrategy();
  /// Destructor
  virtual ~WeightingStrategy() = default;
  /**
  Calculate the weight at distance from epicenter.
  @param distance : difference between the central detector location and the
  nearest neighbour
  @return calculated weight
  */
  virtual double weightAt(const Mantid::Kernel::V3D &distance) = 0;

  /**
  Calculate the weight at distance from epicenter.
  @param adjX : The number of Y (vertical) adjacent pixels to average together
  @param ix : current index x
  @param adjY : The number of X (vertical) adjacent pixels to average together
  @param iy : current index y
  */
  virtual double weightAt(const double &adjX, const double &ix, const double &adjY, const double &iy) = 0;

protected:
  /// Cutoff member.
  double m_cutOff;
};

/*
Flat (no weighting) strategy. Concrete WeightingStrategy
*/
class MANTID_ALGORITHMS_DLL FlatWeighting : public WeightingStrategy {
public:
  double weightAt(const double &, const double &, const double &, const double &) override;
  double weightAt(const Mantid::Kernel::V3D &) override;
};

/*
Linear weighting strategy.
*/
class MANTID_ALGORITHMS_DLL LinearWeighting : public WeightingStrategy {
public:
  LinearWeighting(const double cutOff);
  double weightAt(const Mantid::Kernel::V3D &) override;
  double weightAt(const double &adjX, const double &ix, const double &adjY, const double &iy) override;
};

/*
Parabolic weighting strategy.
*/
class MANTID_ALGORITHMS_DLL ParabolicWeighting : public WeightingStrategy {
public:
  ParabolicWeighting(const double cutOff);
  double weightAt(const Mantid::Kernel::V3D &) override;
  double weightAt(const double &adjX, const double &ix, const double &adjY, const double &iy) override;
};

/*
Null weighting strategy.
*/
class MANTID_ALGORITHMS_DLL NullWeighting : public WeightingStrategy {
public:
  double weightAt(const Mantid::Kernel::V3D &) override;
  double weightAt(const double &, const double &, const double &, const double &) override;
};

/*
Gaussian nD Strategy.

y = exp(-0.5*((r./p(1)).^2) where p = sqtr(2)*sigma
*/
class MANTID_ALGORITHMS_DLL GaussianWeightingnD : public WeightingStrategy {
public:
  GaussianWeightingnD(double cutOff, double sigma);
  double weightAt(const Mantid::Kernel::V3D &) override;
  double weightAt(const double &, const double &, const double &, const double &) override;

private:
  double calculateGaussian(const double normalisedDistanceSq);
  double m_twiceSigmaSquared;
};

} // namespace Algorithms
} // namespace Mantid
