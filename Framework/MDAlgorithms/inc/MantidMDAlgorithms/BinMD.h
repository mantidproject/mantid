// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_BINMD_H_
#define MANTID_MDALGORITHMS_BINMD_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/CoordTransform.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidDataObjects/MDBox.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include "MantidKernel/System.h"
#include "MantidKernel/VMD.h"
#include "MantidMDAlgorithms/SlicingAlgorithm.h"

namespace Mantid {
namespace Geometry {
// Forward declaration
class MDImplicitFunction;
} // namespace Geometry
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
  const std::vector<std::string> seeAlso() const override {
    return {"SliceMDHisto", "ProjectMD", "CutMD", "SliceMD"};
  }
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
  Mantid::DataObjects::MDHistoWorkspace_sptr outWS;
  /// Progress reporting
  std::unique_ptr<Mantid::API::Progress> prog = nullptr;
  /// ImplicitFunction used
  Mantid::Geometry::MDImplicitFunction *implicitFunction;

  /// Cached values for speed up
  size_t *indexMultiplier;
  signal_t *signals;
  signal_t *errors;
  signal_t *numEvents;
  bool m_accumulate{false};
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_BINMD_H_ */
