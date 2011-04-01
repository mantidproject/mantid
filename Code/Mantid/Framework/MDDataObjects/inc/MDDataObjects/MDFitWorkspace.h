#ifndef H_MD_FITWORKSPACE
#define H_MD_FITWORKSPACE
#include "MDDataObjects/DllExport.h"
#include "MDDataObjects/MDDataPoints.h"
#include "MDDataObjects/MDIndexCalculator.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidGeometry/MDGeometry/MDCell.h"
#include "MantidGeometry/MDGeometry/MDGeometry.h"
#include "MantidGeometry/Instrument/Instrument.h"

#include <boost/scoped_ptr.hpp>
#include <map>

/** 

MDFitWorkspace is a simpler version of the MDWorkspace. It is intended to be used 
for testing MD fitting.


@author Roman Tolchenov, Tessella plc
@date 29/03/2011

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
  namespace API
  {
    class IMDIterator;
  }

  namespace MDDataObjects
  {

   class MDWorkspaceIndexCalculator;

    class EXPORT_OPT_MANTID_MDDATAOBJECTS MDFitWorkspace :  public API::IMDWorkspace
    {
      typedef std::vector<boost::shared_ptr<Mantid::Geometry::MDPoint> > MDPointMap;
      typedef std::vector<Mantid::Geometry::MDCell> MDCellMap;
    public:

      MDFitWorkspace(unsigned int nDimensions=4,unsigned int nRecDims=3);

      /*---            Overidden/Implemented methods           ---*/

      virtual size_t getMemorySize(void) const;

      //  IMD workspace interface functions
      /// return ID specifying the workspace kind
      virtual const std::string id() const { return "MDFitWorkspace"; }

      virtual ~MDFitWorkspace(void)
      {
      };

      /// Gets the number of points(MDDataPoints, events) contributed to the workspace.
      ///TODO: resolve -- do we need number of points contributed to workspace or availible in it -- > different things if we 
      /// assume existence of DND objects
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
      virtual const Mantid::Geometry::SignalAggregate& getPoint(unsigned int index) const;

      /// Get the cell at the specified index/increment.
      virtual const Mantid::Geometry::SignalAggregate& getCell(unsigned int dim1Increment) const;

      /// Get the cell at the specified index/increment.
      virtual const Mantid::Geometry::SignalAggregate& getCell(unsigned int dim1Increment, unsigned int dim2Increment) const;

      /// Get the cell at the specified index/increment.
      virtual const Mantid::Geometry::SignalAggregate& getCell(unsigned int dim1Increment, unsigned int dim2Increment, unsigned int dim3Increment) const;

      /// Get the cell at the specified index/increment.
      virtual const Mantid::Geometry::SignalAggregate& getCell(unsigned int dim1Increment, unsigned int dim2Increment, unsigned int dim3Increment, unsigned int dim4Increment) const;

      /// Get the cell at the specified index/increment.
      virtual const Mantid::Geometry::SignalAggregate& getCell(...) const;

      /// Get the location of the workspace on disk, supports Horace implementation of rebinning.
      virtual std::string getWSLocation() const{return "";}

      /// Get the geometry xml.
      virtual std::string getGeometryXML() const{return "";}

      void setInstrument(const Mantid::Geometry::IInstrument_sptr& instr);

      /// Creates a new iterator pointing to the first cell in the workspace
      virtual API::IMDIterator* createIterator() const;

      /*---            New methods           ---*/

      void setDimension(int idim,const std::string& paramString);
      void setCell(int index,const std::vector<boost::shared_ptr<Mantid::Geometry::MDPoint> >& points);

    private:
      /// Cells of this workspace
      mutable MDCellMap m_cells;

      /// MD points of this workspace
      mutable MDPointMap m_points;

      // Shared pointer to a base instrument.
      mutable boost::shared_ptr<Mantid::Geometry::Instrument> m_instrument;

      /// The instrument parameter map.
      mutable boost::shared_ptr<Geometry::ParameterMap> m_parmap;

      static Kernel::Logger& g_log;
      /// pointer to the geometry basis e.g. the unit cell of the bravis latice of the crystal and additional dimensions
      std::vector<boost::shared_ptr<Mantid::Geometry::IMDDimension> > m_dimensions;

      /// Calculations between the MD indeces and the single flat index
      boost::scoped_ptr<MDWorkspaceIndexCalculator> m_indexCalculator;

    };

    ///shared pointer to the MD workspace base class
    typedef boost::shared_ptr<MDFitWorkspace> MDFitWorkspace_sptr;
    ///shared pointer to the MD workspace base class (const version)
    typedef boost::shared_ptr<const MDFitWorkspace> MDFitWorkspace_const_sptr;


  } // MDDataObjects
}   // Mantid

#endif // H_MD_FITWORKSPACE
