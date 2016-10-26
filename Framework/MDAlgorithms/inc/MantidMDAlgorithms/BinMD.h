#ifndef MANTID_MDALGORITHMS_BINMD_H_
#define MANTID_MDALGORITHMS_BINMD_H_

#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/MDBox.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
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

  /// Algorithm's name for identification
  const std::string name() const override { return "BinMD"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Take a MDEventWorkspace and bin into into a dense, "
           "multi-dimensional histogram workspace (MDHistoWorkspace).";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "MDAlgorithms\\Slicing";
  }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  //    /// Helper method
  //    template<typename MDE, size_t nd>
  //    void do_centerpointBin(typename MDEventWorkspace<MDE, nd>::sptr ws);

  /// Helper method
  template <typename MDE, size_t nd>
  void binByIterating(typename DataObjects::MDEventWorkspace<MDE, nd>::sptr ws);

  /// Method to bin a single MDBox
  template <typename MDE, size_t nd>
  void binMDBox(DataObjects::MDBox<MDE, nd> *box, const size_t *const chunkMin,
                const size_t *const chunkMax);

  /// The output MDHistoWorkspace
  Mantid::DataObjects::MDHistoWorkspace_sptr m_outWS;

  /// Progress reporting
  std::unique_ptr<Mantid::API::Progress> m_progress;

  /// ImplicitFunction used
  Mantid::Geometry::MDImplicitFunction *m_implicitFunction;

  /// Cached values for speed up
  size_t *m_indexMultiplier;
  signal_t *m_signals;
  signal_t *m_errors;
  signal_t *m_numEvents;

};

} // namespace Mantid
} // namespace DataObjects

#endif /* MANTID_MDALGORITHMS_BINMD_H_ */
