#ifndef H_MD_WORKSPACE
#define H_MD_WORKSPACE
#include "MDDataObjects/MDDataPoints.h"
#include "MDDataObjects/MDImage.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MDDataObjects/IMD_FileFormat.h"
#include "MantidGeometry/MDGeometry/MDGeometry.h"


/** Class is empty to provide calling name for all MD-workspace components and provide interfaces to all workspace  public set functions 
*   and to the one not included in separate components (like get_detectors)    (in a future) 


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

    class DLLExport MDWorkspace :  public IMDWorkspace
    {
    public:

      virtual int getNPoints() const;

      virtual long getMemorySize(void)const;
      MDWorkspace(unsigned int nDimensions=4,unsigned int nRecDims=3)
      {};
      //  IMD workspace interface functions
      /// return ID specifying the workspace kind
      virtual const std::string id() const { return "MD-Workspace"; }
      ///
      virtual unsigned int getNumDims(void) const{return m_spImageData->getGeometry()->getNumDims();}

      void init(boost::shared_ptr<Mantid::MDDataObjects::IMD_FileFormat> spFile, Mantid::Geometry::MDGeometry* geometry);

      virtual ~MDWorkspace(void){};

      /// read the the pixels corresponding to cells in the vector cell_num
      size_t read_pix_selection(const std::vector<size_t> &cells_nums,size_t &start_cell,std::vector<char> &pix_buf,size_t &n_pix_in_buffer);

      Mantid::Geometry::MDGeometry const * const getGeometry() const;

      void alloc_mdd_arrays(const MDGeometryDescription &transf){m_spImageData->alloc_mdd_arrays(transf);}
      /// identify proper file reader which corresponds to the file name and read memory resident part of the workspace into memory
      void read_mdd();
      /// read the whole pixels dataset in the memory
      void read_pix(void);
      /// function writes the MDD data using current file reader; if the file is not opened, a default file reader is used. 
      void write_mdd();
      /// function writes back MDD data to the existing dataset attached to the class;  Should throw if the size of the data changed (and this should not happen)
      //    bool write_mdd(void);
      Mantid::Geometry::MDGeometry const * const       get_const_spMDGeometry() const {return m_spImageData->getGeometry();}
      boost::shared_ptr<Mantid::MDDataObjects::MDImage const>  get_const_spMDImage()  const {return m_spImageData;}
      boost::shared_ptr<Mantid::MDDataObjects::MDDataPoints const> get_const_spMDDPoints() const {return m_spDataPoints;}

      //Mantid::Geometry::MDGeometry const * const       get_spMDGeometry(){return m_spImageData->getGeometry();}
      boost::shared_ptr<Mantid::MDDataObjects::MDImage> get_spMDImage()  {return m_spImageData;}
      boost::shared_ptr<Mantid::MDDataObjects::MDDataPoints>get_spMDDPoints(){return m_spDataPoints;}

      virtual boost::shared_ptr<const Mantid::Geometry::IMDDimension>  getXDimension() const;

      virtual boost::shared_ptr< const Mantid::Geometry::IMDDimension>  getYDimension() const;

      virtual boost::shared_ptr<const Mantid::Geometry::IMDDimension>  getZDimension() const;

      virtual boost::shared_ptr<const Mantid::Geometry::IMDDimension>  gettDimension() const;

      virtual boost::shared_ptr<const Mantid::Geometry::IMDDimension>  getDimension(std::string id) const;

      virtual boost::shared_ptr<const Mantid::Geometry::MDPoint>  getPoint(int index) const;

      virtual  boost::shared_ptr<const Mantid::Geometry::MDCell>  getCell(int dim1Increment) const;

      virtual boost::shared_ptr<const Mantid::Geometry::MDCell>  getCell(int dim1Increment, int dim2Increment) const;

      virtual boost::shared_ptr<const Mantid::Geometry::MDCell>  getCell(int dim1Increment, int dim2Increment, int dim3Increment) const;

      virtual  boost::shared_ptr<const Mantid::Geometry::MDCell>  getCell(int dim1Increment, int dim2Increment, int dim3Increment, int dim4Increment) const;

      virtual boost::shared_ptr<const Mantid::Geometry::MDCell>  getCell(...) const;

    private:
      static Kernel::Logger& g_log;

     
      boost::shared_ptr<Mantid::MDDataObjects::MDImage> m_spImageData;
      boost::shared_ptr<Mantid::MDDataObjects::MDDataPoints> m_spDataPoints;
      boost::shared_ptr<Mantid::MDDataObjects::IMD_FileFormat> m_spFile;
    };


    ///shared pointer to the MD workspace base class
    typedef boost::shared_ptr<MDWorkspace> MDWorkspace_sptr;
    ///shared pointer to the MD workspace base class (const version)
    typedef boost::shared_ptr<const MDWorkspace> MDWorkspace_const_sptr;



  }}



#endif