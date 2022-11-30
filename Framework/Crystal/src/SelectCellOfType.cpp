// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/SelectCellOfType.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidCrystal/SelectCellWithForm.h"
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Crystal/ScalarUtils.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid::Crystal {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SelectCellOfType)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

/** Initialize the algorithm's properties.
 */
void SelectCellOfType::init() {
  this->declareProperty(std::make_unique<WorkspaceProperty<IPeaksWorkspace>>("PeaksWorkspace", "", Direction::InOut),
                        "Input Peaks Workspace");

  std::vector<std::string> type_list;
  type_list.emplace_back(ReducedCell::CUBIC());
  type_list.emplace_back(ReducedCell::HEXAGONAL());
  type_list.emplace_back(ReducedCell::RHOMBOHEDRAL());
  type_list.emplace_back(ReducedCell::TETRAGONAL());
  type_list.emplace_back(ReducedCell::ORTHORHOMBIC());
  type_list.emplace_back(ReducedCell::MONOCLINIC());
  type_list.emplace_back(ReducedCell::TRICLINIC());

  declareProperty("CellType", type_list[0], std::make_shared<Kernel::StringListValidator>(type_list),
                  "The conventional cell type to use");

  std::vector<std::string> centering_list;
  centering_list.emplace_back(ReducedCell::F_CENTERED());
  centering_list.emplace_back(ReducedCell::I_CENTERED());
  centering_list.emplace_back(ReducedCell::C_CENTERED());
  centering_list.emplace_back(ReducedCell::P_CENTERED());
  centering_list.emplace_back(ReducedCell::R_CENTERED());

  declareProperty("Centering", centering_list[3], std::make_shared<Kernel::StringListValidator>(centering_list),
                  "The centering for the conventional cell");

  this->declareProperty("Apply", false, "Update UB and re-index the peaks");
  this->declareProperty("Tolerance", 0.12, "Indexing Tolerance");

  this->declareProperty(std::make_unique<PropertyWithValue<int>>("NumIndexed", 0, Direction::Output),
                        "The number of indexed peaks if apply==true.");

  this->declareProperty(std::make_unique<PropertyWithValue<double>>("AverageError", 0.0, Direction::Output),
                        "The average HKL indexing error if apply==true.");

  this->declareProperty("AllowPermutations", true, "Allow permutations of conventional cells");

  this->declareProperty(std::make_unique<ArrayProperty<double>>("TransformationMatrix", Direction::Output),
                        "The transformation matrix");
}

/** Execute the algorithm.
 */
void SelectCellOfType::exec() {
  IPeaksWorkspace_sptr ws = this->getProperty("PeaksWorkspace");
  if (!ws) {
    throw std::runtime_error("Could not read the peaks workspace");
  }

  // copy current lattice
  Matrix<double> UB = ws->sample().getOrientedLattice().getUB();

  if (!IndexingUtils::CheckUB(UB)) {
    throw std::runtime_error("ERROR: The stored UB is not a valid orientation matrix");
  }

  std::string cell_type = this->getProperty("CellType");
  std::string centering = this->getProperty("Centering");
  bool apply = this->getProperty("Apply");
  double tolerance = this->getProperty("Tolerance");
  bool allowPermutations = this->getProperty("AllowPermutations");

  std::vector<ConventionalCell> list = ScalarUtils::GetCells(UB, cell_type, centering, allowPermutations);

  ConventionalCell info = ScalarUtils::GetCellBestError(list, true);

  DblMatrix newUB = info.GetNewUB();

  std::string message = info.GetDescription() + " Lat Par:" + IndexingUtils::GetLatticeParameterString(newUB);

  g_log.notice(std::string(message));

  DblMatrix T = info.GetHKL_Tran();
  g_log.notice() << "Reduced to Conventional Cell Transformation Matrix =  " << T.str() << '\n';
  this->setProperty("TransformationMatrix", T.getVector());

  if (apply) {
    int num_indexed;
    double average_error = 0.0;

    SelectCellWithForm::ApplyTransform(newUB, ws, tolerance, num_indexed, average_error);

    // Tell the user what happened.
    g_log.notice() << "Re-indexed the peaks with the new UB. \n";
    g_log.notice() << "Now, " << num_indexed << " are indexed with average error " << average_error << '\n';

    // Save output properties
    this->setProperty("NumIndexed", num_indexed);
    this->setProperty("AverageError", average_error);
  }
}

} // namespace Mantid::Crystal
