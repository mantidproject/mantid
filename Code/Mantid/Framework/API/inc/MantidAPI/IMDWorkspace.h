#ifndef MANTID_API_IMDWORKSPACE_H_
#define MANTID_API_IMDWORKSPACE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <stdint.h>
#include "MantidAPI/Workspace.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDCell.h"
#include "MantidGeometry/MDGeometry/MDPoint.h"
#include <boost/shared_ptr.hpp>
#include <vector>
#include <stdarg.h>

namespace Mantid
{


  namespace API
  {

    class IMDIterator;
    
    /** Base MD Workspace Abstract Class.
     *  
     *   It defines common interface to Matrix Workspace and MD workspace. It is expected that all algorithms, which are applicable 
     *   to both 2D matrix workspace and MD workspace will use methods, with interfaces, defined here. 

     @author Alex Buts, ISIS, RAL
     @date 04/10/2010

     Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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


    class DLLExport IMDWorkspace : public Workspace
    {
    public:
      
      /// Get the number of points associated with the workspace; For MD workspace it is number of points contributing into the workspace
        // TODO: -- what is its meaning for the matrix workspace, may be different or the same? different logic of the operations
      virtual uint64_t getNPoints() const = 0;

      /// Get the number of dimensions
      virtual size_t getNumDims() const = 0;

      /// Get the x-dimension mapping.
      virtual Mantid::Geometry::IMDDimension_const_sptr getXDimension() const = 0;

      /// Get the y-dimension mapping.
      virtual Mantid::Geometry::IMDDimension_const_sptr getYDimension() const = 0;

      /// Get the z-dimension mapping.
      virtual Mantid::Geometry::IMDDimension_const_sptr getZDimension() const = 0;

      /// Get the t-dimension mapping.
      virtual Mantid::Geometry::IMDDimension_const_sptr getTDimension() const = 0;

      /// Get the dimension with the specified id.
      virtual Mantid::Geometry::IMDDimension_const_sptr getDimension(std::string id) const = 0;

      /// Get those dimensions which have not been collapsed.
      virtual Mantid::Geometry::VecIMDDimension_const_sptr getNonIntegratedDimensions() const = 0;

      /// Get the dimension
      virtual Mantid::Geometry::IMDDimension_sptr getDimensionNum(size_t index)
      { (void) index;
      throw std::runtime_error("Not implemented yet.");
      }

      /// Get the dimension ids in their order
      virtual const std::vector<std::string> getDimensionIDs() const = 0;





      /// Get the signal at the specified index.
      virtual double getSignalAt(size_t index1) const
      {
        return getPoint(index1).getSignal();
      }

      /// Get the signal at the specified index given in 4 dimensions (typically X,Y,Z,t)
      virtual double getSignalAt(size_t index1, size_t index2) const
      {
        return getCell(index1,index2).getSignal();
      }

      /// Get the signal at the specified index given in 4 dimensions (typically X,Y,Z,t)
      virtual double getSignalAt(size_t index1, size_t index2, size_t index3) const
      {
        return getCell(index1,index2,index3).getSignal();
      }

      /// Get the signal at the specified index given in 4 dimensions (typically X,Y,Z,t)
      virtual double getSignalAt(size_t index1, size_t index2, size_t index3, size_t index4) const
      {
        return getCell(index1,index2,index3,index4).getSignal();
      }


      /// Get the error of the signal at the specified index.
      virtual double getErrorAt(size_t index) const
      {
        return getPoint(index).getError();
      }

      /// Get the signal at the specified index given in 4 dimensions (typically X,Y,Z,t)
      virtual double getErrorAt(size_t index1, size_t index2) const
      {
        return getCell(index1,index2).getError();
      }

      /// Get the signal at the specified index given in 4 dimensions (typically X,Y,Z,t)
      virtual double getErrorAt(size_t index1, size_t index2, size_t index3) const
      {
        return getCell(index1,index2,index3).getError();
      }

      /// Get the signal at the specified index given in 4 dimensions (typically X,Y,Z,t)
      virtual double getErrorAt(size_t index1, size_t index2, size_t index3, size_t index4) const
      {
        return getCell(index1,index2,index3,index4).getError();
      }


      /// Get the signal at the specified index, normalized by cell volume
      virtual double getSignalNormalizedAt(size_t index) const
      { UNUSED_ARG(index);
        throw std::runtime_error("Not implemented yet.");
      }

      /// Get the signal at the specified index given in 4 dimensions (typically X,Y,Z,t), normalized by cell volume
      virtual double getSignalNormalizedAt(size_t index1, size_t index2) const
      { UNUSED_ARG(index1); UNUSED_ARG(index2);
        throw std::runtime_error("Not implemented yet.");
      }

      /// Get the signal at the specified index given in 4 dimensions (typically X,Y,Z,t), normalized by cell volume
      virtual double getSignalNormalizedAt(size_t index1, size_t index2, size_t index3) const
      { UNUSED_ARG(index1); UNUSED_ARG(index2); UNUSED_ARG(index3);
        throw std::runtime_error("Not implemented yet.");
      }

      /// Get the signal at the specified index given in 4 dimensions (typically X,Y,Z,t), normalized by cell volume
      virtual double getSignalNormalizedAt(size_t index1, size_t index2, size_t index3, size_t index4) const
      { UNUSED_ARG(index1); UNUSED_ARG(index2); UNUSED_ARG(index3); UNUSED_ARG(index4);
        throw std::runtime_error("Not implemented yet.");
      }


      /// Get the error of the signal at the specified index, normalized by cell volume
      virtual double getErrorNormalizedAt(size_t index) const
      { UNUSED_ARG(index);
        throw std::runtime_error("Not implemented yet.");
      }

      /// Get the signal at the specified index given in 4 dimensions (typically X,Y,Z,t), normalized by cell volume
      virtual double getErrorNormalizedAt(size_t index1, size_t index2) const
      { UNUSED_ARG(index1); UNUSED_ARG(index2);
        throw std::runtime_error("Not implemented yet.");
      }

      /// Get the signal at the specified index given in 4 dimensions (typically X,Y,Z,t), normalized by cell volume
      virtual double getErrorNormalizedAt(size_t index1, size_t index2, size_t index3) const
      { UNUSED_ARG(index1); UNUSED_ARG(index2); UNUSED_ARG(index3);
        throw std::runtime_error("Not implemented yet.");
      }

      /// Get the signal at the specified index given in 4 dimensions (typically X,Y,Z,t), normalized by cell volume
      virtual double getErrorNormalizedAt(size_t index1, size_t index2, size_t index3, size_t index4) const
      { UNUSED_ARG(index1); UNUSED_ARG(index2); UNUSED_ARG(index3); UNUSED_ARG(index4);
        throw std::runtime_error("Not implemented yet.");
      }




      /// Return a vector containing a copy of the signal data in the workspace. TODO: Make this more efficient if needed.
      virtual std::vector<double> getSignalDataVector() const
      {
        throw std::runtime_error("Not implemented yet.");
      }

      /// Return a vector containing a copy of the signal data in the workspace. TODO: Make this more efficient if needed.
      virtual std::vector<double> getErrorDataVector() const
      {
        throw std::runtime_error("Not implemented yet.");
      }




      /// Get the point at the specified index.
      virtual const Mantid::Geometry::SignalAggregate& getPoint(size_t index) const = 0;

      // TODO: JZ : As far as I can tell, these getCell() calls are relatively unused, and may perhaps be replaced by getSignal() calls

      /// Get the cell at the specified index/increment.
      virtual const Mantid::Geometry::SignalAggregate& getCell(size_t dim1Increment) const = 0;

      /// Get the cell at the specified index/increment.
      virtual const Mantid::Geometry::SignalAggregate& getCell(size_t dim1Increment, size_t dim2Increment) const = 0;

      /// Get the cell at the specified index/increment.
      virtual const Mantid::Geometry::SignalAggregate& getCell(size_t dim1Increment, size_t dim2Increment, size_t dim3Increment) const = 0;

      /// Get the cell at the specified index/increment.
      virtual const Mantid::Geometry::SignalAggregate& getCell(size_t dim1Increment, size_t dim2Increment, size_t dim3Increment, size_t dim4Increment) const = 0;

      /// Get the cell at the specified index/increment.
      virtual const Mantid::Geometry::SignalAggregate& getCell(...) const = 0;

      /// Horace sytle implementations need to have access to the underlying file. 
      virtual std::string getWSLocation() const = 0;

      /// All MD type workspaces have an effective geometry. MD type workspaces must provide this geometry in a serialized format.
      virtual std::string getGeometryXML() const = 0;

      /// Creates a new iterator pointing to the first cell in the workspace
      virtual IMDIterator* createIterator() const;

      virtual ~IMDWorkspace();

    };
    
    /// Shared pointer to the matrix workspace base class
    typedef boost::shared_ptr<IMDWorkspace> IMDWorkspace_sptr;
    /// Shared pointer to the matrix workspace base class (const version)
    typedef boost::shared_ptr<const IMDWorkspace> IMDWorkspace_const_sptr;
  }
}
#endif //MANTID_API_IMDWORKSPACE_H_

