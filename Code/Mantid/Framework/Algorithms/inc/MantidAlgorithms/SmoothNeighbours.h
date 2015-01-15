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
typedef std::map<specid_t, Mantid::Kernel::V3D> SpectraDistanceMap;

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
    SpectraDistanceMap::iterator it = unfiltered.begin();
    SpectraDistanceMap neighbSpectra;
    while (it != unfiltered.end()) {
      // Strip out spectra that don't meet the radius criteria.
      if (it->second.norm() <= m_cutoff) {
        neighbSpectra.insert(std::make_pair(it->first, it->second));
      }
      it++;
    }
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
  /// Destructor
  virtual ~SmoothNeighbours(){};
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "SmoothNeighbours"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Perform a moving-average smoothing by summing spectra of nearest "
           "neighbours over the face of detectors.";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return (1); }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Transforms\\Smoothing"; }

private:
  // Overridden Algorithm methods
  void init();
  void exec();

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
  typedef std::pair<size_t, double> weightedNeighbour;

  /// Vector of list of neighbours (with weight) for each workspace index.
  std::vector<std::vector<weightedNeighbour>> m_neighbours;

  /// Progress reporter
  Mantid::API::Progress *m_prog;
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_SmoothNeighbours_H_*/
