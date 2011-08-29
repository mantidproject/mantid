#ifndef MANTID_MDEVENTS_BINTOMDHISTOWORKSPACE_H_
#define MANTID_MDEVENTS_BINTOMDHISTOWORKSPACE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidMDEvents/MDBox.h"


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

    /// Helper method
    template<typename MDE, size_t nd>
    void binByIterating(typename MDEventWorkspace<MDE, nd>::sptr ws);

    /// Method to bin a single MDBox
    template<typename MDE, size_t nd>
    void binMDBox(MDBox<MDE, nd> * box, coord_t * chunkMin, coord_t * chunkMax);

    /// Input binning dimensions
    std::vector<Mantid::Geometry::MDHistoDimension_sptr> binDimensionsIn;
    /// The output MDHistoWorkspace
    Mantid::MDEvents::MDHistoWorkspace_sptr outWS;
    /// Progress reporting
    Mantid::API::Progress * prog;
    /// ImplicitFunction used
    Mantid::Geometry::MDImplicitFunction * implicitFunction;

    /// Bin dimensions to actually use
    std::vector<Mantid::Geometry::MDHistoDimension_sptr> binDimensions;
    /// Index of the dimension in the MDEW for the dimension in the output.
    std::vector<size_t> dimensionToBinFrom;

    /// Do we perform a coordinate transformation? NULL if no.
    CoordTransform * m_transform;

    /// Cached values for speed up
    size_t numBD;
    coord_t * min;
    coord_t * max;
    coord_t * step;
    size_t * indexMultiplier;
    signal_t * signals;
    signal_t * errors;


  };


} // namespace Mantid
} // namespace MDEvents

#endif  /* MANTID_MDEVENTS_BINTOMDHISTOWORKSPACE_H_ */
