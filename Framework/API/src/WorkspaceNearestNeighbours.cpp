// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/WorkspaceNearestNeighbours.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Objects/BoundingBox.h"
// Nearest neighbours library
#include "MantidKernel/ANN/ANN.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Timer.h"

namespace Mantid {
using namespace Geometry;
namespace API {
using Kernel::V3D;
using Mantid::detid_t;

/**
 * Constructor
 * @param nNeighbours :: Number of neighbours to use
 * @param spectrumInfo :: Reference to the SpectrumInfo of the underlying
 * workspace
 * @param spectrumNumbers :: Vector of spectrum numbers, defining the ordering
 * of spectra
 * @param ignoreMaskedDetectors :: flag indicating that masked detectors should
 * be ignored.
 */
WorkspaceNearestNeighbours::WorkspaceNearestNeighbours(
    int nNeighbours, const SpectrumInfo &spectrumInfo,
    std::vector<specnum_t> spectrumNumbers, bool ignoreMaskedDetectors)
    : m_spectrumInfo(spectrumInfo),
      m_spectrumNumbers(std::move(spectrumNumbers)),
      m_noNeighbours(nNeighbours),
      m_cutoff(std::numeric_limits<double>::lowest()), m_radius(0.),
      m_bIgnoreMaskedDetectors(ignoreMaskedDetectors) {
  this->build(m_noNeighbours);
}

/**
 * Returns a map of the spectrum numbers to the distances for the nearest
 * neighbours.
 * @param spectrum :: Spectrum No of the central pixel
 * @return map of Detector ID's to distance
 */
std::map<specnum_t, V3D>
WorkspaceNearestNeighbours::neighbours(const specnum_t spectrum) const {
  return defaultNeighbours(spectrum);
}

/**
 * Returns a map of the spectrum numbers to the distances for the nearest
 * neighbours.
 * @param spectrum :: Spectrum No of the central pixel
 * @param radius :: cut-off distance for detector list to returns
 * @return map of Detector ID's to distance
 * @throw NotFoundError if component is not recognised as a detector
 */
std::map<specnum_t, V3D>
WorkspaceNearestNeighbours::neighboursInRadius(const specnum_t spectrum,
                                               const double radius) const {
  // If the radius is stupid then don't let it continue as well be stuck forever
  if (radius < 0.0 || radius > 10.0) {
    throw std::invalid_argument(
        "NearestNeighbours::neighbours - Invalid radius parameter.");
  }

  std::map<specnum_t, V3D> result;
  if (radius == 0.0) {
    const int eightNearest = 8;
    if (m_noNeighbours != eightNearest) {
      // Note: Should be able to do this better but time constraints for the
      // moment mean that
      // it is necessary.
      // Cast is necessary as the user should see this as a const member
      const_cast<WorkspaceNearestNeighbours *>(this)->build(eightNearest);
    }
    result = defaultNeighbours(spectrum);
  } else if (radius > m_cutoff && m_radius != radius) {
    // We might have to see how efficient this ends up being.
    int neighbours = m_noNeighbours + 1;
    while (true) {
      try {
        const_cast<WorkspaceNearestNeighbours *>(this)->build(neighbours);
      } catch (std::invalid_argument &) {
        break;
      }
      if (radius < m_cutoff)
        break;
      else
        neighbours += 1;
    }
  }
  m_radius = radius;

  std::map<detid_t, V3D> nearest = defaultNeighbours(spectrum);
  for (std::map<specnum_t, V3D>::const_iterator cit = nearest.begin();
       cit != nearest.end(); ++cit) {
    if (cit->second.norm() <= radius) {
      result[cit->first] = cit->second;
    }
  }
  return result;
}

//--------------------------------------------------------------------------
// Private member functions
//--------------------------------------------------------------------------
/**
 * Builds a map based on the given number of neighbours
 * @param noNeighbours :: The number of nearest neighbours to use to build
 * the graph
 */
void WorkspaceNearestNeighbours::build(const int noNeighbours) {
  const auto indices = getSpectraDetectors();
  if (indices.empty()) {
    throw std::runtime_error(
        "NearestNeighbours::build - Cannot find any spectra");
  }
  const int nspectra =
      static_cast<int>(indices.size()); // ANN only deals with integers
  if (noNeighbours >= nspectra) {
    throw std::invalid_argument(
        "NearestNeighbours::build - Invalid number of neighbours");
  }

  // Clear current
  m_graph.clear();
  m_specToVertex.clear();
  m_noNeighbours = noNeighbours;

  BoundingBox bbox;
  // Base the scaling on the first detector, should be adequate but we can look
  // at this
  const auto &firstDet = m_spectrumInfo.detector(indices.front());
  firstDet.getBoundingBox(bbox);
  m_scale = V3D(bbox.width());
  ANNpointArray dataPoints = annAllocPts(nspectra, 3);
  MapIV pointNoToVertex;

  int pointNo = 0;
  for (const auto i : indices) {
    const specnum_t spectrum = m_spectrumNumbers[i];
    V3D pos = m_spectrumInfo.position(i) / m_scale;
    dataPoints[pointNo][0] = pos.X();
    dataPoints[pointNo][1] = pos.Y();
    dataPoints[pointNo][2] = pos.Z();
    Vertex vertex = boost::add_vertex(spectrum, m_graph);
    pointNoToVertex[pointNo] = vertex;
    m_specToVertex[spectrum] = vertex;
    ++pointNo;
  }

  auto annTree = std::make_unique<ANNkd_tree>(dataPoints, nspectra, 3);
  pointNo = 0;
  // Run the nearest neighbour search on each detector, reusing the arrays
  // Set size initially to avoid array index error when testing in debug mode
  std::vector<ANNidx> nnIndexList(m_noNeighbours);
  std::vector<ANNdist> nnDistList(m_noNeighbours);

  for (const auto idx : indices) {
    ANNpoint scaledPos = dataPoints[pointNo];
    annTree->annkSearch(scaledPos,      // Point to search nearest neighbours of
                        m_noNeighbours, // Number of neighbours to find (8)
                        nnIndexList.data(), // Index list of results
                        nnDistList.data(), // List of distances to each of these
                        0.0 // Error bound (?) is this the radius to search in?
    );
    // The distances that are returned are in our scaled coordinate
    // system. We store the real space ones.
    const V3D realPos = V3D(scaledPos[0], scaledPos[1], scaledPos[2]) * m_scale;
    for (int i = 0; i < m_noNeighbours; i++) {
      ANNidx index = nnIndexList[i];
      V3D neighbour = V3D(dataPoints[index][0], dataPoints[index][1],
                          dataPoints[index][2]) *
                      m_scale;
      V3D distance = neighbour - realPos;
      double separation = distance.norm();
      boost::add_edge(m_specToVertex[m_spectrumNumbers[idx]], // from
                      pointNoToVertex[index],                 // to
                      distance, m_graph);
      if (separation > m_cutoff) {
        m_cutoff = separation;
      }
    }
    pointNo++;
  }
  annDeallocPts(dataPoints);
  annClose();
  pointNoToVertex.clear();

  m_vertexID = get(boost::vertex_name, m_graph);
  m_edgeLength = get(boost::edge_name, m_graph);
}

/**
 * Returns a map of the spectrum numbers to the nearest detectors and their
 * distance from the detector specified in the argument.
 * @param spectrum :: The spectrum number
 * @return map of detID to distance
 * @throw NotFoundError if detector ID is not recognised
 */
std::map<specnum_t, V3D>
WorkspaceNearestNeighbours::defaultNeighbours(const specnum_t spectrum) const {
  auto vertex = m_specToVertex.find(spectrum);

  if (vertex != m_specToVertex.end()) {
    std::map<specnum_t, V3D> result;
    std::pair<Graph::adjacency_iterator, Graph::adjacency_iterator> adjacent =
        boost::adjacent_vertices(vertex->second, m_graph);
    Graph::adjacency_iterator adjIt;
    for (adjIt = adjacent.first; adjIt != adjacent.second; adjIt++) {
      Vertex nearest = (*adjIt);
      specnum_t nrSpec = specnum_t(m_vertexID[nearest]);
      std::pair<Graph::edge_descriptor, bool> nrEd =
          boost::edge(vertex->second, nearest, m_graph);
      result[nrSpec] = m_edgeLength[nrEd.first];
    }
    return result;
  } else {
    throw Mantid::Kernel::Exception::NotFoundError(
        "NearestNeighbours: Unable to find spectrum in vertex map", spectrum);
  }
}

/// Returns the list of valid spectrum indices
std::vector<size_t> WorkspaceNearestNeighbours::getSpectraDetectors() {
  std::vector<size_t> indices;
  const auto nSpec = m_spectrumNumbers.size();
  indices.reserve(nSpec);
  for (size_t i = 0; i < nSpec; ++i) {
    // Always ignore monitors and ignore masked detectors if requested.
    const bool heedMasking =
        m_bIgnoreMaskedDetectors && m_spectrumInfo.isMasked(i);
    if (!m_spectrumInfo.isMonitor(i) && !heedMasking) {
      indices.emplace_back(i);
    }
  }
  return indices;
}
} // namespace API
} // namespace Mantid
