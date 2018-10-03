// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CRYSTAL_SELECT_CELL_WITH_FORM_H_
#define MANTID_CRYSTAL_SELECT_CELL_WITH_FORM_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Crystal {
/** ShowPossibleCells : Algorithm to display a list of possible conventional
    cells corresponding to the UB saved in the sample associated
    with the specified PeaksWorkspace, provided the saved UB is for a Niggli
    reduced cell.

    @author Dennis Mikkelson
    @date   2012-02-13
  */
class DLLExport SelectCellWithForm : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "SelectCellWithForm"; };

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"FindUBUsingFFT", "FindUBUsingIndexedPeaks",
            "FindUBUsingLatticeParameters"};
  }

  /// Algorithm's category for identification
  const std::string category() const override { return "Crystal\\Cell"; }

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Select a conventional cell with a specific form number, "
           "corresponding to the UB stored with the sample for this peaks "
           "works space.";
  }

  static Kernel::Matrix<double>
  DetermineErrors(std::vector<double> &sigabc, const Kernel::Matrix<double> &UB,
                  const DataObjects::PeaksWorkspace_sptr &ws, double tolerance);

private:
  /// Initialise the properties
  void init() override;

  /// Run the algorithm
  void exec() override;
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_SELECT_CELL_WITH_FORM_H */
