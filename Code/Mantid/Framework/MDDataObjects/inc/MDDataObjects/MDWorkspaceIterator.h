#ifndef MD_WORKSPACE_ITERATOR_H
#define MD_WORKSPACE_ITERATOR_H

#include "MantidAPI/IMDIterator.h"
#include "MDDataObjects/MDIndexCalculator.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include <vector>

namespace Mantid
{
  namespace MDDataObjects
  {
    /** Concrete implementation of IMDIterator for use with MDWorkspaces. Based closely upon Roman Tolchenov's implementation
    used in MDFitWorkspace.

    @author Owen Arnold, Tessella Support Services plc
    @date 03/05/2011

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport MDWorkspaceIterator : public Mantid::API::IMDIterator
    {

    public:

      MDWorkspaceIterator(const MDWorkspaceIndexCalculator& indexCalculator, std::vector<Mantid::Geometry::IMDDimension_sptr> dimensions);

      ~MDWorkspaceIterator();

      /// Get the size of the data
      virtual size_t getDataSize() const;

      /// Get the i-th coordinate of the current cell
      virtual double getCoordinate(size_t i) const;

      /// Advance to the next cell. If the current cell is the last one in the workspace
      /// do nothing and return false.
      virtual bool next();

      ///< return the current data pointer (index)
      virtual size_t getPointer() const;

    private:

      const MDWorkspaceIndexCalculator m_indexCalculator;
      size_t m_cur_pointer;
      size_t m_end_pointer;
      std::vector<size_t> m_index;
      std::vector<boost::shared_ptr<Mantid::Geometry::IMDDimension> > m_dimensions;

    };
  }
}

#endif
