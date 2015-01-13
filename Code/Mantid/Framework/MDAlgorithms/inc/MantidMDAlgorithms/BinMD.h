#ifndef MANTID_MDEVENTS_BINMD_H_
#define MANTID_MDEVENTS_BINMD_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/CoordTransform.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidKernel/System.h"
#include "MantidKernel/VMD.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidMDAlgorithms/SlicingAlgorithm.h"

namespace Mantid {
namespace Geometry {
// Forward declaration
class MDImplicitFunction;
}
namespace MDAlgorithms {

/** Take a MDEventWorkspace and bin it to a dense histogram
 * in a MDHistoWorkspace. This is principally used for visualization.
 *
 * The output workspace may have fewer
 * dimensions than the input MDEventWorkspace.
 *
 * @author Janik Zikovsky
 * @date 2011-03-29 11:28:06.048254
 */
class DLLExport BinMD : public SlicingAlgorithm {
public:
  BinMD();
  ~BinMD();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "BinMD"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Take a MDEventWorkspace and bin into into a dense, "
           "multi-dimensional histogram workspace (MDHistoWorkspace).";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 1; }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "MDAlgorithms"; }

private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();

  //    /// Helper method
  //    template<typename MDE, size_t nd>
  //    void do_centerpointBin(typename MDEventWorkspace<MDE, nd>::sptr ws);

  /// Helper method
  template <typename MDE, size_t nd>
  void binByIterating(typename MDEvents::MDEventWorkspace<MDE, nd>::sptr ws);

  /// Method to bin a single MDBox
  template <typename MDE, size_t nd>
  void binMDBox(MDEvents::MDBox<MDE, nd> *box, const size_t *const chunkMin,
                const size_t *const chunkMax);

  /// The output MDHistoWorkspace
  Mantid::MDEvents::MDHistoWorkspace_sptr outWS;
  /// Progress reporting
  Mantid::API::Progress *prog;
  /// ImplicitFunction used
  Mantid::Geometry::MDImplicitFunction *implicitFunction;

  /// Cached values for speed up
  size_t *indexMultiplier;
  signal_t *signals;
  signal_t *errors;
  signal_t *numEvents;
};

} // namespace Mantid
} // namespace MDEvents

#endif /* MANTID_MDEVENTS_BINMD_H_ */
