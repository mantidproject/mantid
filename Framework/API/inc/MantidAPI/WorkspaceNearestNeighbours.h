// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_INSTRUMENT_NEARESTNEIGHBOURS
#define MANTID_GEOMETRY_INSTRUMENT_NEARESTNEIGHBOURS

#include "MantidAPI/DllConfig.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/V3D.h"
// Boost graphing
#ifndef Q_MOC_RUN
#include <boost/graph/adjacency_list.hpp>
#include <boost/shared_ptr.hpp>
#include <unordered_map>
#endif

namespace Mantid {
namespace Geometry {
class Instrument;
class IDetector;
} // namespace Geometry
namespace API {
class SpectrumInfo;
/**
 * This class is not intended for direct use. Use WorkspaceNearestNeighbourInfo
 * instead!
 *
 * This class is used to find the nearest neighbours of a detector in the
 * instrument geometry. This class can be queried through calls to the
 * getNeighbours() function on a Detector object.
 *
 * This class uses the ANN Library, from David M Mount and Sunil Arya which
 * is incorporated into Mantid's Kernel module. Mantid uses version 1.1.2
 * of this library.
 * ANN is available from <http://www.cs.umd.edu/~mount/ANN/> and is released
 * under the GNU LGPL.
 *
 * Known potential issue: boost's graph has an issue that may cause compilation
 * errors in some circumstances in the current version of boost used by
 * Mantid (1.43) based on tr1::tie. This issue is fixed in later versions
 * of boost, but should in theory be guarded against by this file only being
 * included in ParameterMap.
 *
 *
 */
class MANTID_API_DLL WorkspaceNearestNeighbours {
public:
  WorkspaceNearestNeighbours(int nNeighbours, const SpectrumInfo &spectrumInfo,
                             std::vector<specnum_t> spectrumNumbers,
                             bool ignoreMaskedDetectors = false);

  // Neighbouring spectra by radius
  std::map<specnum_t, Mantid::Kernel::V3D>
  neighboursInRadius(specnum_t spectrum, double radius = 0.0) const;

  // Neighbouring spectra by
  std::map<specnum_t, Mantid::Kernel::V3D> neighbours(specnum_t spectrum) const;

protected:
  std::vector<size_t> getSpectraDetectors();

private:
  /// A reference to the SpectrumInfo
  const SpectrumInfo &m_spectrumInfo;
  /// Vector of spectrum numbers
  const std::vector<specnum_t> m_spectrumNumbers;

  /// typedef for Graph object used to hold the calculated information
  using Graph = boost::adjacency_list<
      boost::vecS, boost::vecS, boost::directedS,
      boost::property<boost::vertex_name_t, int64_t>,
      boost::property<boost::edge_name_t, Mantid::Kernel::V3D>>;
  /// Vertex descriptor object for Graph
  using Vertex = boost::graph_traits<Graph>::vertex_descriptor;
  /// map object of int to Graph Vertex descriptor
  using MapIV = std::unordered_map<specnum_t, Vertex>;

  /// Construct the graph based on the given number of neighbours and the
  /// current instument and spectra-detector mapping
  void build(const int noNeighbours);
  /// Query the graph for the default number of nearest neighbours to specified
  /// detector
  std::map<specnum_t, Mantid::Kernel::V3D>
  defaultNeighbours(const specnum_t spectrum) const;
  /// The current number of nearest neighbours
  int m_noNeighbours;
  /// The largest value of the distance to a nearest neighbour
  double m_cutoff;
  /// map between the DetectorID and the Graph node descriptor
  MapIV m_specToVertex;
  /// boost::graph object
  Graph m_graph;
  /// property map holding the node's related DetectorID's
  boost::property_map<Graph, boost::vertex_name_t>::type m_vertexID;
  /// property map holding the edge's related Distance value.
  boost::property_map<Graph, boost::edge_name_t>::type m_edgeLength;
  /// V3D for scaling
  Kernel::V3D m_scale;
  /// Cached radius value. used to avoid uncessary recalculations.
  mutable double m_radius;
  /// Flag indicating that masked detectors should be ignored
  bool m_bIgnoreMaskedDetectors;
};

} // namespace API
} // namespace Mantid

#endif
