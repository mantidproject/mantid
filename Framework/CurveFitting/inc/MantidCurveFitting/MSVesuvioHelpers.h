#ifndef MANTID_CURVEFITTING_MSVESUVIOHELPERS_H
#define MANTID_CURVEFITTING_MSVESUVIOHELPERS_H

#include "MantidKernel/normal_distribution.h"
#include <random>
#include <vector>

namespace Mantid {
namespace CurveFitting {
namespace MSVesuvioHelper {
// Absorption energy for double-difference gold foil
double finalEnergyAuDD(const double randv);
// Absorption energy for gold foil YAP
double finalEnergyAuYap(const double randv);
// Absorption energy for uranium
double finalEnergyUranium(const double randv);

// Ties together random numbers with various probability distributions
class RandomVariateGenerator {
public:
  RandomVariateGenerator(const int seed);
  /// Returns a flat random number between 0.0 & 1.0
  double flat();
  /// Returns a random number distributed  by a normal distribution
  double gaussian(const double mean, const double sigma);

private:
  RandomVariateGenerator();
  std::mt19937 m_engine;
};

// Stores counts for each scatter order
// for a "run" of a given number of events
struct Simulation {
  Simulation(const size_t order, const size_t ntimes);

  std::vector<std::vector<double>> counts;
  size_t maxorder;
};
// Stores counts for each scatter order with errors
struct SimulationWithErrors {
  SimulationWithErrors(const size_t order, const size_t ntimes)
      : sim(order, ntimes), errors(order, std::vector<double>(ntimes)) {}

  void normalise();
  Simulation sim;
  std::vector<std::vector<double>> errors;
};

// Accumulates and averages the results of a set of simulations
struct SimulationAggregator {
  SimulationAggregator(const size_t nruns);

  // Creates a placeholder for a new simulation
  Simulation &newSimulation(const size_t order, const size_t ntimes);
  SimulationWithErrors average() const;

  std::vector<Simulation> results;
};
}
}
}

#endif // MANTID_CURVEFITTING_MSVESUVIOHELPERS_H
