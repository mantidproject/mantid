#ifndef H_MD_WORKSPACE
#define H_MD_WORKSPACE
#include <map>
#include "MDDataObjects/MDDataPoints.h"
#include "MDDataObjects/MDImage.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MDDataObjects/IMD_FileFormat.h"
#include "MantidGeometry/MDGeometry/MDGeometry.h"
#include "MantidGeometry/MDGeometry/MDDimension.h"
#include "MantidGeometry/Instrument/Instrument.h"


/** MD-workspace -> main class which keeps all data objects necessary for work with 
*   MD data used in visualisation and analysis
*
*  MDWorkspace: a workspace containing multidimensional scattering data.
*  For instance, this may represent the data from a single run, transformed 
*  to reciprocal space = 3 dimensions.
*  Add an energy loss for inelastic instruments = 4 dimensions.
*  Add another dimension, for example, Temperature = 5 dimensions.
*
*  A single MD workspace may combine the data from several runs.
*
*   There are 5 objects (components) intended to be here.
*  <li> 1)  MD_basis describing a reciprocal lattice of a crystal + additional dimensions (e.g. energy exchange or temperature)</li>
*  <li> 2)  MDImage composed of MD geometry and MD Image data itself </li>
*  <li> 3)  MDDataPoints -- class responsible for providing all data obtained from experiments transformed to reciprocal space </li>
*  <li> 4)  fileFormat   -- class providing reading/writing operations for all other classes -> supports different file formats </li>
*  <li> 5)  InstrumentDescription -- neded for simulation and analysis; not implemented at the moment 
TODO: implement  </li>
*
* The components provide interfaces to all workspace  public set functions
* and to the one not included in separate components (like get_detectors)    (in a future)
*


@author Alex Buts, RAL ISIS
@date 08/10/2010

Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

namespace Mantid
{
  namespace MDDataObjects
  {

    //Seam method
    boost::shared_ptr<Mantid::MDDataObjects::MDDataPoints> getDataPoints(boost::shared_ptr<MDImage> imageData);

    //Seam method.
    boost::shared_ptr<Mantid::MDDataObjects::MDImage> getImageData(const Mantid::Geometry::MDGeometry* geometry);

    typedef std::map<size_t, Mantid::Geometry::MDPoint> MDPointMap;
    typedef std::map<size_t, Mantid::Geometry::MDCell> MDCellMap;
    class DLLExport MDWorkspace :  public API::IMDWorkspace
    {
    public:

      MDWorkspace(unsigned int nDimensions=4,unsigned int nRecDims=3);

      virtual size_t getMemorySize(void) const;

      //  IMD workspace interface functions
     /// return ID specifying the workspace kind
      virtual const std::string id() const { return "MD-Workspace"; }

	  ///OBSOLETE? or should be modified as does not work properly at the moment  Total share pointers mess;
      void init(boost::shared_ptr<IMD_FileFormat> spFile, Mantid::Geometry::MDGeometry* geometry);
	  /** initialize from another workspace but with different MD image and (sub) set of data points; 
	   the basis and the instrument description(s) are the same and copied from the source

	   TODO: All set methods have to be ready to make copy-construction of the corresponding object and analyse reference counter if 
	   shared pointers to be implemented. 

	   file manager will be different so should come from somewhere. Probably from factory when requested
	   save algorithm will have this file property and should create proper file manager.
	   the workspace itself will allocate temporary file for its internal usage -- this file will be basis for final workspace file when saved;
	   */
	  void init(boost::shared_ptr<const MDWorkspace> SourceWorkspace,const Geometry::MDGeometryDescription *const transf=NULL);

      /** Initialize on the basis of separate components */
	  void init(std::auto_ptr<IMD_FileFormat> pFile,
                std::auto_ptr<Geometry::MDGeometryBasis> pBasis,
                const Geometry::MDGeometryDescription &geomDescr,
                const MDPointDescription &pd);

      virtual ~MDWorkspace(void)
      {
      };

    
	  /// get variois components of the workspace
	  Mantid::Geometry::MDGeometryBasis &   get_const_MDBaisis()  const{return *(m_spMDBasis);}
      Mantid::Geometry::MDGeometry const&   get_const_MDGeometry()const{return (m_spMDImage->get_const_MDGeometry());}
      Mantid::MDDataObjects::MDImage    &   get_const_MDImage()   const{return *(m_spMDImage);}
      Mantid::MDDataObjects::MDDataPoints & get_const_MDDPoints() const{return *(m_spDataPoints);}
      IMD_FileFormat                      & get_const_FileReader()const{return *(m_spFile);}      


	  // methods which provide access to mutable parts and allow to modify the data 
      boost::shared_ptr<Mantid::MDDataObjects::MDImage> get_spMDImage()      {return m_spMDImage;}
      boost::shared_ptr<Mantid::MDDataObjects::MDDataPoints>get_spMDDPoints(){return m_spDataPoints;}


	
      
        /// Gets the number of points(MDDataPoints, events) contributed to the workspace.
      ///TODO: resolve -- do we need number of points contributed to workspace or availible in it -- > different things if we 
      /// assume existence of DND objects; currently returns number of points in a sqw object e.g. number of MDDataPoints and throws if MDDPoints are undefined;
      virtual uint64_t getNPoints() const;

      /// Get the number of dimensions
      virtual size_t getNumDims() const;

      /// Get the x-dimension mapping.
      virtual boost::shared_ptr<const Mantid::Geometry::IMDDimension> getXDimension() const;

      /// Get the y-dimension mapping.
      virtual boost::shared_ptr<const Mantid::Geometry::IMDDimension> getYDimension() const;

      /// Get the z-dimension mapping.
      virtual boost::shared_ptr<const Mantid::Geometry::IMDDimension> getZDimension() const;

      /// Get the t-dimension mapping.
      virtual boost::shared_ptr<const Mantid::Geometry::IMDDimension> getTDimension() const;

      /// Get the dimension with the specified id.
      virtual boost::shared_ptr<const Mantid::Geometry::IMDDimension> getDimension(std::string id) const;

      /// Get the dimension ids in their order
      virtual const std::vector<std::string> getDimensionIDs() const;

      /// Get the point at the specified index.
      virtual const Mantid::Geometry::SignalAggregate& getPoint(size_t index) const;

      /// Get the cell at the specified index/increment.
      virtual const Mantid::Geometry::SignalAggregate& getCell(size_t dim1Increment) const;

      /// Get the cell at the specified index/increment.
      virtual const Mantid::Geometry::SignalAggregate& getCell(size_t dim1Increment, size_t dim2Increment) const;

      /// Get the cell at the specified index/increment.
      virtual const Mantid::Geometry::SignalAggregate& getCell(size_t dim1Increment, size_t dim2Increment, size_t dim3Increment) const;

      /// Get the cell at the specified index/increment.
      virtual const Mantid::Geometry::SignalAggregate& getCell(size_t dim1Increment, size_t dim2Increment, size_t dim3Increment, size_t dim4Increment) const;

      /// Get the cell at the specified index/increment.
      virtual const Mantid::Geometry::SignalAggregate& getCell(...) const;

      /// IMDWorkspace getNormalized signal.
      virtual double getSignalNormalizedAt(size_t index1, size_t index2, size_t index3, size_t index4) const;

      /// Get the location of the workspace on disk, supports Horace implementation of rebinning.
      virtual std::string getWSLocation() const;

      /// Get the geometry xml.
      virtual std::string getGeometryXML() const;

      void setInstrument(const Mantid::Geometry::IInstrument_sptr& instr);

      /// Creates a new iterator pointing to the first cell in the workspace
      Mantid::API::IMDIterator* createIterator() const;

      /// Getter for those dimensions that are not collapsed.
      Mantid::Geometry::VecIMDDimension_const_sptr getNonIntegratedDimensions() const;

    protected:

      /// Getter via geometry for dimensions. Not part of public API.
      virtual Mantid::Geometry::VecIMDDimension_sptr getDimensions() const;

    private:
      /// Cache. Gives MDWorkspace ownership of MDPoints (as an IMDWorkspace), while allowing polymorphic behaviour.
      /// Note this is NOT an optimized implementation yet, but it does support lazy instantiation.
      mutable MDPointMap m_mdPointMap;
      /// Cache. Gives MDWorkspace ownership of MDCells (as an IMDWorkspace), while allowing polymorphic behaviour.
      /// Note this is NOT an optimized implementation yet, but it does support lazy instantiation.
      mutable MDCellMap m_mdCellMap;

      // Shared pointer to a base instrument.
      mutable boost::shared_ptr<Mantid::Geometry::Instrument> sptr_instrument;

      /// The instrument parameter map.
      mutable boost::shared_ptr<Geometry::ParameterMap> m_parmap;

      static Kernel::Logger& g_log;
	  /// pointer to the geometry basis e.g. the unit cell of the bravis latice of the crystal and additional dimensions
	  boost::shared_ptr<Mantid::Geometry::MDGeometryBasis> m_spMDBasis;
      /// Pointer to MD Image of cells, which aslo provides the data 
      boost::shared_ptr<Mantid::MDDataObjects::MDImage> m_spMDImage;

      /// Internal data storage, class provides placement and IO (pack/unpack) operations for sparce array of data points.
      boost::shared_ptr<Mantid::MDDataObjects::MDDataPoints> m_spDataPoints;

      /// Pointer to a file reader/writer
      boost::shared_ptr<Mantid::MDDataObjects::IMD_FileFormat> m_spFile;
      /// no copy constructor -- its meaning is unclear; 
      MDWorkspace(const MDWorkspace &){};

      /// Determine if a new IMDWorkspace MDCell is required.
      inline bool newCellRequired(const size_t& singleDimensionIndex, const MD_image_point& mdImagePoint) const;

     };

    ///shared pointer to the MD workspace base class
    typedef boost::shared_ptr<MDWorkspace> MDWorkspace_sptr;
    ///shared pointer to the MD workspace base class (const version)
    typedef boost::shared_ptr<const MDWorkspace> MDWorkspace_const_sptr;


