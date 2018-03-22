#ifndef MANTID_ALGORITHMS_SmoothNeighbours_H_
#define MANTID_ALGORITHMS_SmoothNeighbours_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/WeightingStrategy.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include <boost/scoped_ptr.hpp>

namespace Mantid {
namespace Algorithms {
using SpectraDistanceMap = std::map<specnum_t, Mantid::Kernel::V3D>;

/*
Filters spectra detector list by radius.
*/
class DLLExport RadiusFilter {
public:
  /**
  Constructor
  @param cutoff : radius cutoff for filtering
  */
  RadiusFilter(double cutoff) : m_cutoff(cutoff) {
    if (cutoff < 0) {
      throw std::invalid_argument(
          "RadiusFilter - Cannot have a negative cutoff.");
    }
  }
  /**
  Apply the filtering based on radius.
  @param unfiltered : unfiltered spectra-distance map.
  @return filtered spectra-distance map.
  */
  SpectraDistanceMap apply(SpectraDistanceMap &unfiltered) const {
    SpectraDistanceMap neighbSpectra;
    double cutoff{m_cutoff};
    std::copy_if(
        unfiltered.begin(), unfiltered.end(),
        std::inserter(neighbSpectra, neighbSpectra.end()),
        [cutoff](
            const std::pair<specnum_t, Mantid::Kernel::V3D> &spectraDistance) {
          return spectraDistance.second.norm() <= cutoff;
        });
    return neighbSpectra;
  }

private:
  /// Radius cutoff.
  double m_cutoff;
};

/** Smooth neighboring pixels.

  @authors Janik Zikovsky, Vickie Lynch, SNS
  @date Oct 2010

  Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport SmoothNeighbours : public API::Algorithm {
public:
  /// Default constructor
  SmoothNeighbours();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "SmoothNeighbours"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Perform a moving-average smoothing by summing spectra of nearest "
           "neighbours over the face of detectors.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return (1); }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "Transforms\\Smoothing";
  }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;

  void execWorkspace2D();
  void execEvent(Mantid::DataObjects::EventWorkspace_sptr ws);
  void findNeighboursRectangular();
  void findNeighboursUbiqutious();
  Mantid::Geometry::Instrument_const_sptr fetchInstrument() const;

  /// Sets the weighting stragegy.
  void setWeightingStrategy(const std::string strategyName, double &cutOff);
  /// Translate the entered radius into meters.
  double translateToMeters(const std::string radiusUnits,
                           const double &enteredRadius);

  /// Build the instrument/detector setup in workspace
  void setupNewInstrument(API::MatrixWorkspace_sptr outws);

  /// Build the instrument/detector setup in workspace
  void spreadPixels(API::MatrixWorkspace_sptr outws);

  /// Non rectangular detector group name
  static const std::string NON_UNIFORM_GROUP;
  /// Rectangular detector group name
  static const std::string RECTANGULAR_GROUP;
  /// Input workspace name
  static const std::string INPUT_WORKSPACE;
  /// Number to sum
  int AdjX;
  /// Number to sum
  int AdjY;
  /// Edge pixels to ignore
  int Edge;
  /// Radius to search nearest neighbours
  double Radius;
  /// Number of neighbours
  int nNeighbours;
  /// Weight the neighbours during summing
  boost::scoped_ptr<WeightingStrategy> WeightedSum;
  /// PreserveEvents
  bool PreserveEvents;
  ///  expand by pixel IDs
  bool expandSumAllPixels;
  /// number of output workspace pixels
  size_t outWI;

  /// Input workspace
  Mantid::API::MatrixWorkspace_sptr inWS;

  /// Each neighbours is specified as a pair with workspace index, weight.
  using weightedNeighbour = std::pair<size_t, double>;

  /// Vector of list of neighbours (with weight) for each workspace index.
  std::vector<std::vector<weightedNeighbour>> m_neighbours;

  /// Progress reporter
  std::unique_ptr<Mantid::API::Progress> m_progress = nullptr;
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_SmoothNeighbours_H_*/
