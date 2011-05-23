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
      m_scale()
    {
      const int noNeighbours = 8;
      this->build(instrument, spectraMap, noNeighbours);
    }
   
    /**
     * Returns a map of the spectrum numbers to the distances for the nearest neighbours.
     * @param component :: IComponent pointer to Detector object
     * @param radius :: cut-off distance for detector list to returns
     * @return map of Detector ID's to distance
     * @throw NotFoundError if component is not recognised as a detector
     */
    std::map<specid_t, double> NearestNeighbours::neighbours(const specid_t spectrum, const double radius) const
    {
      std::map<specid_t, double> nearest = defaultNeighbours(spectrum);
      if ( radius == 0.0 )
      {
	return nearest;
      }
      std::map<specid_t, double> result;
      std::map<specid_t, double>::const_iterator nnIt;
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
     * @param noNeighbours :: The number of nearest neighbours to use to build 
     * the graph 
     */
    void NearestNeighbours::build(IInstrument_const_sptr instrument, 
				  const ISpectraDetectorMap & spectraMap,
				  const int noNeighbours)
    {
      std::map<specid_t, IDetector_sptr> spectraDets =
	getSpectraDetectors(instrument, spectraMap);
      if( spectraDets.empty() )
      {
	throw std::runtime_error("NearestNeighbours::build - Cannot find any spectra");
      }

      const int nspectra = static_cast<int>(spectraDets.size()); //ANN only deals with integers
      BoundingBox bbox;
      // Base the scaling on the first detector, should be adequate but we can look at this
      IDetector_sptr firstDet = (*spectraDets.begin()).second;
      firstDet->getBoundingBox(bbox);
      m_scale.reset(new V3D(bbox.width()));
      ANNpointArray dataPoints = annAllocPts(nspectra, 3);
      MapIV pointNoToVertex;
      
      std::map<specid_t, IDetector_sptr>::const_iterator detIt;
      int pointNo = 0;
      for ( detIt = spectraDets.begin(); detIt != spectraDets.end(); ++detIt )
      {
      	IDetector_sptr detector = detIt->second;
      	const specid_t spectrum = detIt->first;
      	V3D pos = detector->getPos() / (*m_scale);
      	dataPoints[pointNo][0] = pos.X();
      	dataPoints[pointNo][1] = pos.Y();
      	dataPoints[pointNo][2] = pos.Z();
      	Vertex vertex = boost::add_vertex(spectrum, m_graph);
      	pointNoToVertex[pointNo] = vertex;
      	m_specToVertex[spectrum] = vertex;
	++pointNo;
      }

      ANNkd_tree *annTree = new ANNkd_tree(dataPoints, nspectra, 3);
      pointNo = 0;
      // Run the nearest neighbour search on each detector, reusing the arrays
      ANNidxArray nnIndexList = new ANNidx[noNeighbours];
      ANNdistArray nnDistList = new ANNdist[noNeighbours];
      for ( detIt = spectraDets.begin(); detIt != spectraDets.end(); ++detIt )
      {

      	annTree->annkSearch(
      	  dataPoints[pointNo], // Point to search nearest neighbours of
      	  noNeighbours, // Number of neighbours to find (8)
      	  nnIndexList, // Index list of results
      	  nnDistList, // List of distances to each of these
      	  0.0 // Error bound (?) is this the radius to search in?
      	  );
      	for ( int i = 0; i < noNeighbours; i++ )
      	{
      	  boost::add_edge(
      	    m_specToVertex[detIt->first], // from
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
     * Returns a map of the spectrum numbers to the nearest detectors and their
     * distance from the detector specified in the argument.
     * @param spectrum :: The spectrum number
     * @return map of detID to distance
     * @throw NotFoundError if detector ID is not recognised
     */
    std::map<specid_t, double> NearestNeighbours::defaultNeighbours(const specid_t spectrum) const
    {  
      MapIV::const_iterator vertex = m_specToVertex.find(spectrum);
      
      if ( vertex != m_specToVertex.end() )
      {
	std::map<specid_t, double> result;
	std::pair<Graph::adjacency_iterator, Graph::adjacency_iterator> adjacent = boost::adjacent_vertices(vertex->second, m_graph);
	Graph::adjacency_iterator adjIt;  
	for ( adjIt = adjacent.first; adjIt != adjacent.second; adjIt++ )
	{
	  Vertex nearest = (*adjIt);
	  specid_t nrSpec = specid_t(m_vertexID[nearest]);
	  std::pair<Graph::edge_descriptor, bool> nrEd = boost::edge(vertex->second, nearest, m_graph);
	  double distance = m_edgeLength[nrEd.first];
	  distance = sqrt(distance);
	  result[nrSpec] = distance;
	}
	return result;
      }
      else
      {
	throw Mantid::Kernel::Exception::NotFoundError("NearestNeighbours: Unable to find spectrum in vertex map", spectrum);
      }
    }

    /**
     * Get the list of detectors associated with a spectra
     * @param instrument :: A pointer to the instrument
     * @param spectraMap :: A reference to the spectra map
     * @returns A map of spectra number to detector pointer
     */
     std::map<specid_t, IDetector_sptr>
     NearestNeighbours::getSpectraDetectors(IInstrument_const_sptr instrument, 
					    const ISpectraDetectorMap & spectraMap)
     {
      std::map<specid_t, IDetector_sptr> spectra;
      spectraMap.moveIteratorToStart();
      // The map could return identical numbers as we go along, only consider the unique ones
      specid_t lastSpectrum(INT_MAX);
      while(spectraMap.hasNext())
      {
	specid_t spectrumNo = spectraMap.getCurrentSpectrum();
	if( spectrumNo != lastSpectrum )
	{
	  std::vector<int> detIDs = spectraMap.getDetectors(spectrumNo);
	  IDetector_sptr det = instrument->getDetector(detIDs);
	  if( !det->isMonitor() ) 
 	  {
	    spectra.insert(std::make_pair(spectrumNo, det));
	  }
	  lastSpectrum = spectrumNo;
	}
	spectraMap.advanceIterator();	
      }
      return spectra;
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