/// Non-member helper. Does not need access to private fields.
/// Creates coordinates to represent cell in 4D given a set of dimensions
Mantid::Geometry::VecCoordinate create4DPolyhedron(
    unsigned int dim1Increment,
    unsigned int dim2Increment,
    unsigned int dim3Increment,
    unsigned int dim4Increment,
    Mantid::Geometry::IMDDimension_sptr xDimension,
    Mantid::Geometry::IMDDimension_sptr yDimension,
    Mantid::Geometry::IMDDimension_sptr zDimension,
    Mantid::Geometry::IMDDimension_sptr tDimension);

/// Non-member helper. Does not need access to private fields.
/// Creates coordinates to represent cell in 3D given a set of dimensions
Mantid::Geometry::VecCoordinate createPolyhedron(
    unsigned int dim1Increment,
    unsigned int dim2Increment,
    unsigned int dim3Increment,
    Mantid::Geometry::IMDDimension_sptr xDimension,
    Mantid::Geometry::IMDDimension_sptr yDimension,
    Mantid::Geometry::IMDDimension_sptr zDimension);

/// Non-member helper. Does not need access to private fields.
/// Creates coordinates to represent cell in 2D given a set of dimensions
Mantid::Geometry::VecCoordinate createPolygon(
    unsigned int dim1Increment,
    unsigned int dim2Increment,
    Mantid::Geometry::IMDDimension_sptr xDimension,
    Mantid::Geometry::IMDDimension_sptr yDimension);

/// Non-member helper. Does not need access to private fields.
/// Creates coordinates to represent cell in 1D given a set of dimensions
Mantid::Geometry::VecCoordinate createLine(
    unsigned int dim1Increment,
    Mantid::Geometry::IMDDimension_sptr xDimension);


}
}

#endif
