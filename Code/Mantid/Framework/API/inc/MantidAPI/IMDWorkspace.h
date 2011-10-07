#ifndef MANTID_API_IMDWORKSPACE_H_
#define MANTID_API_IMDWORKSPACE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <stdint.h>
#include "MantidAPI/Workspace.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include <boost/shared_ptr.hpp>
#include <vector>
#include <stdarg.h>
#include "MantidAPI/MDGeometry.h"

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


    class MANTID_API_DLL IMDWorkspace : public Workspace, public Mantid::API::MDGeometry
    {
    public:
      
      /// Get the number of points associated with the workspace; For MD workspace it is number of points contributing into the workspace
        // TODO: -- what is its meaning for the matrix workspace, may be different or the same? different logic of the operations
      virtual uint64_t getNPoints() const = 0;

      /// Get those dimensions which have not been collapsed.
      virtual Mantid::Geometry::VecIMDDimension_const_sptr getNonIntegratedDimensions() const = 0;

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

