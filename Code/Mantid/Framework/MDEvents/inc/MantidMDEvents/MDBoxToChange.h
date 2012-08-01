#ifndef MDEVENTS_MDBOX_CHANGESLIST_H
#define MDEVENTS_MDBOX_CHANGESLIST_H
#include "MantidKernel/System.h"
#include "MantidKernel/ThreadScheduler.h"
#include "MantidAPI/BoxCtrlChangesInterface.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDGridBox.h"
namespace Mantid
{
  namespace MDEvents
  {
/** The class to contain and do operations over the list of MDBox-es which have been changed and need
   * to be split eventually
   *
   * @date 30-07-2012

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

//-----------------------------------------------    
    template<typename MDE, size_t nd>
    class DLLExport MDBoxToChange
    {
    public:
      MDBoxToChange():m_ParentGridBox(NULL),m_Index(std::numeric_limits<size_t>::max()-1){};
      MDBoxToChange(MDBox<MDE,nd> *box,size_t Index);

      size_t getIndex()const{return m_Index;}
      MDGridBox<MDE,nd>* splitToGridBox();

      MDGridBox<MDE,nd>* getParent()const{return m_ParentGridBox;}
    private:
      MDGridBox<MDE,nd>* m_ParentGridBox;
      size_t m_Index;
    };

  }
}

#endif