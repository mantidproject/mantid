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
  auto o_lattice = std::make_unique<OrientedLattice>(ws->mutableSample().getOrientedLattice());
  Matrix<double> UB = o_lattice->getUB();

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
  g_log.notice() << "Transformation Matrix =  " << T.str() << '\n';
  this->setProperty("TransformationMatrix", T.getVector());

  if (apply) {
    std::vector<double> sigabc(7);
    std::vector<double> sigq(3);
    int n_peaks = ws->getNumberPeaks();

    int num_indexed = 0;
    double average_error = 0.0;

    std::vector<V3D> hkl_vectors;
    std::vector<V3D> mnp_vectors;
    std::vector<V3D> q_vectors;

    //----------------------------------------------
    o_lattice->setUB(newUB);

    V3D modVec1 = T * o_lattice->getModVec(0);
    V3D modVec2 = T * o_lattice->getModVec(1);
    V3D modVec3 = T * o_lattice->getModVec(2);

    o_lattice->setModVec1(modVec1);
    o_lattice->setModVec2(modVec2);
    o_lattice->setModVec3(modVec3);

    DblMatrix modHKL(3, 3);
    modHKL.setColumn(0, modVec1);
    modHKL.setColumn(1, modVec2);
    modHKL.setColumn(2, modVec3);
    o_lattice->setModHKL(modHKL);

    DblMatrix newModUB = newUB * modHKL;
    o_lattice->setModUB(newModUB);

    int modDim = IndexingUtils::GetModulationVectors(newUB, newModUB, modVec1, modVec2, modVec3);

    for (int i = 0; i < n_peaks; i++) {
      IPeak &peak = ws->getPeak(i);
      // if (IndexingUtils::ValidIndex(hkl_vectors[i], tolerance)) {
      //   peak.setHKL(hkl_vectors[i]);
      // }
      peak.setHKL(T * peak.getHKL());
      peak.setIntHKL(T * peak.getIntHKL());
      peak.setIntMNP(T * peak.getIntMNP());

      q_vectors.emplace_back(peak.getQSampleFrame());
      hkl_vectors.emplace_back(peak.getIntHKL());
      mnp_vectors.emplace_back(peak.getIntMNP());
    }

    IndexingUtils::Optimize_6dUB(newUB, newModUB, hkl_vectors, mnp_vectors, modDim, q_vectors, sigabc, sigq);
    num_indexed = IndexingUtils::CalculateMillerIndices(newUB, q_vectors, tolerance, hkl_vectors, average_error);

    o_lattice->setError(sigabc[0], sigabc[1], sigabc[2], sigabc[3], sigabc[4], sigabc[5]);

    ws->mutableSample().setOrientedLattice(std::move(o_lattice));

    // Tell the user what happened.
    g_log.notice() << "Re-indexed the peaks with the new UB. \n";
    g_log.notice() << "Now, " << num_indexed << " are indexed with average error " << average_error << '\n';

    // Save output properties
    this->setProperty("NumIndexed", num_indexed);
    this->setProperty("AverageError", average_error);
  }
}

} // namespace Mantid::Crystal
