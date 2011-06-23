#ifndef MDEVENTSTEST_HELPER_H
#define MDEVENTSTEST_HELPER_H

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Utils.h"
#include "MantidMDEvents/BoxController.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidTestHelpers/DLLExport.h"

namespace Mantid
{
namespace MDEvents
{


/** Set of helper methods for testing MDEventWorkspace things
 *
 * @author Janik Zikovsky
 * @date March 29, 2011
 * */
namespace MDEventsTestHelper
{

  /** Create an EventWorkspace containing fake data
   * of single-crystal diffraction.
   * Instrument is MINITOPAZ
   *
   * @return EventWorkspace_sptr
   */
  DLL_TESTHELPERS Mantid::DataObjects::EventWorkspace_sptr createDiffractionEventWorkspace(int numEvents);


  //-------------------------------------------------------------------------------------
  /** Create a test MDEventWorkspace<nd> . Dimensions are names Axis0, Axis1, etc.
   *
   * @param splitInto :: each dimension will split into this many subgrids
   * @param min :: extent of each dimension (min)
   * @param max :: extent of each dimension (max)
   * @param numEventsPerBox :: will create one MDEvent in the center of each sub-box.
   *        0 = don't split box, don't add events
   * @return
   */
  template<size_t nd>
  boost::shared_ptr<Mantid::MDEvents::MDEventWorkspace<Mantid::MDEvents::MDEvent<nd>,nd> >
    makeMDEW(size_t splitInto, double min, double max, size_t numEventsPerBox = 0)
  {
    boost::shared_ptr<Mantid::MDEvents::MDEventWorkspace<Mantid::MDEvents::MDEvent<nd>,nd> >
            out(new Mantid::MDEvents::MDEventWorkspace<Mantid::MDEvents::MDEvent<nd>,nd>());
    Mantid::MDEvents::BoxController_sptr bc(new Mantid::MDEvents::BoxController(nd));
    bc->setSplitThreshold(100);
    bc->setSplitInto(splitInto);
    out->setBoxController(bc);

    for (size_t d=0; d<nd;d++)
    {
      std::ostringstream name;
      name << "Axis" << d;
      Mantid::Geometry::MDHistoDimension_sptr dim(new Mantid::Geometry::MDHistoDimension( name.str(),name.str(), "m", min, max, 0));
      out->addDimension(dim);
    }
    out->initialize();

    if (numEventsPerBox > 0)
    {
      out->splitBox();
      size_t * index = Mantid::Kernel::Utils::nestedForLoopSetUp(nd);
      size_t * index_max = Mantid::Kernel::Utils::nestedForLoopSetUp(nd, splitInto);
      bool allDone = false;
      while (!allDone)
      {
        for (size_t i=0; i < numEventsPerBox; i++)
        {
          // Put an event in the middle of each box
          Mantid::coord_t centers[nd];
          for (size_t d=0; d<nd; d++)
            centers[d] = min + (double(index[d])+0.5)*(max-min)/double(splitInto);
          out->addEvent( Mantid::MDEvents::MDEvent<nd>(1.0, 1.0, centers) );
        }

        allDone = Mantid::Kernel::Utils::nestedForLoopIncrement(nd, index, index_max);
      }
      out->refreshCache();
    }

    return out;
  }




  //=====================================================================================
  //===================================== HELPER METHODS ================================
  //=====================================================================================

  //-------------------------------------------------------------------------------------
  /** Generate an empty MDBox */
  static MDBox<MDEvent<1>,1> * makeMDBox1(size_t splitInto=10)
  {
    // Split at 5 events
    BoxController_sptr splitter(new BoxController(1));
    splitter->setSplitThreshold(5);
    // Splits into 10 boxes
    splitter->setSplitInto(splitInto);
    // Set the size
    MDBox<MDEvent<1>,1> * out = new MDBox<MDEvent<1>,1>(splitter);
    out->setExtents(0, 0.0, 10.0);
    out->calcVolume();
    return out;
  }

  //-------------------------------------------------------------------------------------
  /** Generate an empty MDBox with 3 dimensions, split 10x5x2 */
  static MDBox<MDEvent<3>,3> * makeMDBox3()
  {
    // Split at 5 events
    BoxController_sptr splitter(new BoxController(3));
    splitter->setSplitThreshold(5);
    // Splits into 10x5x2 boxes
    splitter->setSplitInto(10);
    splitter->setSplitInto(1,5);
    splitter->setSplitInto(2,2);
    // Set the size to 10.0 in all directions
    MDBox<MDEvent<3>,3> * out = new MDBox<MDEvent<3>,3>(splitter);
    for (size_t d=0; d<3; d++)
      out->setExtents(d, 0.0, 10.0);
    return out;
  }


  //-------------------------------------------------------------------------------------
  /** Generate an empty MDBox with 2 dimensions, splitting in (default) 10x10 boxes.
   * Box size is 10x10.
   *
   * @param split0, split1 :: for uneven splitting
   * */
  template <size_t nd>
  static MDGridBox<MDEvent<nd>,nd> * makeMDGridBox(size_t split0=10, size_t split1=10, coord_t dimensionMin=0.0, coord_t dimensionMax=10.0)
  {
    // Split at 5 events
    BoxController_sptr splitter(new BoxController(nd));
    splitter->setSplitThreshold(5);
    // Splits into 10x10x.. boxes
    splitter->setSplitInto(split0);
    splitter->setSplitInto(0, split0);
    splitter->setSplitInto(1, split1);
    // Set the size to 10.0 in all directions
    MDBox<MDEvent<nd>,nd> * box = new MDBox<MDEvent<nd>,nd>(splitter);
    for (size_t d=0; d<nd; d++)
      box->setExtents(d, dimensionMin, dimensionMax);

    // Split
    MDGridBox<MDEvent<nd>,nd> * out = new MDGridBox<MDEvent<nd>,nd>(box);

    return out;
  }

