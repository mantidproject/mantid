// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/ShowPossibleCells.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Crystal/ScalarUtils.h"
#include "MantidJson/Json.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"

#include <boost/algorithm/string/replace.hpp>
#include <json/json.h>

namespace Mantid::Crystal {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ShowPossibleCells)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

/** Initialize the algorithm's properties.
 */
void ShowPossibleCells::init() {
  this->declareProperty(std::make_unique<WorkspaceProperty<IPeaksWorkspace>>("PeaksWorkspace", "", Direction::InOut),
                        "Input Peaks Workspace");

  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);

  this->declareProperty(
      std::make_unique<PropertyWithValue<double>>("MaxScalarError", 0.2, mustBePositive, Direction::Input),
      "Max Scalar Error (0.2)");

  this->declareProperty("BestOnly", true, "Show at most one for each Bravais Lattice");

  this->declareProperty(std::make_unique<PropertyWithValue<int>>("NumberOfCells", 0, Direction::Output),
                        "Gets set with the number of possible cells.");

  this->declareProperty("AllowPermutations", true, "Allow permutations of conventional cells");

  this->declareProperty(std::make_unique<Mantid::Kernel::ArrayProperty<std::string>>("Cells", Direction::Output),
                        "A list of the different cells");
}

/** Execute the algorithm.
 */
void ShowPossibleCells::exec() {
  IPeaksWorkspace_const_sptr ws = this->getProperty("PeaksWorkspace");
  if (!ws) {
    throw std::runtime_error("Could not read the peaks workspace");
  }

  OrientedLattice o_lattice = ws->sample().getOrientedLattice();
  const Matrix<double> &UB = o_lattice.getUB();

  if (!IndexingUtils::CheckUB(UB)) {
    throw std::runtime_error("ERROR: The stored UB is not a valid orientation matrix");
  }

  double max_scalar_error = this->getProperty("MaxScalarError");
  bool best_only = this->getProperty("BestOnly");
  bool allowPermutations = this->getProperty("AllowPermutations");

  std::vector<ConventionalCell> list = ScalarUtils::GetCells(UB, best_only, allowPermutations);

  ScalarUtils::RemoveHighErrorForms(list, max_scalar_error);

  size_t num_cells = list.size();

  // now tell the user the number of possible conventional cells:
  g_log.notice() << "Num Cells : " << num_cells << '\n';

  std::vector<std::string> cells;

  for (size_t i = 0; i < num_cells; i++) {
    DblMatrix newUB = list[i].GetNewUB();
    std::string message = list[i].GetDescription() + " Lat Par:" + IndexingUtils::GetLatticeParameterString(newUB);

    g_log.notice(std::string(message));

    Json::Value root;
    root["Error"] = list[i].GetError();
    root["FormNumber"] = static_cast<uint32_t>(list[i].GetFormNum());
    root["CellType"] = list[i].GetCellType();
    root["Centering"] = list[i].GetCentering();
    Json::Value outUB;
    for (double x : newUB.getVector())
      outUB.append(x);
    root["UB"] = outUB;

    std::vector<double> lattice_parameters;
    IndexingUtils::GetLatticeParameters(newUB, lattice_parameters);
    root["a"] = lattice_parameters[0];
    root["b"] = lattice_parameters[1];
    root["c"] = lattice_parameters[2];
    root["alpha"] = lattice_parameters[3];
    root["beta"] = lattice_parameters[4];
    root["gamma"] = lattice_parameters[5];
    root["volume"] = lattice_parameters[6];

    cells.push_back(Mantid::JsonHelpers::jsonToString(root));
  }

  this->setProperty("NumberOfCells", static_cast<int>(num_cells));
  this->setProperty("Cells", cells);
}

} // namespace Mantid::Crystal
