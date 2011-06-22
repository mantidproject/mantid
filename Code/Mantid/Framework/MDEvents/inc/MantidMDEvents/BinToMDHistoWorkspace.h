#ifndef MANTID_MDEVENTS_BINTOMDHISTOWORKSPACE_H_
#define MANTID_MDEVENTS_BINTOMDHISTOWORKSPACE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/BinToMDHistoWorkspace.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDHistoWorkspace.h"

namespace Mantid
{
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
  class DLLExport BinToMDHistoWorkspace  : public API::Algorithm
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

    /// Input binning dimensions
    std::vector<Mantid::Geometry::MDHistoDimension_sptr> binDimensionsIn;
    /// The output MDHistoWorkspace
    Mantid::MDEvents::MDHistoWorkspace_sptr outWS;
    /// Progress reporting
    Mantid::API::Progress * prog;
    /// ImplicitFunction used
    Mantid::API::ImplicitFunction * implicitFunction;



  };


} // namespace Mantid
} // namespace MDEvents

#endif  /* MANTID_MDEVENTS_BINTOMDHISTOWORKSPACE_H_ */
