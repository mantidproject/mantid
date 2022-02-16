// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/SelectCellWithForm.h"
#include "MantidAPI/Sample.h"
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Crystal/ScalarUtils.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid::Crystal {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SelectCellWithForm)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

/** Initialize the algorithm's properties.
 */
void SelectCellWithForm::init() {
  this->declareProperty(std::make_unique<WorkspaceProperty<IPeaksWorkspace>>("PeaksWorkspace", "", Direction::InOut),
                        "Input Peaks Workspace");

  auto mustBePositive = std::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(1);

  this->declareProperty(std::make_unique<PropertyWithValue<int>>("FormNumber", 0, mustBePositive, Direction::Input),
                        "Form number for the desired cell");
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

std::map<std::string, std::string> SelectCellWithForm::validateInputs() {
  std::map<std::string, std::string> result;

  // Case 0: invalid peaksworkspace
  IPeaksWorkspace_sptr ws = this->getProperty("PeaksWorkspace");
  if (!ws) {
    result["PeaksWorkspace"] = "Must be a valid PeaksWorkspace";
  }

  // Case 1:
  auto const o_lattice = ws->sample().getOrientedLattice();
  auto const UB = o_lattice.getUB();
  if (!IndexingUtils::CheckUB(UB)) {
    result["PeaksWorkspace"] = "The stored UB is not a valid orientation matrix";
  }

  return result;
}

Kernel::Matrix<double> SelectCellWithForm::DetermineErrors(std::vector<double> &sigabc,
                                                           const Kernel::Matrix<double> &UB,
                                                           const IPeaksWorkspace_sptr &ws, double tolerance) {

  std::vector<V3D> miller_ind;
  std::vector<V3D> q_vectors;
  std::vector<V3D> q_vectors0;
  int npeaks = ws->getNumberPeaks();
  double fit_error;
  miller_ind.reserve(npeaks);
  q_vectors.reserve(npeaks);
  q_vectors0.reserve(npeaks);
  for (int i = 0; i < npeaks; i++)
    q_vectors0.emplace_back(ws->getPeak(i).getQSampleFrame());

  Kernel::Matrix<double> newUB1(3, 3);
  IndexingUtils::GetIndexedPeaks(UB, q_vectors0, tolerance, miller_ind, q_vectors, fit_error);
  IndexingUtils::Optimize_UB(newUB1, miller_ind, q_vectors, sigabc);

  auto nindexed_old = static_cast<int>(q_vectors.size());
  int nindexed_new = IndexingUtils::NumberIndexed(newUB1, q_vectors0, tolerance);
  bool latErrorsValid = true;
  if (nindexed_old < .8 * nindexed_new || .8 * nindexed_old > nindexed_new)
    latErrorsValid = false;
  else {
    double maxDiff = 0;
    double maxEntry = 0;
    for (int row = 0; row < 3; row++)
      for (int col = 0; col < 3; col++) {
        double diff = fabs(UB[row][col] - newUB1[row][col]);
        double V = std::max<double>(fabs(UB[row][col]), fabs(newUB1[row][col]));
        if (diff > maxDiff)
          maxDiff = diff;
        if (V > maxEntry)
          maxEntry = V;
      }
    if (maxEntry == 0 || maxDiff / maxEntry > .1)
      latErrorsValid = false;
  }

  if (!latErrorsValid) {
    std::fill(sigabc.begin(), sigabc.end(), 0.);
    return UB;

  } else

    return newUB1;
}

/**
 * @brief This function takes in UB and modUB along with the corresponding peaksworkspace
 *
 * @param lattice_constant_errors lattice constant errors calculated during UB optimization
 * @param UB starting UB matrix
 * @param modUB start modUB matrix
 * @param ws input peaks workspace
 * @param tolerance tolerance used during peak indexation
 * @return Kernel::Matrix<double>
 */
Kernel::Matrix<double> SelectCellWithForm::CalculateUBWithErrors(std::vector<double> &lattice_constant_errors,
                                                                 const Kernel::Matrix<double> &UB,
                                                                 const Kernel::Matrix<double> &modUB,
                                                                 const IPeaksWorkspace_sptr &ws, double tolerance) {
  const int npeaks = ws->getNumberPeaks();
  std::vector<V3D> q_vectors;
  std::vector<V3D> q_vectors_indexed;
  std::vector<V3D> hkl_vectors;
  std::vector<V3D> mnp_vectors;
  q_vectors.reserve(npeaks);
  q_vectors_indexed.reserve(npeaks);
  hkl_vectors.reserve(npeaks);
  mnp_vectors.reserve(npeaks);

  UBmatrix UB_new(3, 3);
  UBmatrix modUB_new(3, 3);
  std::vector<double> mod_vectors_errors(3);

  for (int i = 0; i < npeaks; ++i) {
    q_vectors.emplace_back(ws->getPeak(i).getQSampleFrame());
  }

  // re-index with new UB
  double fit_error;
  IndexingUtils::GetIndexedPeaks(UB, q_vectors, tolerance, hkl_vectors, q_vectors_indexed, fit_error);

  // re-calculate mode vectors [mnp]
  // NOTE: if modUB in singular (non-invertable), set all mnp to 0;
  //       otherwise, calculate mnp from UB, hkl, and qs_indexed
  const int npeaks_indexed = static_cast<int>(q_vectors_indexed.size());
  auto modUB_inv = UBmatrix(modUB);
  auto det_modUB = modUB_inv.Invert();
  if (det_modUB == 0) {
    for (int i = 0; i < npeaks_indexed; ++i) {
      mnp_vectors.emplace_back(V3D(0, 0, 0));
    }
  } else {
    for (int i = 0; i < npeaks_indexed; ++i) {
      auto q_ideal = UB * q_vectors_indexed[i];
      auto dq = q_vectors[i] - q_ideal;
      auto mnp = modUB_inv * dq;
      mnp_vectors.emplace_back(mnp);
    }
  }

  // get the modulation vectors dimension [1-3]
  int ModDim;
  auto const o_lattice = ws->mutableSample().getOrientedLattice();
  auto const modvec0 = o_lattice.getModVec(0);
  auto const modvec1 = o_lattice.getModVec(1);
  auto const modvec2 = o_lattice.getModVec(2);
  if (modvec2.norm2() > 0.0)
    ModDim = 3;
  else if (modvec1.norm2() > 0.0)
    ModDim = 2;
  else if (modvec0.norm2() > 0.0)
    ModDim = 1;
  else
    ModDim = 0;

  IndexingUtils::Optimize_6dUB(UB_new, modUB_new, hkl_vectors, mnp_vectors, ModDim, q_vectors_indexed,
                               lattice_constant_errors, mod_vectors_errors);

  // Make sure that the newly optimized UB (UB_new) is close to the original UB,
  // otherwise something is wrong and we should cowardly reject the results by
  // - returning the original UB and modUB
  // - set the lattice constants errors to 0 since the calculated ones are most
  //   likely wrong
  // -- make sure both UB are valid orientation matrix
  bool accept_new_UB = true;
  if (IndexingUtils::CheckUB(UB) && IndexingUtils::CheckUB(UB_new)) {
    auto UB_inv = UBmatrix(UB);
    UB_inv.Invert();
    auto UB_new_dot_UB_inv = UB_new * UB_inv;
    if (1 - UB_new_dot_UB_inv.determinant() > 1e-3) {
      accept_new_UB = false;
    }
  } else {
    accept_new_UB = false;
    lattice_constant_errors.assign(6, 0.);
  }

  //
  Kernel::Matrix<double> UB_optimized(3, 6);
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      if (accept_new_UB) {
        // accept the optimized UB
        UB_optimized[i][j] = UB_new[i][j];
        UB_optimized[i][j + 3] = modUB_new[i][j];
      } else {
        // reject the optimized UB and restore the original ones
        UB_optimized[i][j] = UB[i][j];
        UB_optimized[i][j + 3] = modUB[i][j];
      }
    }
  }

  //
  return UB_optimized;
}