  //-------------------------------------------------------------------------------------
  /** Feed a MDGridBox with evenly-spaced events
   *
   * @param box :: MDGridBox pointer
   * @param repeat :: how many events to stick in the same place
   * @param numPerSide :: e.g. if 10, and 3 dimensions, there will be 10x10x10 events
   * @param start :: x-coordinate starts at this for event 0
   * @param step :: x-coordinate increases by this much.
   */
  template <size_t nd>
  static void feedMDBox(MDGridBox<MDEvent<nd>,nd> * box, size_t repeat=1, size_t numPerSide=10, coord_t start=0.5, coord_t step=1.0)
  {
    size_t * counters = Mantid::Kernel::Utils::nestedForLoopSetUp(nd,0);
    size_t * index_max = Mantid::Kernel::Utils::nestedForLoopSetUp(nd,numPerSide);
    // Recursive for loop
    bool allDone = false;
    while (!allDone)
    {
      // Generate the position from the counter
      coord_t centers[nd];
      for (size_t d=0;d<nd;d++)
        centers[d] = double(counters[d])*step + start;

      // Add that event 'repeat' times
      for (size_t i=0; i<repeat; ++i)
        box->addEvent( MDEvent<nd>(1.0, 1.0, centers) );

      // Increment the nested for loop
      allDone = Mantid::Kernel::Utils::nestedForLoopIncrement(nd, counters, index_max);
    }
    box->refreshCache(NULL);
    delete [] counters;
    delete [] index_max;
  }


  //-------------------------------------------------------------------------------------
  /** Recursively split an existing MDGridBox
   *
   * @param box :: box to split
   * @param atRecurseLevel :: This is the recursion level at which we are
   * @param recurseLimit :: this is where to spot
   */
  template<size_t nd>
  static void recurseSplit(MDGridBox<MDEvent<nd>,nd> * box, size_t atRecurseLevel, size_t recurseLimit)
  {
    typedef std::vector<IMDBox<MDEvent<nd>,nd> *> boxVector;
    if (atRecurseLevel >= recurseLimit) return;

    // Split all the contents
    boxVector boxes;
    boxes = box->getBoxes();
    for (size_t i=0; i< boxes.size(); i++)
      box->splitContents(i);

    // Retrieve the contained MDGridBoxes
    boxes = box->getBoxes();

    // Go through them and split them
    for (size_t i=0; i< boxes.size(); i++)
    {
      MDGridBox<MDEvent<nd>,nd> * containedbox = dynamic_cast<MDGridBox<MDEvent<nd>,nd> *>(boxes[i]);
      if (containedbox)
        recurseSplit(containedbox, atRecurseLevel+1, recurseLimit);
    }
  }


  //-------------------------------------------------------------------------------------
  /** Generate a recursively gridded MDGridBox
   *
   * @param splitInto :: boxes split into this many boxes/side
   * @param levels :: levels of splitting recursion (0=just the top level is split)
   * @return
   */
  template<size_t nd>
  static MDGridBox<MDEvent<nd>,nd> * makeRecursiveMDGridBox(size_t splitInto, size_t levels)
  {
    // Split at 5 events
    BoxController_sptr splitter(new BoxController(nd));
    splitter->setSplitThreshold(5);
    splitter->resetNumBoxes();
    splitter->setMaxDepth(levels+1);
    // Splits into splitInto x splitInto x ... boxes
    splitter->setSplitInto(splitInto);
    // Set the size to splitInto*1.0 in all directions
    MDBox<MDEvent<nd>,nd> * box = new MDBox<MDEvent<nd>,nd>(splitter);
    for (size_t d=0; d<nd; d++)
      box->setExtents(d, 0.0, double(splitInto));
    // Split into the gridbox.
    MDGridBox<MDEvent<nd>,nd> * gridbox = new MDGridBox<MDEvent<nd>,nd>(box);

    // Now recursively split more
    recurseSplit(gridbox, 0, levels);

    return gridbox;
  }


  //-------------------------------------------------------------------------------------
  /** Return a vector with this many MDEvents, spaced evenly from 0.5, 1.5, etc. */
  static std::vector<MDEvent<1> > makeMDEvents1(size_t num)
  {
    std::vector<MDEvent<1> > out;
    for (double i=0; i<num; i++)
    {
      coord_t coords[1] = {i*1.0+0.5};
      out.push_back( MDEvent<1>(1.0, 1.0, coords) );
    }
    return out;
  }


  //-------------------------------------------------------------------------------------
  /** Helper function compares the extents of the given box */
  template<typename MDBOX>
  static void extents_match(MDBOX box, size_t dim, double min, double max)
  {
    TSM_ASSERT_DELTA(dim, box->getExtents(dim).min, min, 1e-6);
    TSM_ASSERT_DELTA(dim, box->getExtents(dim).max, max, 1e-6);
  }


  //=====================================================================================
  //===================================== TEST METHODS ==================================
  //=====================================================================================



} // namespace
}
}

#endif
