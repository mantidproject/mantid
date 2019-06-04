// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/ShowPossibleCells.h"
#include "MantidAPI/Sample.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Crystal/ScalarUtils.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid {
namespace Crystal {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ShowPossibleCells)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

/** Initialize the algorithm's properties.
 */
void ShowPossibleCells::init() {
  this->declareProperty(std::make_unique<WorkspaceProperty<PeaksWorkspace>>(
                            "PeaksWorkspace", "", Direction::InOut),
                        "Input Peaks Workspace");

  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);

  this->declareProperty(
      std::make_unique<PropertyWithValue<double>>("MaxScalarError", 0.2,
                                             mustBePositive, Direction::Input),
      "Max Scalar Error (0.2)");

  this->declareProperty("BestOnly", true,
                        "Show at most one for each Bravais Lattice");

  this->declareProperty(std::make_unique<PropertyWithValue<int>>("NumberOfCells", 0,
                                                            Direction::Output),
                        "Gets set with the number of possible cells.");

  this->declareProperty("AllowPermutations", true,
                        "Allow permutations of conventional cells");
}

/** Execute the algorithm.
 */
void ShowPossibleCells::exec() {
  PeaksWorkspace_const_sptr ws = this->getProperty("PeaksWorkspace");
  if (!ws) {
    throw std::runtime_error("Could not read the peaks workspace");
  }

  OrientedLattice o_lattice = ws->sample().getOrientedLattice();
  const Matrix<double> &UB = o_lattice.getUB();

  if (!IndexingUtils::CheckUB(UB)) {
    throw std::runtime_error(
        "ERROR: The stored UB is not a valid orientation matrix");
  }

  double max_scalar_error = this->getProperty("MaxScalarError");
  bool best_only = this->getProperty("BestOnly");
  bool allowPermutations = this->getProperty("AllowPermutations");

  std::vector<ConventionalCell> list =
      ScalarUtils::GetCells(UB, best_only, allowPermutations);

  ScalarUtils::RemoveHighErrorForms(list, max_scalar_error);

  size_t num_cells = list.size();

  // now tell the user the number of possible conventional cells:
  g_log.notice() << "Num Cells : " << num_cells << '\n';

  for (size_t i = 0; i < num_cells; i++) {
    DblMatrix newUB = list[i].GetNewUB();
    std::string message = list[i].GetDescription() + " Lat Par:" +
                          IndexingUtils::GetLatticeParameterString(newUB);

    g_log.notice(std::string(message));
  }

  this->setProperty("NumberOfCells", static_cast<int>(num_cells));
}

} // namespace Crystal
} // namespace Mantid
