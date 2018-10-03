// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CRYSTAL_SELECT_CELL_OF_TYPE_H_
#define MANTID_CRYSTAL_SELECT_CELL_OF_TYPE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Crystal {
/** ShowPossibleCells : Algorithm to display a list of possible conventional
    cells corresponding to the UB saved in the sample associated
    with the specified PeaksWorkspace, provided the saved UB is for a Niggli
    reduced cell.

    @author Dennis Mikkelson
    @date   2012-02-09
  */
class DLLExport SelectCellOfType : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "SelectCellOfType"; };

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
    return "Select a conventional cell with a specific lattice type and "
           "centering, corresponding to the UB stored with the sample for this "
           "peaks works space.";
  }

private:
  /// Initialise the properties
  void init() override;

  /// Run the algorithm
  void exec() override;
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_SELECT_CELL_OF_TYPE_H */
