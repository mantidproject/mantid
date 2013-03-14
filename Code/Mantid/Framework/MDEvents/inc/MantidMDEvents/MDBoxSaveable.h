#ifndef MANTID_MDEVENTS_MDBOX_SAVEABLE_H
#define MANTID_MDEVENTS_MDBOX_SAVEABLE_H

#include "MantidKernel/Saveable.h"
#include "MantidAPI/IMDNode.h"

namespace Mantid
{
namespace MDEvents
{

  //===============================================================================================
  /** The class responsible for implementing methods which automatically save/load MDBox in conjuction with 
      DiskBuffer

      @date March 15, 2013

      Copyright &copy; 2008-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

      File change history is stored at: <https://github.com/mantidproject/mantid>.
      Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
    class DLLExport MDBoxSaveable : public Kernel::Saveable
    {
        public:
            MDBoxSaveable(API::IMDNode *const, size_t ID);

            /// Save the data to the place, specified by the object
            virtual void save();

            /// Load the data which are not in memory yet and merge them with the data in memory;
            virtual void load();

   
        /// @return the amount of memory that the object takes up in the MRU.
        virtual uint64_t getTotalDataSize() const
            { return m_MDNode->getTotalDataSize(); }
        /** @return the size of the event vector. ! Note that this is NOT necessarily the same as the number of points 
                (because it might be cached to disk) or the size on disk (because you might have called AddEvents) */
        virtual size_t getDataMemorySize()const 
        {  return m_MDNode->getDataInMemorySize();}
        private:
            API::IMDNode *m_MDNode;
    };

}
}

#endif