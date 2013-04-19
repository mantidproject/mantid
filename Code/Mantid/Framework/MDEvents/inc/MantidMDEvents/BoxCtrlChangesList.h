#ifndef MANTID_BOXCONTROLLER_CHANGELIST_H_
#define MANTID_BOXCONTROLLER_CHANGELIST_H_

#include "MantidKernel/System.h"
#include "MantidKernel/ThreadPool.h"
#include <vector>
#include "MantidAPI/BoxController.h"
#include "MantidMDEvents/MDBoxToChange.h"


namespace Mantid
{
namespace MDEvents
{
 /** This class is used to keep list of boxes, which have to be eventually split
  
   *
   * @date  27/07/2012

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File/ change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>

   */
 class BoxCtrlChangesList: public API::BoxController
 {
    //-----------------------------------------------------------------------------------
    /** Add a MDBox pointer to the list of boxes to split.
     * Thread-safe for adding.
     * No duplicate checking is done!
     *
     * @param theBox -- Box to split
     */
 public:
   void addBoxToSplit(API::IMDNode *theBox)
    {
      Kernel::Mutex::ScopedLock _lock(m_boxesToSplitMutex);
      m_boxesToSplit.push_back(theBox);
    }

       //-----------------------------------------------------------------------------------
    /** Get a reference to the vector of BoxesToSplit that can be split.
     * thread safe!
     */
    std::vector< API::IMDNode *> getBoxesToSplit()const
    {
      Kernel::Mutex::ScopedLock _lock(m_boxesToSplitMutex);
      return m_boxesToSplit;
    }
    //-----------------------------------------------------------------------------------
    /** Clears the list of boxes that are big enough to split */
    void clearBoxesToSplit()
    {
      Kernel::Mutex::ScopedLock _lock(m_boxesToSplitMutex);
      m_boxesToSplit.clear();
    }
    /**constructor with number of dimensions */
    BoxCtrlChangesList(size_t nd):BoxController(nd){};

    BoxController * clone()const
    {
        return new BoxCtrlChangesList(*this);
    }
 private: 
    /**Copy constructor from a BoxCtrlChangesList*/
    BoxCtrlChangesList(const BoxCtrlChangesList & other):
    BoxController(other)
    {      
      m_boxesToSplit.assign(other.m_boxesToSplit.begin(),other.m_boxesToSplit.end());
    }


    /// Mutex for modifying the m_boxesToSplit member
    mutable Mantid::Kernel::Mutex m_boxesToSplitMutex;

     /// Vector of MDBoxes to change 
    std::vector<API::IMDNode *> m_boxesToSplit;
 };

}
}
#endif
