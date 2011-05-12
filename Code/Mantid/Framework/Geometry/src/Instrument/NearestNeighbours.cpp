#include "MantidGeometry/Instrument/NearestNeighbours.h"

#include "MantidKernel/ANN/ANN.h"

#include "MantidGeometry/IInstrument.h"
#include "MantidGeometry/IDetector.h"

using namespace Mantid::Geometry;

/**
* @param instrument :: shared pointer to IInstrument object
*/
NearestNeighbours::NearestNeighbours(boost::shared_ptr<const Mantid::Geometry::IInstrument> instrument) : m_instrument(instrument), m_noNeighbours(8), m_isPopulated(false), m_scale(NULL)
{
}

/**
* @returns true if graph has been built, false otherwise.
*/
bool NearestNeighbours::isPopulated() const
{
  return m_isPopulated;
}

/**
* Clears the data from the graph.
*/
void NearestNeighbours::clear()
{
  m_graph.clear();
  m_detIDtoVertex.clear();
  m_isPopulated = false;
}

/**
* Builds the graph of the detectors and calculates nearest neighbours.
*/
void NearestNeighbours::build()
{
  clear();
  populate();
}

/**
* Returns a map of the DetectorID's to the nearest detectors and their
* distance from the detector specified in the argument.
* @param detID :: detector identifier for subject of query
* @return map of detID to distance
* @throw NotFoundError if detector ID is not recognised
*/
std::map<int64_t, double> NearestNeighbours::neighbours(const int detID) const
{  
  MapIV::const_iterator vertex = m_detIDtoVertex.find(detID);

  if ( vertex != m_detIDtoVertex.end() )
  {
    std::map<int64_t, double> result;
    std::pair<Graph::adjacency_iterator, Graph::adjacency_iterator> adjacent = boost::adjacent_vertices(vertex->second, m_graph);
    Graph::adjacency_iterator adjIt;  
    for ( adjIt = adjacent.first; adjIt != adjacent.second; adjIt++ )
    {
      Vertex nearest = (*adjIt);
      int64_t nrID = m_vertexID[nearest];
      std::pair<Graph::edge_descriptor, bool> nrEd = boost::edge(vertex->second, nearest, m_graph);
      double distance = m_edgeLength[nrEd.first];
      distance = sqrt(distance);
      result[nrID] = distance;
    }
    return result;
  }
  else
  {
    throw Mantid::Kernel::Exception::NotFoundError("NearestNeighbours: Detector with specified DetectorID not found in Instrument.", detID);
  }
}

/**
* Returns a map of the DetectorID's to the nearest detectors within a specified
* distance. Uses neighbours(detID) function. Will pass on up a NotFoundError exception
* if detID is invalid.
* @param detID :: detector identifier for subject of query
* @param radius :: cut-off distance for detector list to returns
* @return map of detID to distance
* @throw NotFoundError if detID is not recognised
*/
std::map<int64_t, double> NearestNeighbours::neighbours(const int detID, const double radius) const
{
  std::map<int64_t, double> nn = neighbours(detID);
  std::map<int64_t, double> result;
  std::map<int64_t, double>::iterator nnIt;
  for ( nnIt = nn.begin(); nnIt != nn.end(); ++nnIt )
  {
    if ( nnIt->second <= radius )
    {
      result[nnIt->first] = nnIt->second;
    }
  }
  return result;
}

/**
* Returns a map of the DetectorID's to the nearest detectors within a specified
* distance. Uses neighbours(detID) function. Will pass on up a NotFoundError exception
* if detID is invalid.
* @param component :: IComponent pointer to Detector object
* @param radius :: cut-off distance for detector list to returns
* @return map of Detector ID's to distance
* @throw NotFoundError if component is not recognised as a detector
*/
std::map<int64_t, double> NearestNeighbours::neighbours(const IComponent *component, const double radius) const
{
  const IDetector* detector = dynamic_cast<const Mantid::Geometry::IDetector*>(component);
  int detID = detector->getID();
  std::map<int64_t, double> nearest = neighbours(detID);
  if ( radius == 0.0 )
  {
    return nearest;
  }
  std::map<int64_t, double> result;
  std::map<int64_t, double>::iterator nnIt;
  for ( nnIt = nearest.begin(); nnIt != nearest.end(); ++nnIt )
  {
    if ( nnIt->second <= radius )
    {
      result[nnIt->first] = nnIt->second;
    }
  }
  return result;
}

