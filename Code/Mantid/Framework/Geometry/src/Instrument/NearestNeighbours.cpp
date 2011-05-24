//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidGeometry/Instrument/NearestNeighbours.h"
#include "MantidGeometry/IInstrument.h"
#include "MantidGeometry/ISpectraDetectorMap.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
// Nearest neighbours library
#include "MantidKernel/ANN/ANN.h"
#include "MantidKernel/Timer.h"

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
      m_instrument(instrument), m_spectraMap(spectraMap), m_noNeighbours(8), m_cutoff(-DBL_MAX), m_scale()
    {
      this->build(m_noNeighbours);
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
      // If the radius is stupid then don't let it continue as well be stuck forever
      if( radius < 0.0 || radius > 10.0 )
      {
	throw std::invalid_argument("NearestNeighbours::neighbours - Invalid radius parameter.");
      }

      std::map<specid_t, double> result;
      if( radius == 0.0 ) 
      {
	// Note: Should be able to do this better but time constraints for the moment mean that
	// it is necessary. 
	// Cast is necessary as the user should see this as a const member
	const_cast<NearestNeighbours*>(this)->build(8);
	result = defaultNeighbours(spectrum);	  
      }
      else if( radius > m_cutoff )
      {
	// We might have to see how efficient this ends up being.
	int neighbours = m_noNeighbours + 1;
	while( true )
	{
	  try
	  {
	    const_cast<NearestNeighbours*>(this)->build(neighbours);
	  }
	  catch(std::invalid_argument&)
	  {
	    break;
	  }
	  if( radius < m_cutoff ) break;
	  else neighbours += 1;
	}
      }
    
      std::map<detid_t, double> nearest = defaultNeighbours(spectrum);
      std::map<specid_t, double>::const_iterator cend;
      for(std::map<specid_t, double>::const_iterator cit = nearest.begin();
	  cit != nearest.end(); ++cit )
      {
	if ( cit->second <= radius )
	{
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
    void NearestNeighbours::build(const int noNeighbours)
    {
      std::map<specid_t, IDetector_sptr> spectraDets 
	= getSpectraDetectors(m_instrument, m_spectraMap);
      if( spectraDets.empty() )
      {
	throw std::runtime_error("NearestNeighbours::build - Cannot find any spectra");
      }
      const int nspectra = static_cast<int>(spectraDets.size()); //ANN only deals with integers
      if( noNeighbours >= nspectra )
      {
	throw std::invalid_argument("NearestNeighbours::build - Invalid number of neighbours");
      }

      // Clear current
      m_graph.clear(); 
      m_specToVertex.clear();
      m_noNeighbours = noNeighbours;

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
      	V3D pos = detector->getPos()/(*m_scale);
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
      ANNidxArray nnIndexList = new ANNidx[m_noNeighbours];
      ANNdistArray nnDistList = new ANNdist[m_noNeighbours];

      for ( detIt = spectraDets.begin(); detIt != spectraDets.end(); ++detIt )
      {
	ANNpoint scaledPos = dataPoints[pointNo]; 
      	annTree->annkSearch(
      	  scaledPos, // Point to search nearest neighbours of
      	  m_noNeighbours, // Number of neighbours to find (8)
      	  nnIndexList, // Index list of results
      	  nnDistList, // List of distances to each of these
      	  0.0 // Error bound (?) is this the radius to search in?
      	  );
	// The distances that are returned are in our scaled coordinate
	// system. We store the real space ones.
	V3D realPos = V3D(scaledPos[0], scaledPos[1], scaledPos[2])*(*m_scale);
      	for ( int i = 0; i < m_noNeighbours; i++ )
      	{
	  ANNidx index = nnIndexList[i];
	  V3D neighbour = V3D(dataPoints[index][0], dataPoints[index][1], dataPoints[index][2])*(*m_scale);
	  double separation = (neighbour-realPos).norm();
      	  boost::add_edge(
      	    m_specToVertex[detIt->first], // from
      	    pointNoToVertex[index], // to
	    separation, 
      	    m_graph
      	    );
	  if( separation > m_cutoff )
	  {
	    m_cutoff = separation;
	  }
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
	  result[nrSpec] = m_edgeLength[nrEd.first];
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
      if( spectraMap.nElements() == 0 ) return spectra;
      spectraMap.moveIteratorToStart();
      // The map could return identical numbers as we go along, only consider the unique ones
      specid_t lastSpectrum = spectraMap.getCurrentSpectrum();
      std::vector<int> detIDs = spectraMap.getDetectors(lastSpectrum);
      IDetector_sptr det = instrument->getDetector(detIDs);
      if( !det->isMonitor() ) 
      {
	spectra.insert(std::make_pair(lastSpectrum, det));
      }
      while(spectraMap.hasNext())
      {
	spectraMap.advanceIterator();
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
      }
      return spectra;
    }

  }
}
