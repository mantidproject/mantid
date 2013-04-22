#ifndef MDEVENTS_MDBOX_CHANGESLIST_H
#define MDEVENTS_MDBOX_CHANGESLIST_H
#include "MantidKernel/System.h"
#include "MantidKernel/ThreadScheduler.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDGridBox.h"
namespace Mantid
{
  namespace MDEvents
  {
    /** The class to contains the information about an MDBox which has to be eventually split and aboul location of this box 
    * in the MDBox structure
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

    File/ change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */

    //-----------------------------------------------    

      class MDBoxToChange: API::IMDNode
      {
        public:
        MDBoxToChange():m_ParentGridBox(NULL),m_Index(std::numeric_limits<size_t>::max()-1){};

        size_t getIndex()const{return m_Index;}

        API::IMDNode* getParent()const{return m_ParentGridBox;}

      // Below for the time being:
      //MDGridBox<MDE,nd>* splitToGridBox();
      //MDBoxToChange(MDBox<MDE,nd> *box,size_t Index);

    private:
      /// the pointer to the greedbox, which contains box to split
      API::IMDNode* m_ParentGridBox;
      /// index of the box to split in the gridbox array
      size_t m_Index;
    public:  
      /**function checks if the containing box has enough data 
       * the definition "enough" is also specified within this function
      */
      bool isFull(size_t /*maxSize=1000*/)
      {
        /**stub */
        return true;
      }
      /**constructor */
      MDBoxToChange(API::IMDNode*box,size_t Index)
      {
        m_Index = Index;
        API::IMDNode* parent = box->getParent();
        if(parent)
        {
          m_ParentGridBox=parent;
        }
        else  //HACK! if not parent, it is probably root a box -> such type should be created but meanwhile;
        {
          m_ParentGridBox=box;
          m_Index  = std::numeric_limits<size_t>::max();
        }

      }

      /**DESCRIBE */
      API::IMDNode * splitToGridBox()
      {
        //API::IMDNode* pMDBox;
        bool rootBox(false);
        //// get the actual box to split: TODO: disabled until MD factory can generate a MDBox<nd> with reference to IMDNode
        if(m_Index==std::numeric_limits<size_t>::max())
        {

        //    rootBox = true;
        //    pMDBox = reinterpret_cast<MDBox<MDE,nd>*>(m_ParentGridBox);
        //}
        //else   
        //    pMDBox = dynamic_cast<MDBox<MDE,nd>*>(m_ParentGridBox->getChild(m_Index));


        ////  Construct the grid instead of box. This should take the object out of the disk MRU
        //MDGridBox<MDE, nd> * gridbox = new MDGridBox<MDE, nd>(pMDBox);
        //// Track how many MDBoxes there are in the overall workspace
        //pMDBox->getBoxController()->trackNumBoxes(pMDBox->getDepth());


        //if(rootBox) // carefull -- root pointer is not redefined here!
        //{
        //  // this makes workspace data pointer invalid, but the actual pointer will remain dangling so care should be taken not to dereference it 
        //  delete pMDBox;  
        //  m_ParentGridBox = gridbox;
        //}
        //else
        //{  // this will delete the old box and set new gridBox instead
        //  m_ParentGridBox->setChild(m_Index,gridbox);
        }
        // make this grid box undefined again
        m_Index=std::numeric_limits<size_t>::max()-1;

        //return gridbox;
        return NULL;
      }
    };

  }
}

#endif