/**
* Creates a graph object for the instrument, with the nodes being the detectors and edges linking the detectors to their
* eight (8) nearest neighbours. 
*/
void NearestNeighbours::populate()
{
  // Do not rebuild the graph if it has already been done.
  if ( isPopulated() )
  {
    return;
  }

  // ANN Variables
  ANNpointArray dataPoints;
  ANNkd_tree* annTree;
  // maps
  MapIV pointNoToVertex;
  
  std::map<int64_t, Mantid::Geometry::IDetector_sptr> detectors;
  m_instrument->getDetectors(detectors);
  std::map<int64_t, Mantid::Geometry::IDetector_sptr>::iterator detIt;

  size_t ndets = detectors.size(); // also number of points in array  
  int ndet = 0;
  for ( detIt = detectors.begin(); detIt != detectors.end(); detIt++ )
  {
    if ( detIt->second->isMonitor() ) { continue; }
    else { ndet++; }

    if ( m_scale == NULL && (ndets / ndet) == 1 )
    {
      // create scaling vector      
      boost::shared_ptr<Mantid::Geometry::Detector> det = boost::dynamic_pointer_cast<Mantid::Geometry::Detector>(detIt->second);
      BoundingBox bbox;
      det->getBoundingBox(bbox);
      double xmax = bbox.xMax();
      double ymax = bbox.yMax();
      double zmax = bbox.zMax();
      double xmin = bbox.xMin();
      double ymin = bbox.yMin();
      double zmin = bbox.zMin();
      m_scale = new Mantid::Geometry::V3D((xmax-xmin), (ymax-ymin), (zmax-zmin));
    }
  }

  dataPoints = annAllocPts(ndet, 3);
  int pointNo = 0;

  for ( detIt = detectors.begin(); detIt != detectors.end(); detIt++ )
  {
    Mantid::Geometry::IDetector_sptr detector = detIt->second;

    // We do not want to consider monitors
    if ( detector->isMonitor() ) { continue; }

    const int detID = detector->getID();
    Mantid::Geometry::V3D pos = detector->getPos() / *m_scale;
    dataPoints[pointNo][0] = pos.X();
    dataPoints[pointNo][1] = pos.Y();
    dataPoints[pointNo][2] = pos.Z();
    Vertex vertex = boost::add_vertex(detID, m_graph);
    pointNoToVertex[pointNo] = vertex;
    m_detIDtoVertex[detID] = vertex;
    pointNo++;
  }

  annTree = new ANNkd_tree(dataPoints, ndet, 3);
  pointNo = 0;
  for ( detIt = detectors.begin(); detIt != detectors.end(); detIt++ )
  {
    // we don't want to consider monitors
    if ( detIt->second->isMonitor() ) { continue; }

    // run the nearest neighbour search on each detector.
    ANNidxArray nnIndexList = new ANNidx[m_noNeighbours];
    ANNdistArray nnDistList = new ANNdist[m_noNeighbours];

    annTree->annkSearch(
      dataPoints[pointNo], // Point to search nearest neighbours of
      m_noNeighbours, // Number of neighbours to find (8)
      nnIndexList, // Index list of results
      nnDistList, // List of distances to each of these
      0.0 // Error bound (?) is this the radius to search in?
      );
    for ( int i = 0; i < m_noNeighbours; i++ )
    {
      boost::add_edge(
        m_detIDtoVertex[detIt->first], // from
        pointNoToVertex[nnIndexList[i]], // to
        nnDistList[i], // Distance Squared
        m_graph
        );
    }
    pointNo++;
    delete [] nnIndexList;
    delete [] nnDistList;
  }
  
  delete annTree;
  annDeallocPts(dataPoints);
  annClose();
  pointNoToVertex.clear();

  m_vertexID = get(boost::vertex_name, m_graph);
  m_edgeLength = get(boost::edge_name, m_graph);

  m_isPopulated = true;
}