/** Execute the algorithm.
 */
void SelectCellWithForm::exec() {
  IPeaksWorkspace_sptr ws = this->getProperty("PeaksWorkspace");

  // copy current lattice
  auto o_lattice = std::make_unique<OrientedLattice>(ws->mutableSample().getOrientedLattice());
  Matrix<double> UB = o_lattice->getUB();

  bool allowPermutations = this->getProperty("AllowPermutations");

  int form_num = this->getProperty("FormNumber");
  bool apply = this->getProperty("Apply");
  double tolerance = this->getProperty("Tolerance");

  ConventionalCell info = ScalarUtils::GetCellForForm(UB, form_num, allowPermutations);

  DblMatrix newUB = info.GetNewUB();

  std::string message = info.GetDescription() + " Lat Par:" + IndexingUtils::GetLatticeParameterString(newUB);

  g_log.notice(std::string(message));

  DblMatrix T = info.GetHKL_Tran();
  g_log.notice() << "Transformation Matrix =  " << T.str() << '\n';
  this->setProperty("TransformationMatrix", T.getVector());

  if (apply) {
    //----------------------------------- Try to optimize(LSQ) to find lattice
    // errors ------------------------
    //                       UB matrix may NOT have been found by unconstrained
    //                       least squares optimization

    //----------------------------------------------
    o_lattice->setUB(newUB);
    std::vector<double> sigabc(6);
    DetermineErrors(sigabc, newUB, ws, tolerance);

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

} // namespace Mantid::Crystal
