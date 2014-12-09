#ifndef MANTID_MDEVENTS_MDBOX_SAVEABLE_H
#define MANTID_MDEVENTS_MDBOX_SAVEABLE_H

#include "MantidKernel/ISaveable.h"
#include "MantidAPI/IMDNode.h"

namespace Mantid
{
    namespace MDEvents
    {

        //===============================================================================================
        /** Two classes responsible for implementing methods which automatically save/load MDBox in conjuction with 
        DiskBuffer
        One class responsible for saving events into nexus and another one -- for identifying the data positions in a file in conjuction with DB 

        @date March 15, 2013

        Copyright &copy; 2008-2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
        class DLLExport MDBoxSaveable : public Kernel::ISaveable
        {
        public:
            MDBoxSaveable(API::IMDNode *const);

            /// Save the data to the place, specified by the object
            virtual void save()const;

            /// Load the data which are not in memory yet and merge them with the data in memory;
            virtual void load();
            /// Method to flush the data to disk and ensure it is written.
            virtual void flushData()const;
            /// remove objects data from memory but keep all averages
            virtual void clearDataFromMemory()
            { m_MDNode->clearDataFromMemory();}


            /// @return the amount of memory that the object takes up in the MRU.
            virtual uint64_t getTotalDataSize() const
            { return m_MDNode->getTotalDataSize(); }
            /**@return the size of the event vector. ! Note that this is NOT necessarily the same as the number of points 
            (because it might be cached to disk) or the size on disk (because you might have called AddEvents) */
            virtual size_t getDataMemorySize()const 
            {  return m_MDNode->getDataInMemorySize();}

            ~MDBoxSaveable(){}
        private:
            API::IMDNode *const m_MDNode;
        };


    }
}

#endif