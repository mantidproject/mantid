#ifndef MANTID_MDEVENTS_BINTOMDHISTOWORKSPACE_H_
#define MANTID_MDEVENTS_BINTOMDHISTOWORKSPACE_H_
/*WIKI* 

This algorithm performs dense binning of the events in multiple dimensions of an input [[MDEventWorkspace]] and places them into a dense MDHistoWorkspace with 1-4 dimensions.

The input MDEventWorkspace may have more dimensions than the number of output dimensions. The names of the dimensions in the DimX, etc. parameters are used to find the corresponding dimensions that will be created in the output.

An ImplicitFunction can be defined using the ImplicitFunctionXML parameter; any points NOT belonging inside of the ImplicitFunction will be set as NaN (not-a-number). 

As of now, binning is only performed along axes perpendicular to the dimensions defined in the MDEventWorkspace.
*WIKI*/

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/CoordTransform.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include "MantidKernel/VMD.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidMDEvents/SlicingAlgorithm.h"

using Mantid::API::IMDEventWorkspace_sptr;


namespace Mantid
{
  namespace Geometry
  {
    //Forward declaration
    class MDImplicitFunction;
  }
namespace MDEvents
{

  /** Take a MDEventWorkspace and bin it to a dense histogram
   * in a MDHistoWorkspace. This is principally used for visualization.
   * 
   * The output workspace may have fewer
   * dimensions than the input MDEventWorkspace.
   *
   * @author Janik Zikovsky
   * @date 2011-03-29 11:28:06.048254
   */
  class DLLExport BinToMDHistoWorkspace  : public SlicingAlgorithm
  {
  public:
    BinToMDHistoWorkspace();
    ~BinToMDHistoWorkspace();

    /// Algorithm's name for identification
    virtual const std::string name() const { return "BinToMDHistoWorkspace";};
    /// Algorithm's version for identification
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "MDEvents";}

  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    /// Initialise the properties
    void init();
    /// Run the algorithm
    void exec();

    /// Helper method
    template<typename MDE, size_t nd>
    void do_centerpointBin(typename MDEventWorkspace<MDE, nd>::sptr ws);

    /// Helper method
    template<typename MDE, size_t nd>
    void binByIterating(typename MDEventWorkspace<MDE, nd>::sptr ws);

    /// Method to bin a single MDBox
    template<typename MDE, size_t nd>
    void binMDBox(MDBox<MDE, nd> * box, size_t * chunkMin, size_t * chunkMax);


    /// The output MDHistoWorkspace
    Mantid::MDEvents::MDHistoWorkspace_sptr outWS;
    /// Progress reporting
    Mantid::API::Progress * prog;
    /// ImplicitFunction used
    Mantid::Geometry::MDImplicitFunction * implicitFunction;

    /// Cached values for speed up
    size_t * indexMultiplier;
    signal_t * signals;
    signal_t * errors;


  };


} // namespace Mantid
} // namespace MDEvents

#endif  /* MANTID_MDEVENTS_BINTOMDHISTOWORKSPACE_H_ */
