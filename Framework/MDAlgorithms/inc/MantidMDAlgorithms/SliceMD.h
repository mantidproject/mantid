// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidMDAlgorithms/SlicingAlgorithm.h"

namespace Mantid {
namespace MDAlgorithms {

/** Algorithm that can take a slice out of an original MDEventWorkspace
 * while preserving all the events contained wherein.

  @author Janik Zikovsky
  @date 2011-09-27
*/
class DLLExport SliceMD : public SlicingAlgorithm {
public:
  //// enum describes situations, which could happen for input Box binning
  // enum BoxState
  //{
  //  boxTooSmall,  // too few events in the box worth considering its vertices
  //  boxOutside,   // box is not small but is completely out of slice
  //  boxWorthConsidering // box belongs to slice and its events worth
  //  considering for slicing
  //};

  /// Algorithm's name for identification
  const std::string name() const override { return "SliceMD"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Make a MDEventWorkspace containing the events in a slice of an "
           "input MDEventWorkspace.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"SliceMDHisto", "ProjectMD", "CutMD", "BinMD"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "MDAlgorithms\\Slicing"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  /// Helper method
  template <typename MDE, size_t nd> void doExec(typename DataObjects::MDEventWorkspace<MDE, nd>::sptr ws);

  /// Method to actually do the slice
  template <typename MDE, size_t nd, typename OMDE, size_t ond>
  void slice(typename DataObjects::MDEventWorkspace<MDE, nd>::sptr ws);

protected: // for testing
  /*  /// Method to slice box's events if the box itself belongs to the slice
    template<typename MDE, size_t nd, typename OMDE, size_t ond>
    void sliceMDBox(MDBox<MDE, nd> * box, size_t * chunkMin, size_t * chunkMax);

    template<typename MDE, size_t nd, typename OMDE, size_t ond>
    BoxState foundBoxState(MDBox<MDE, nd> * box, size_t * chunkMin, size_t *
    chunkMax);*/
};

} // namespace MDAlgorithms
} // namespace Mantid
