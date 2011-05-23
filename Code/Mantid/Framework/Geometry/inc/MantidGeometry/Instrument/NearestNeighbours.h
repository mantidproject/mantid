#ifndef MANTID_GEOMETRY_INSTRUMENT_NEARESTNEIGHBOURS
#define MANTID_GEOMETRY_INSTRUMENT_NEARESTNEIGHBOURS

#include "MantidKernel/System.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/ISpectraDetectorMap.h"
// Boost graphing
#include <boost/graph/adjacency_list.hpp>
#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

namespace Mantid
{
  namespace Geometry
  {
    //------------------------------------------------------------------------------
    // Forward Declarations
    //------------------------------------------------------------------------------
    class IInstrument;
    class IComponent;
    class V3D;

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
     *  @author Martyn Gigg, Tessella plc
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
      /// Constructor with an instrument and a spectra map
      NearestNeighbours(boost::shared_ptr<const IInstrument> instrument,
			const ISpectraDetectorMap & spectraMap);
      /// Default (empty) destructor
      virtual ~NearestNeighbours() {};

      // Neighbouring spectra by radius
      std::map<specid_t, double> neighbours(const specid_t spectrum, const double radius=0.0) const;

    private:
      /// typedef for Graph object used to hold the calculated information
      typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS,
	boost::property<boost::vertex_name_t, int64_t>,
	boost::property<boost::edge_name_t, double>
	> Graph;
      /// Vertex descriptor object for Graph
      typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
      /// map object of int to Graph Vertex descriptor
      typedef boost::unordered_map<specid_t,Vertex> MapIV;

      /// Construct the graph based on the given number of neighbours and the 
      /// current instument and spectra-detector mapping
      void build(const int noNeighbours);
      /// Query the graph for the default number of nearest neighbours to specified detector
      std::map<specid_t, double> defaultNeighbours(const specid_t spectrum) const;

      /// Get the spectra associated with all in the instrument
      std::map<specid_t, IDetector_sptr> 
	getSpectraDetectors(boost::shared_ptr<const IInstrument> instrument, 
			    const ISpectraDetectorMap & spectraMap);

      /// A pointer the the instrument
      boost::shared_ptr<const IInstrument> m_instrument;
      /// A reference to the spectra map
      const ISpectraDetectorMap & m_spectraMap;
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
      boost::scoped_ptr<V3D> m_scale;
    };
    
    /// Typedef for shared pointer to the NearestNeighbours class
    typedef boost::shared_ptr<Mantid::Geometry::NearestNeighbours> NearestNeighbours_sptr;
    /// Typedef for constant shared pointer to the NearestNeighbours class
    typedef boost::shared_ptr<const Mantid::Geometry::NearestNeighbours> NearestNeighbours_const_sptr;
    
  } // namespace Geometry
} // namespace Mantid

#endif
