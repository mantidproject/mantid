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
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"

namespace Mantid
{


  namespace API
  {

    class IMDIterator;
    
    /** Enum describing different ways to normalize the signal
     * in a MDWorkspace.
     */
    enum MDNormalization
    {
      /// Don't normalize = return raw counts
      NoNormalization = 0,
      /// Divide the signal by the volume of the box/bin
      VolumeNormalization = 1,
      /// Divide the signal by the number of events that contributed to it.
      NumEventsNormalization  = 2
    };


    /** Basic MD Workspace Abstract Class.
     *
     *  This defines the interface that allows one to iterate through several types of workspaces:
     *
     *   - The regularly gridded MDHistoWorkspace
     *   - The recursively binned MDEventWorkspace
     *   - The regular (2D) MatrixWorkspace.
     *  
     @author Janik Zikovsky
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
      
      /// Get the number of points associated with the workspace.
      /// For MDEvenWorkspace it is the number of events contributing into the workspace
      /// For regularly gridded workspace (MDHistoWorkspace and MatrixWorkspace), it is
      /// the number of bins.
      virtual uint64_t getNPoints() const = 0;

      /// Creates a new iterator pointing to the first cell in the workspace
      virtual IMDIterator* createIterator(Mantid::Geometry::MDImplicitFunction * function = NULL) const;

      /// Returns the (normalized) signal at a given coordinates
      virtual signal_t getSignalAtCoord(const coord_t * coords) const
      {
        UNUSED_ARG(coords);
        throw std::runtime_error("getSignalAtCoord() not implemented.");
      }

      //-------------------------------------------------------------------------------------------
      /// Returns the signal (normalized by volume) at a given coordinates
      /// @param coords :: coordinate as a VMD vector
      signal_t getSignalAtCoord(const Mantid::Kernel::VMD & coords) const
      {
        return this->getSignalAtCoord(coords.getBareArray());
      }

      /// Method to generate a line plot through a MD-workspace
      virtual void getLinePlot(const Mantid::Kernel::VMD & start, const Mantid::Kernel::VMD & end,
          Mantid::API::MDNormalization normalize, std::vector<coord_t> & x, std::vector<signal_t> & y) = 0;

      virtual ~IMDWorkspace();

    };
    
    /// Shared pointer to the IMDWorkspace base class
    typedef boost::shared_ptr<IMDWorkspace> IMDWorkspace_sptr;
    /// Shared pointer to the IMDWorkspace base class (const version)
    typedef boost::shared_ptr<const IMDWorkspace> IMDWorkspace_const_sptr;
  }
}
#endif //MANTID_API_IMDWORKSPACE_H_

