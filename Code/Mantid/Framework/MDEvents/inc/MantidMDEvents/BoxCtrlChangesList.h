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

    File/ change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>

   */
 template<class T >
 class BoxCtrlChangesList: public API::BoxController
 {
    //-----------------------------------------------------------------------------------
    /** Add a MDBox pointer to the list of boxes to split.
     * Thread-safe for adding.
     * No duplicate checking is done!
     *
     * @param ptr :: void ptr that casts to a particular MDBox<> * type.
     */
 public:
   void addBoxToSplit(const T &theBox)
    {
      m_boxesToSplitMutex.lock();
      m_boxesToSplit.push_back(theBox);
      m_boxesToSplitMutex.unlock();
    }

    //-----------------------------------------------------------------------------------
    /** Get a reference to the vector of boxes that must be split.
     * Not thread safe!
     */
    std::vector<T> getBoxesToSplit()const
    {
      return m_boxesToSplit;
    }
    //-----------------------------------------------------------------------------------
    /** Get a reference to the vector of BoxesToSplit that can be split.
     * thread safe!
     */

    template<class MDBoxToChange >
    std::vector< MDBoxToChange > getBoxesToSplit()const
    {
      m_boxesToSplitMutex.lock();
      return m_boxesToSplit;
      m_boxesToSplitMutex.unlock();
    }
    //-----------------------------------------------------------------------------------
    /** Clears the list of boxes that are big enough to split */
    void clearBoxesToSplit()
    {
      m_boxesToSplitMutex.lock();
      m_boxesToSplit.clear();
      m_boxesToSplitMutex.unlock();
    }
    /**Copy constructor from a box controller pointer */
    BoxCtrlChangesList(const API::BoxController & theController):
    BoxController(theController)
    {
      auto *bc = dynamic_cast<const BoxCtrlChangesList<T>* >(&theController);
      if(bc)m_boxesToSplit.assign(bc->m_boxesToSplit.begin(),bc->m_boxesToSplit.end());
    }

    /**Copy constructor from a BoxCtrlChangesList, not default as mutex can not be copied */
    BoxCtrlChangesList(const BoxCtrlChangesList & other):
    BoxController(other)
    {      
      m_boxesToSplit.assign(other.m_boxesToSplit.begin(),other.m_boxesToSplit.end());
    }

    /**constructor with number of dimensions */
    BoxCtrlChangesList(size_t nd):BoxController(nd){};

 private:
//
    /// Mutex for modifying the m_boxesToSplit member
    Mantid::Kernel::Mutex m_boxesToSplitMutex;

     /// Vector of MDBoxes to change 
    std::vector<T> m_boxesToSplit;
 };

}
}
#endif