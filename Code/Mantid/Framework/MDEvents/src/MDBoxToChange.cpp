#include "MantidMDEvents/MDBoxToChange.h"
#include <limits>

namespace Mantid
{
namespace MDEvents
{

    //template<typename MDE,size_t nd>
    //MDBoxToChange<MDE,nd>::MDBoxToChange(MDBox<MDE,nd> *box,size_t Index)
 


    //template<typename MDE,size_t nd>
    //MDGridBox<MDE,nd>* MDBoxToChange<MDE,nd>::splitToGridBox()
 
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