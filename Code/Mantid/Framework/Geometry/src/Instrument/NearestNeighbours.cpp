//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidGeometry/Instrument/NearestNeighbours.h"
#include "MantidGeometry/IInstrument.h"
#include "MantidGeometry/ISpectraDetectorMap.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
// Nearest neighbours library
#include "MantidKernel/ANN/ANN.h"

namespace Mantid
{
  namespace Geometry
  {
    using Mantid::detid_t;

    /**
     * Constructor
     * @param instrument :: A shared pointer to IInstrument object
     * @param spectraMap :: A reference to the spectra-detector mapping
     */
    NearestNeighbours::NearestNeighbours(IInstrument_const_sptr instrument,
					 const ISpectraDetectorMap & spectraMap) : 
      m_noNeighbours(8),  m_scale()
    {
      build(instrument, spectraMap);
    }

    /**
     * Returns a map of the DetectorID's to the nearest detectors and their
     * distance from the detector specified in the argument.
     * @param detID :: detector identifier for subject of query
     * @return map of detID to distance
     * @throw NotFoundError if detector ID is not recognised
     */
    std::map<detid_t, double> NearestNeighbours::neighbours(const detid_t detID) const
    {  
      MapIV::const_iterator vertex = m_detIDtoVertex.find(detID);
      
      if ( vertex != m_detIDtoVertex.end() )
      {
	std::map<detid_t, double> result;
	std::pair<Graph::adjacency_iterator, Graph::adjacency_iterator> adjacent = boost::adjacent_vertices(vertex->second, m_graph);
	Graph::adjacency_iterator adjIt;  
	for ( adjIt = adjacent.first; adjIt != adjacent.second; adjIt++ )
	{
	  Vertex nearest = (*adjIt);
	  detid_t nrID = detid_t(m_vertexID[nearest]);
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
    std::map<detid_t, double> NearestNeighbours::neighbours(const detid_t detID, const double radius) const
    {
      std::map<detid_t, double> nn = neighbours(detID);
      std::map<detid_t, double> result;
      std::map<detid_t, double>::iterator nnIt;
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
    std::map<detid_t, double> NearestNeighbours::neighbours(const IComponent *component, const double radius) const
    {
      const IDetector* detector = dynamic_cast<const Mantid::Geometry::IDetector*>(component);
      int detID = detector->getID();
      std::map<detid_t, double> nearest = neighbours(detID);
      if ( radius == 0.0 )
      {
	return nearest;
      }
      std::map<detid_t, double> result;
      std::map<detid_t, double>::iterator nnIt;
      for ( nnIt = nearest.begin(); nnIt != nearest.end(); ++nnIt )
      {
	if ( nnIt->second <= radius )
	{
	  result[nnIt->first] = nnIt->second;
	}
      }
      return result;
    }
    
    //--------------------------------------------------------------------------
    // Private member functions
    //--------------------------------------------------------------------------
    /**
     * Builds a map based on the given instrument and spectra map
     * @param instrument :: A pointer to the instrument
     * @param spectraMap :: A reference to the spectra-detector mapping
     */
    void NearestNeighbours::build(IInstrument_const_sptr instrument, 
				  const ISpectraDetectorMap & spectraMap)
    {
      std::vector<IDetector_sptr> spectraDets;
      spectraDets.reserve(spectraMap.nElements());
      getSpectraDetectors(spectraDets, instrument, spectraMap);
      if( spectraDets.empty() )
      {
	throw std::runtime_error("NearestNeighbours::build - Cannot find any spectra");
      }
      
      const int nspectra = static_cast<int>(spectraDets.size()); //ANN only deals with integers
      BoundingBox bbox;
      spectraDets[0]->getBoundingBox(bbox);
      m_scale.reset(new V3D(bbox.width()));
      ANNpointArray dataPoints = annAllocPts(nspectra, 3);
      MapIV pointNoToVertex;
      
      std::vector<IDetector_sptr>::const_iterator detIt;
      int pointNo = 0;
      for ( detIt = spectraDets.begin(); detIt != spectraDets.end(); detIt++ )
      {
      	IDetector_sptr detector = *detIt;
      	const detid_t detID = detector->getID();
      	V3D pos = detector->getPos() / (*m_scale);
      	dataPoints[pointNo][0] = pos.X();
      	dataPoints[pointNo][1] = pos.Y();
      	dataPoints[pointNo][2] = pos.Z();
      	Vertex vertex = boost::add_vertex(detID, m_graph);
      	pointNoToVertex[pointNo] = vertex;
      	m_detIDtoVertex[detID] = vertex;
	++pointNo;
      }

      ANNkd_tree *annTree = new ANNkd_tree(dataPoints, nspectra, 3);
      pointNo = 0;
      // Run the nearest neighbour search on each detector, reusing the arrays
      ANNidxArray nnIndexList = new ANNidx[m_noNeighbours];
      ANNdistArray nnDistList = new ANNdist[m_noNeighbours];
      for ( detIt = spectraDets.begin(); detIt != spectraDets.end(); detIt++ )
      {

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
      	    m_detIDtoVertex[(*detIt)->getID()], // from
      	    pointNoToVertex[nnIndexList[i]], // to
      	    nnDistList[i], // Distance Squared
      	    m_graph
      	    );
      	}
      	pointNo++;
      }
      delete [] nnIndexList;
      delete [] nnDistList;
      delete annTree;
      annDeallocPts(dataPoints);
      annClose();
      pointNoToVertex.clear();

      m_vertexID = get(boost::vertex_name, m_graph);
      m_edgeLength = get(boost::edge_name, m_graph);
    }

    /**
     * Get the list of detectors associated with a spectra
     * @param[output] spectra :: A vector of IDetector pointers filled with the results of the search
     * @param instrument :: A pointer to the instrument
     * @param spectraMap :: A reference to the spectra map
     */
    void NearestNeighbours::getSpectraDetectors(std::vector<IDetector_sptr>& spectra, 
						IInstrument_const_sptr instrument, 
						const ISpectraDetectorMap & spectraMap)
    {
      spectraMap.moveIteratorToStart();
      // The map could return identical numbers as we go along, only consider the unique ones
      specid_t lastSpectrum(INT_MAX);
      while(spectraMap.hasNext())
      {
	specid_t spectrumNo = spectraMap.getCurrentSpectrum();
	if( spectrumNo != lastSpectrum )
	{
	  std::vector<int> detIDs = spectraMap.getDetectors(spectrumNo);
	  std::vector<IDetector_sptr> dets = instrument->getDetectors(detIDs);
	  //Assume that if the first is a monitor, then we want to ignore it
	  if( !dets[0]->isMonitor() ) 
	  {
	    if( dets.size() == 1 )
	    {
	      spectra.insert(spectra.end(), dets[0]);
	    }
	    else
	    {
	      spectra.insert(spectra.end(), IDetector_sptr(new DetectorGroup(dets, false)));
	    }
	  }
	  lastSpectrum = spectrumNo;
	}
	spectraMap.advanceIterator();	
      }
    }

    /**
     * Creates a graph object for the instrument, with the nodes being the detectors and 
     * edges linking the detectors to their  nearest neighbours. 
     */
    void NearestNeighbours::populate()
    {
      throw Kernel::Exception::NotImplementedError("NearestNeighbours::populate()");
    }
  }
}
