#include "MantidMDEvents/MDBoxToChange.h"
#include <limits>

namespace Mantid
{
namespace MDEvents
{

    template<typename MDE,size_t nd>
    MDBoxToChange<MDE,nd>::MDBoxToChange(MDBox<MDE,nd> *box,size_t Index)
    {
      m_Index = Index;
      MDGridBox<MDE, nd> * parent = dynamic_cast<MDGridBox<MDE, nd> * >(box->getParent());
      if(parent)
      {
          m_ParentGridBox=parent;
      }
      else  //HACK! if not parent, it is probably root a box -> such type should be created but meanwhile;
      {
          m_ParentGridBox=reinterpret_cast<MDGridBox<MDE,nd> *>(box);
          m_Index  = std::numeric_limits<size_t>::max();
      }

    }


    template<typename MDE,size_t nd>
    MDGridBox<MDE,nd>* MDBoxToChange<MDE,nd>::splitToGridBox()
    {
      MDBox<MDE, nd> *pMDBox;
      bool rootBox(false);
      // get the actual box to split:
      if(m_Index==std::numeric_limits<size_t>::max())
      {
        rootBox = true;
        pMDBox = reinterpret_cast<MDBox<MDE,nd>*>(m_ParentGridBox);
      }
      else   pMDBox = dynamic_cast<MDBox<MDE,nd>*>(m_ParentGridBox->getChild(m_Index));


     //  Construct the grid instead of box. This should take the object out of the disk MRU
       MDGridBox<MDE, nd> * gridbox = new MDGridBox<MDE, nd>(pMDBox);
      // Track how many MDBoxes there are in the overall workspace
       pMDBox->getBoxController()->trackNumBoxes(pMDBox->getDepth());


      if(rootBox) // carefull -- root pointer is not redefined here!
      {
       // this makes workspace data pointer invalid, but the actual pointer will remain dangling so care should be taken not to dereference it 
        delete pMDBox;  
        m_ParentGridBox = gridbox;
      }
      else
      {  // this will delete the old box and set new gridBox instead
         m_ParentGridBox->setChild(m_Index,gridbox);
      }
    // make this grid box undefined again
      m_Index=std::numeric_limits<size_t>::max()-1;
   
      return gridbox;
    }

  ///**convert input box into MDGridBox and split MDGrid box into its children
  // *
  // * @param   pointer to MDBox;
  // * @returns pointer to MDGridBox created instead of the input MDBox
  //*/
  //TMDE_CLASS(
  //bool MDBoxToChange)::splitAllIfNeeded(MDBoxToChange<MDE,nd> > &theCell,Kernel::ThreadScheduler * ts)  
  //{
  //  bool rootBoxSplit(false);
  //  auto pMDBox = theCell.getPBox();
  //  if(!pMDBox)throw(std::invalid_argument("should accept MDBox argument only "));
  //    // Construct the grid box. This should take the object out of the disk MRU
  //  MDGridBox<MDE, nd> * gridbox = new MDGridBox<MDE, nd>(pMDBox);
  // // Track how many MDBoxes there are in the overall workspace
  //  pMDBox->getBoxController()->trackNumBoxes(pMDBox->getDepth());

  //     // And now we have a gridded box instead of a boring old regular box.
  //  MDGridBox<MDE, nd> * parent = dynamic_cast<MDGridBox<MDE, nd> * >(gridbox->getParent());
  //  if(parent)
  //  {
  //    parent->setChild(theCell.getIndex(),gridbox);
  //  }
  //  else  // if not parent, it is probably root box;
  //  {
  //    rootBoxSplit = true;
  //    delete theCell.boxPointer; // this makes workspace data pointer invalid
  //    theCell.boxPointer = gridbox;
  //  }
  //  if (ts)
  //  {
  //    // Create a task to split the newly created MDGridBox.
  //    ts->push(new FunctionTask(boost::bind(&MDGridBox<MDE,nd>::splitAllIfNeeded, &*gridbox, ts) ) );
  //  }
  //  else
  //  {
  //    gridbox->splitAllIfNeeded(NULL);
  //  }
  //  return rootBoxSplit;
  //}


} // endNamespace MDExents
} // endNamespace Mantid;