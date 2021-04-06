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

namespace Mantid {
namespace Crystal {
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
    std::vector<double> sigabc(6);
    SelectCellWithForm::DetermineErrors(sigabc, newUB, ws, tolerance);
    //----------------------------------------------
    o_lattice->setUB(newUB);

    o_lattice->setError(sigabc[0], sigabc[1], sigabc[2], sigabc[3], sigabc[4], sigabc[5]);

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
        peak.setIntHKL(T * peak.getIntHKL());
        peak.setHKL(T * peak.getHKL());
      }
    }
    ws->mutableSample().setOrientedLattice(std::move(o_lattice));

    // Tell the user what happened.
    g_log.notice() << "Re-indexed the peaks with the new UB. \n";
    g_log.notice() << "Now, " << num_indexed << " are indexed with average error " << average_error << '\n';

    // Save output properties
    this->setProperty("NumIndexed", num_indexed);
    this->setProperty("AverageError", average_error);
  }
}

} // namespace Crystal
} // namespace Mantid
