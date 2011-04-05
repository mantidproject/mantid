#ifndef MDEVENTSTEST_HELPER_H
#define MDEVENTSTEST_HELPER_H
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidTestHelpers/DLLExport.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/BoxController.h"
#include "MantidKernel/Utils.h"

/** Set of helper methods for testing MDEventWorkspace things
 *
 * @author Janik Zikovsky
 * @date March 29, 2011
 * */
namespace MDEventsHelper
{


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
          Mantid::MDEvents::CoordType centers[nd];
          for (size_t d=0; d<nd; d++)
            centers[d] = min + (index[d]+0.5)*(max-min)/splitInto;
          out->addEvent( Mantid::MDEvents::MDEvent<nd>(1.0, 1.0, centers) );
        }

        allDone = Mantid::Kernel::Utils::nestedForLoopIncrement(nd, index, index_max);
      }
      out->refreshCache();
    }

    return out;
  }



} // namespace


#endif
