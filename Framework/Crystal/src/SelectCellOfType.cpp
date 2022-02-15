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

  declareProperty("Apply", false, "Update UB and re-index the peaks");
  declareProperty("Tolerance", 0.12, "Indexing Tolerance");

  declareProperty(std::make_unique<PropertyWithValue<int>>("NumIndexed", 0, Direction::Output),
                  "The number of indexed peaks if apply==true.");

  declareProperty(std::make_unique<PropertyWithValue<double>>("AverageError", 0.0, Direction::Output),
                  "The average HKL indexing error if apply==true.");

  declareProperty("AllowPermutations", true, "Allow permutations of conventional cells");

  declareProperty(std::make_unique<ArrayProperty<double>>("TransformationMatrix", Direction::Output),
                  "The transformation matrix");
}

/**
 * @brief Validate input arguments and workspaces
 *
 * @return std::map<std::string, std::string>
 */
std::map<std::string, std::string> SelectCellOfType::validateInputs() {
  std::map<std::string, std::string> result;

  // Case 0: PeaksWorkspace is not valid
  IPeaksWorkspace_sptr ws = getProperty("PeaksWorkspace");
  if (!ws) {
    result["PeaksWorkspace"] = "Could not read the peaks workspace";
  }

  // Case 1: Attached UB is invalid
  auto const o_lattice = ws->mutableSample().getOrientedLattice();
  auto const UB = o_lattice.getUB();
  if (!IndexingUtils::CheckUB(UB)) {
    result["PeaksWorkspace"] = "ERROR: The stored UB is not a valid orientation matrix";
  }

  return result;
}

/** Execute the algorithm.
 */
void SelectCellOfType::exec() {
  // -- Parse input
  IPeaksWorkspace_sptr ws = getProperty("PeaksWorkspace");
  std::string cell_type = getProperty("CellType");
  std::string centering = getProperty("Centering");
  bool apply = getProperty("Apply");
  double tolerance = getProperty("Tolerance");
  bool allowPermutations = getProperty("AllowPermutations");

  // copy current lattice
  auto o_lattice = std::make_unique<OrientedLattice>(ws->mutableSample().getOrientedLattice());
  Matrix<double> UB = o_lattice->getUB();
  // NOTE: we need the previous mod vectors to find the new mod vectors after cell change
  auto modvector_0 = o_lattice->getModVec(0);
  auto modvector_1 = o_lattice->getModVec(1);
  auto modvector_2 = o_lattice->getModVec(2);

  std::vector<ConventionalCell> list = ScalarUtils::GetCells(UB, cell_type, centering, allowPermutations);
  ConventionalCell info = ScalarUtils::GetCellBestError(list, true);
  //
  DblMatrix newUB = info.GetNewUB();
  DblMatrix HKLTransformMatrix = info.GetHKL_Tran();
  modvector_0 = HKLTransformMatrix * modvector_0;
  modvector_1 = HKLTransformMatrix * modvector_1;
  modvector_2 = HKLTransformMatrix * modvector_2;
  DblMatrix modHKL(3, 3);
  for (size_t i = 0; i < 3; i++) {
    modHKL[i][0] = modvector_0[i];
    modHKL[i][1] = modvector_1[i];
    modHKL[i][2] = modvector_2[i];
  }
  DblMatrix newModUB = newUB * modHKL;
  setProperty("TransformationMatrix", HKLTransformMatrix.getVector());

  // logging
  std::ostringstream msg;
  msg << info.GetDescription() << " Lattice Parameters: " << IndexingUtils::GetLatticeParameterString(newUB) << '\n'
      << "Transformation Matrix =  " << HKLTransformMatrix.str() << '\n';
  g_log.notice(msg.str());

  if (apply) {
    std::vector<double> lattice_constant_errors(6);
    auto UBs_optimized =
        SelectCellWithForm::CalculateUBWithErrors(lattice_constant_errors, newUB, newModUB, ws, tolerance);
    // NOTE: the returned UB is the augmented UB, i.e.
    // UB = [ UB  | modUB ]
    // 3x6   3x3     3x3
    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 3; ++j) {
        newUB[i][j] = UBs_optimized[i][j];
        newModUB[i][j] = UBs_optimized[i][j + 3];
      }
    }
    //----------------------------------------------
    o_lattice->setUB(newUB);
    o_lattice->setModUB(newModUB);
    o_lattice->setError(lattice_constant_errors[0], lattice_constant_errors[1], lattice_constant_errors[2],
                        lattice_constant_errors[3], lattice_constant_errors[4], lattice_constant_errors[5]);

    int n_peaks = ws->getNumberPeaks();

    int num_indexed = 0;
    double average_error = 0.0;

    if (o_lattice->getMaxOrder() == 0) {
      std::vector<V3D> miller_indices;
      std::vector<V3D> q_vectors;
      for (int i = 0; i < n_peaks; i++) {
        q_vectors.emplace_back(ws->getPeak(i).getQSampleFrame());
      }
      num_indexed = IndexingUtils::CalculateMillerIndices(newUB, q_vectors, tolerance, miller_indices, average_error);

      for (int i = 0; i < n_peaks; i++) {
        IPeak &peak = ws->getPeak(i);
        peak.setIntHKL(miller_indices[i]);
        peak.setHKL(miller_indices[i]);
      }
    } else {
      num_indexed = static_cast<int>(num_indexed);
      for (int i = 0; i < n_peaks; i++) {
        IPeak &peak = ws->getPeak(i);
        average_error += (peak.getHKL()).hklError();
        peak.setIntHKL(HKLTransformMatrix * peak.getIntHKL());
        peak.setHKL(HKLTransformMatrix * peak.getHKL());
      }
    }
    ws->mutableSample().setOrientedLattice(std::move(o_lattice));

    // Tell the user what happened.
    std::ostringstream apply_msg;
    apply_msg << "Re-indexed the peaks with the new UB. \n"
              << "Now, " << num_indexed << " are indexed with average error " << average_error << '\n';
    g_log.notice(apply_msg.str());

    // Save output properties
    setProperty("NumIndexed", num_indexed);
    setProperty("AverageError", average_error);
  }
}

} // namespace Mantid::Crystal
