#ifndef MANTID_GEOMETRY_INSTRUMENT_NEARESTNEIGHBOURS
#define MANTID_GEOMETRY_INSTRUMENT_NEARESTNEIGHBOURS

#include "MantidKernel/System.h"

#include "boost/graph/adjacency_list.hpp"
#include "boost/unordered_map.hpp"
#include <boost/shared_ptr.hpp>

namespace Mantid
{
namespace Geometry
{

// Forward Declarations
class IInstrument;
class IComponent;

/**
*  This class is used to find the nearest neighbours of a detector in the instrument
*  geometry. This class can be queried through calls to the getNeighbours() function
*  on a Detector object.
*
*  This class uses the ANN Library, from David M Mount and Sunil Arya which is incorporated
*  into Mantid's Kernel module. Mantid uses version 1.1.2 of this library.
*  ANN is available from <http://www.cs.umd.edu/~mount/ANN/> and is released under the GNU LGPL.
*  
*  Known potential issue: boost's graph has an issue that may cause compilation errors in some
*  circumstances in the current version of boost used by Mantid (1.43) based on tr1::tie. This
*  issue is fixed in later versions of boost, but should in theory be guarded against by this
*  file only being included in ParameterMap.
*
*  @author Michael Whitty, STFC
*  @date 06/12/2010
*
*  Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
*
*  This file is part of Mantid.
*
*  Mantid is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 3 of the License, or
*  (at your option) any later version.
*
*  Mantid is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*  File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
*  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport NearestNeighbours
{
public:
  /// Default (empty) constructor
  NearestNeighbours(boost::shared_ptr<const Mantid::Geometry::IInstrument> instrument);
  /// Default (empty) destructor
  virtual ~NearestNeighbours() {};

  /// Clear the graph and assosciated elements
  void clear();
  /// Build and populate the graph object
  void build();
  /// whether or not the graph has been created
  bool isPopulated() const;
  /// query the graph for the eight nearest neighbours to specified detector
  std::map<int, double> neighbours(const int detID) const;
  /// as above, but filter based on the distance given
  std::map<int, double> neighbours(const int detID, const double radius) const;
  /// as above, but taking input as an IComponent pointer
  std::map<int, double> neighbours(const IComponent *component, const double radius=0.0) const;

private:
  /// typedef for Graph object used to hold the calculated information
  typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS,
    boost::property<boost::vertex_name_t, int>,
    boost::property<boost::edge_name_t, double>
  > Graph;
  /// Vertex descriptor object for Graph
  typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
  /// map object of int to int
  typedef boost::unordered_map<int,int> MapII;
  /// map object of int to Graph Vertex descriptor
  typedef boost::unordered_map<int,Vertex> MapIV;

  /// populates the graph with the nodes (detectors with id) and edges (neighbour links with distances)
  void populate();

  /// pointer to the instrument object from which to build the graph
  boost::shared_ptr<const Mantid::Geometry::IInstrument> m_instrument;
  /// number of neighbours to find. always set to 8.
  int m_noNeighbours;
  /// flag for whether the graph has been populated
  bool m_isPopulated;
  /// map between the DetectorID and the Graph node descriptor
  MapIV m_detIDtoVertex;
  /// boost::graph object
  Graph m_graph;
  /// property map holding the node's related DetectorID's
  boost::property_map<Graph, boost::vertex_name_t>::type m_vertexID;
  /// property map holding the edge's related Distance value.
  boost::property_map<Graph, boost::edge_name_t>::type m_edgeLength;

};

/// Typedef for shared pointer to the NearestNeighbours class
typedef boost::shared_ptr<Mantid::Geometry::NearestNeighbours> NearestNeighbours_sptr;
/// Typedef for constant shared pointer to the NearestNeighbours class
typedef boost::shared_ptr<const Mantid::Geometry::NearestNeighbours> NearestNeighbours_const_sptr;

} // namespace Geometry
} // namespace Mantid

#endif
