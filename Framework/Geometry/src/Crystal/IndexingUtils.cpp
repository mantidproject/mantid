// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/NiggliCell.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidKernel/Quat.h"

#include <boost/math/special_functions/round.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <gsl/gsl_errno.h>
#include <gsl/gsl_fft_real.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_sys.h>
#include <gsl/gsl_vector.h>

#include <algorithm>
#include <cmath>

using namespace Mantid::Geometry;
using Mantid::Kernel::DblMatrix;
using Mantid::Kernel::Matrix;
using Mantid::Kernel::Quat;
using Mantid::Kernel::V3D;

namespace {
const constexpr double DEG_TO_RAD = M_PI / 180.;
const constexpr double RAD_TO_DEG = 180. / M_PI;
} // namespace

/**
  STATIC method Find_UB: Calculates the matrix that most nearly indexes
  the specified q_vectors, given the lattice parameters.  The sum of the
  squares of the residual errors is returned.  This method first sorts the
  specified q_vectors in order of increasing magnitude.  It then searches
  through all possible orientations to find an initial UB that indexes the
  lowest magnitude peaks.
     The resolution of the search through possible orientations is specified
  by the degrees_per_step parameter.  Approximately 2-4 degrees_per_step is
  usually adequate.  NOTE: This is an expensive calculation which takes
  approximately 1 second using 2 degrees_per_step.  However, the execution
  time is O(n^3) so decreasing the resolution to 1 degree per step will take
  about 8 seconds, etc.  It should not be necessary to decrease this value
  below 1 degree per step, and users will have to be VERY patient, if it is
  decreased much below 1 degree per step.
    The number of peaks used to obtain an initial indexing is specified by
  the "NumInitial" parameter.  Good values for this are typically around
  10-15, though with accurate peak positions, and good values for the lattice
  paramters, as few as 2 can be used.  Using substantially more than 15 peaks
  initially typically has no benefit and increases execution time.

  @param  UB                  3x3 matrix that will be set to the UB matrix
  @param  q_vectors           std::vector of V3D objects that contains the
                              list of q_vectors that are to be indexed
                              NOTE: There must be at least 2 q_vectors.
  @param  lattice             The orientated lattice with the lattice
                              parameters a,b,c and alpha, beta, gamma. The found
                              UB and errors will be set on this lattice.
  @param  required_tolerance  The maximum allowed deviation of Miller indices
                              from integer values for a peak to be indexed.
  @param  base_index          The sequence number of the peak that should
                              be used as the central peak.  On the first
                              scan for a UB matrix that fits the data,
                              the remaining peaks in the list of q_vectors
                              will be shifted by -base_peak, where base_peak
                              is the q_vector with the specified base index.
                              If fewer than 5 peaks are specified in the
                              q_vectors list, this parameter is ignored.
                              If this parameter is -1, and there are at least
                              four peaks in the q_vector list, then a base
                              index will be calculated internally.  In most
                              cases, it should suffice to set this to -1.
  @param  num_initial         The number of low |Q| peaks that should be
                              used to scan for an initial orientation matrix.
  @param  degrees_per_step    The number of degrees between different
                              orientations used during the initial scan.
  @param  fixAll              Fix the lattice parameters and do not optimise
                              the UB matrix.
  @param  iterations          Number of refinements of UB

  @return  This will return the sum of the squares of the residual errors.

  @throws  std::invalid_argument exception if UB is not a 3X3 matrix,
                                 if there are not at least 2 q vectors,
                                 if num_initial is < 2, or
                                 if the required_tolerance or degrees_per_step
                                 is <= 0.
*/
double IndexingUtils::Find_UB(DblMatrix &UB, const std::vector<V3D> &q_vectors, OrientedLattice &lattice,
                              double required_tolerance, int base_index, size_t num_initial, double degrees_per_step,
                              bool fixAll, int iterations) {
  if (UB.numRows() != 3 || UB.numCols() != 3) {
    throw std::invalid_argument("Find_UB(): UB matrix NULL or not 3X3");
  }

  if (q_vectors.size() < 2) {
    throw std::invalid_argument("Find_UB(): Two or more indexed peaks needed to find UB");
  }

  if (required_tolerance <= 0) {
    throw std::invalid_argument("Find_UB(): required_tolerance must be positive");
  }

  if (num_initial < 2) {
    throw std::invalid_argument("Find_UB(): number of peaks for inital scan must be > 2");
  }

  if (degrees_per_step <= 0) {
    throw std::invalid_argument("Find_UB(): degrees_per_step must be positive");
  }

  // get list of peaks, sorted on |Q|
  std::vector<V3D> sorted_qs;
  sorted_qs.reserve(q_vectors.size());

  if (q_vectors.size() > 5) // shift to be centered on peak (we lose
                            // one peak that way, so require > 5)
  {
    std::vector<V3D> shifted_qs(q_vectors);
    size_t mid_ind = q_vectors.size() / 3;
    // either do an initial sort and use
    // default mid index, or use the index
    // specified by the base_peak parameter
    if (base_index < 0 || base_index >= static_cast<int>(q_vectors.size())) {
      std::sort(shifted_qs.begin(), shifted_qs.end(), V3D::compareMagnitude);
    } else {
      mid_ind = base_index;
    }
    V3D mid_vec(shifted_qs[mid_ind]);

    for (size_t i = 0; i < shifted_qs.size(); i++) {
      if (i != mid_ind) {
        V3D shifted_vec(shifted_qs[i]);
        shifted_vec -= mid_vec;
        sorted_qs.emplace_back(shifted_vec);
      }
    }
  } else {
    std::copy(q_vectors.cbegin(), q_vectors.cend(), std::back_inserter(sorted_qs));
  }

  std::sort(sorted_qs.begin(), sorted_qs.end(), V3D::compareMagnitude);

  if (num_initial > sorted_qs.size())
    num_initial = sorted_qs.size();

  std::vector<V3D> some_qs;
  some_qs.reserve(q_vectors.size());

  for (size_t i = 0; i < num_initial; i++)
    some_qs.emplace_back(sorted_qs[i]);

  ScanFor_UB(UB, some_qs, lattice, degrees_per_step, required_tolerance);

  double fit_error = 0;
  std::vector<V3D> miller_ind;
  std::vector<V3D> indexed_qs;
  miller_ind.reserve(q_vectors.size());
  indexed_qs.reserve(q_vectors.size());

  // now gradually bring in the remaining
  // peaks and re-optimize the UB to index
  // them as well
  while (!fixAll && num_initial < sorted_qs.size()) {
    num_initial = std::lround(1.5 * static_cast<double>(num_initial + 3));
    // add 3, in case we started with
    // a very small number of peaks!
    if (num_initial >= sorted_qs.size())
      num_initial = sorted_qs.size();

    for (size_t i = some_qs.size(); i < num_initial; i++)
      some_qs.emplace_back(sorted_qs[i]);
    for (int counter = 0; counter < iterations; counter++) {
      try {
        GetIndexedPeaks(UB, some_qs, required_tolerance, miller_ind, indexed_qs, fit_error);
        Matrix<double> temp_UB(3, 3, false);
        fit_error = Optimize_UB(temp_UB, miller_ind, indexed_qs);
        UB = temp_UB;
      } catch (...) {
        // failed to fit using these peaks, so add some more and try again
      }
    }
  }

  std::vector<double> sigabc(7);
  if (!fixAll && q_vectors.size() >= 3) // try one last refinement using all peaks
  {
    for (int counter = 0; counter < iterations; counter++) {
      try {
        GetIndexedPeaks(UB, q_vectors, required_tolerance, miller_ind, indexed_qs, fit_error);
        Matrix<double> temp_UB = UB;
        fit_error = Optimize_UB(temp_UB, miller_ind, indexed_qs, sigabc);
        UB = temp_UB;
      } catch (...) {
        // failed to improve UB using these peaks, so just return the current UB
      }
    }
  }
  // Regardless of how we got the UB, find the
  // sum-squared errors for the indexing in
  // HKL space.
  GetIndexedPeaks(UB, q_vectors, required_tolerance, miller_ind, indexed_qs, fit_error);
  // set the error on the lattice parameters
  lattice.setUB(UB);
  lattice.setError(sigabc[0], sigabc[1], sigabc[2], sigabc[3], sigabc[4], sigabc[5]);
  return fit_error;
}

/**
    STATIC method Find_UB: This method will attempt to calculate the matrix
  that most nearly indexes the specified q_vectors, given only a range of
  possible unit cell edge lengths.  If successful, the matrix should
  correspond to the Niggli reduced cell.
     The resolution of the search through possible orientations is specified
  by the degrees_per_step parameter.  Approximately 1-3 degrees_per_step is
  usually adequate.  NOTE: This is an expensive calculation which takes
  approximately 1 second using 1 degree_per_step.  However, the execution
  time is O(n^3) so decreasing the resolution to 0.5 degree per step will take
  about 8 seconds, etc.  It should not be necessary to decrease this value
  below 1 degree per step, and users will have to be VERY patient, if it is
  decreased much below 1 degree per step.
    The number of peaks used to obtain an initial indexing is specified by
  the "NumInitial" parameter.  Good values for this are typically around
  15-25.  The specified q_vectors must correspond to a single crystal.  If
  several crystallites are present or there are other sources of "noise"
  leading to invalid peaks, this method will not work well.  The method that
  uses lattice parameters may be better in such cases.  Alternatively, adjust
  the list of specified q_vectors so it does not include noise peaks or peaks
  from more than one crystal, by increasing the threshold for what counts
  as a peak, or by other methods.

  @param  UB                  3x3 matrix that will be set to the UB matrix
  @param  q_vectors           std::vector of V3D objects that contains the
                              list of q_vectors that are to be indexed
                              NOTE: There must be at least 3 q_vectors.
  @param  min_d               Lower bound on shortest unit cell edge length.
                              This does not have to be specified exactly but
                              must be strictly less than the smallest edge
                              length, in Angstroms.
  @param  max_d               Upper bound on longest unit cell edge length.
                              This does not have to be specified exactly but
                              must be strictly more than the longest edge
                              length in angstroms.
  @param  required_tolerance  The maximum allowed deviation of Miller indices
                              from integer values for a peak to be indexed.
  @param  base_index          The sequence number of the peak that should
                              be used as the central peak.  On the first
                              scan for a UB matrix that fits the data,
                              the remaining peaks in the list of q_vectors
                              will be shifted by -base_peak, where base_peak
                              is the q_vector with the specified base index.
                              If fewer than 6 peaks are specified in the
                              q_vectors list, this parameter is ignored.
                              If this parameter is -1, and there are at least
                              five peaks in the q_vector list, then a base
                              index will be calculated internally.  In most
                              cases, it should suffice to set this to -1.
  @param  num_initial         The number of low |Q| peaks that should be
                              used to scan for an initial orientation matrix.
  @param  degrees_per_step    The number of degrees between different
                              orientations used during the initial scan.

  @return  This will return the sum of the squares of the residual errors.

  @throws  std::invalid_argument exception if UB is not a 3X3 matrix,
                                 if there are not at least 3 q vectors,
                                 if min_d >= max_d or min_d <= 0
                                 if num_initial is < 3, or
                                 if the required_tolerance or degrees_per_step
                                 is <= 0.
*/
double IndexingUtils::Find_UB(DblMatrix &UB, const std::vector<V3D> &q_vectors, double min_d, double max_d,
                              double required_tolerance, int base_index, size_t num_initial, double degrees_per_step) {
  if (UB.numRows() != 3 || UB.numCols() != 3) {
    throw std::invalid_argument("Find_UB(): UB matrix NULL or not 3X3");
  }

  if (q_vectors.size() < 3) {
    throw std::invalid_argument("Find_UB(): Three or more indexed peaks needed to find UB");
  }

  if (min_d >= max_d || min_d <= 0) {
    throw std::invalid_argument("Find_UB(): Need 0 < min_d < max_d");
  }

  if (required_tolerance <= 0) {
    throw std::invalid_argument("Find_UB(): required_tolerance must be positive");
  }

  if (num_initial < 3) {
    throw std::invalid_argument("Find_UB(): number of peaks for inital scan must be > 2");
  }

  if (degrees_per_step <= 0) {
    throw std::invalid_argument("Find_UB(): degrees_per_step must be positive");
  }

  // get list of peaks, sorted on |Q|
  std::vector<V3D> sorted_qs;
  sorted_qs.reserve(q_vectors.size());

  if (q_vectors.size() > 5) // shift to be centered on peak (we lose
                            // one peak that way, so require > 5)
  {
    std::vector<V3D> shifted_qs(q_vectors);
    size_t mid_ind = q_vectors.size() / 2;
    // either do an initial sort and use
    // default mid index, or use the index
    // specified by the base_peak parameter
    if (base_index < 0 || base_index >= static_cast<int>(q_vectors.size())) {
      std::sort(shifted_qs.begin(), shifted_qs.end(), V3D::compareMagnitude);
    } else {
      mid_ind = base_index;
    }
    V3D mid_vec(shifted_qs[mid_ind]);

    for (size_t i = 0; i < shifted_qs.size(); i++) {
      if (i != mid_ind) {
        V3D shifted_vec(shifted_qs[i]);
        shifted_vec -= mid_vec;
        sorted_qs.emplace_back(shifted_vec);
      }
    }
  } else {
    std::copy(q_vectors.cbegin(), q_vectors.cend(), std::back_inserter(sorted_qs));
  }

  std::sort(sorted_qs.begin(), sorted_qs.end(), V3D::compareMagnitude);

  if (num_initial > sorted_qs.size())
    num_initial = sorted_qs.size();

  std::vector<V3D> some_qs;
  some_qs.reserve(q_vectors.size());

  for (size_t i = 0; i < num_initial; i++)
    some_qs.emplace_back(sorted_qs[i]);
  std::vector<V3D> directions;
  ScanFor_Directions(directions, some_qs, min_d, max_d, required_tolerance, degrees_per_step);

  if (directions.size() < 3) {
    throw std::runtime_error("Find_UB(): Could not find at least three possible lattice directions");
  }

  std::sort(directions.begin(), directions.end(), V3D::compareMagnitude);

  if (!FormUB_From_abc_Vectors(UB, directions, 0, min_d, max_d)) {
    throw std::runtime_error("Find_UB(): Could not find independent a, b, c directions");
  }
  // now gradually bring in the remaining
  // peaks and re-optimize the UB to index
  // them as well
  std::vector<V3D> miller_ind;
  std::vector<V3D> indexed_qs;
  miller_ind.reserve(q_vectors.size());
  indexed_qs.reserve(q_vectors.size());

  Matrix<double> temp_UB(3, 3, false);
  double fit_error = 0;
  while (num_initial < sorted_qs.size()) {
    num_initial = std::lround(1.5 * static_cast<double>(num_initial + 3));
    // add 3, in case we started with
    // a very small number of peaks!
    if (num_initial >= sorted_qs.size())
      num_initial = sorted_qs.size();

    for (size_t i = some_qs.size(); i < num_initial; i++)
      some_qs.emplace_back(sorted_qs[i]);

    GetIndexedPeaks(UB, some_qs, required_tolerance, miller_ind, indexed_qs, fit_error);
    try {
      fit_error = Optimize_UB(temp_UB, miller_ind, indexed_qs);
      UB = temp_UB;
    } catch (...) {
      // failed to improve with these peaks, so continue with more peaks
      // if possible
    }
  }

  if (q_vectors.size() >= 5) // try one last refinement using all peaks
  {
    GetIndexedPeaks(UB, q_vectors, required_tolerance, miller_ind, indexed_qs, fit_error);
    try {
      fit_error = Optimize_UB(temp_UB, miller_ind, indexed_qs);
      UB = temp_UB;
    } catch (...) {
      // failed to improve with all peaks, so return the UB we had
    }
  }

  if (NiggliCell::MakeNiggliUB(UB, temp_UB))
    UB = temp_UB;

  return fit_error;
}

/**
    STATIC method Find_UB: This method will attempt to calculate the matrix
  that most nearly indexes the specified q_vectors, using FFTs to find
  patterns in projected Q-vectors, given only a range of possible unit cell
  edge lengths. If successful, the resulting matrix should correspond to
  the Niggli reduced cell.
    The resolution of the search through possible orientations is specified
  by the degrees_per_step parameter.  One to two degrees per step is usually
  adequate.
  NOTE: The execution time is O(n^3) where n is the number of degrees per
  step, so decreasing the resolution to 0.5 degree per step will take
  about 8 times longer than using 1 degree per step.  It should not be
  necessary to decrease this value below 1 degree per step, and users will
  have to be VERY patient, if it is decreased much below 1 degree per step.
    The specified q_vectors should correspond to a single crystal, for this
  to work reliably.

  @param  UB                  3x3 matrix that will be set to the UB matrix
  @param  q_vectors           std::vector of V3D objects that contains the
                              list of q_vectors that are to be indexed
                              NOTE: There must be at least 4 q_vectors and it
                              really should have at least 10 or more peaks
                              for this to work quite consistently.
  @param  min_d               Lower bound on shortest unit cell edge length.
                              This does not have to be specified exactly but
                              must be strictly less than the smallest edge
                              length, in Angstroms.
  @param  max_d               Upper bound on longest unit cell edge length.
                              This does not have to be specified exactly but
                              must be strictly more than the longest edge
                              length in angstroms.
  @param  required_tolerance  The maximum allowed deviation of Miller indices
                              from integer values for a peak to be indexed.
  @param  degrees_per_step    The number of degrees between different
                              orientations used during the initial scan.
  @param  iterations          Number of refinements of UB

  @return  This will return the sum of the squares of the residual errors.

  @throws  std::invalid_argument exception if UB is not a 3X3 matrix,
                                 if there are not at least 3 q vectors,
                                 if min_d >= max_d or min_d <= 0,
                                 if the required_tolerance or degrees_per_step
                                 is <= 0,
                                 if at least three possible a,b,c directions
                                 were not found,
                                 or if a valid UB matrix could not be formed
                                 from the a,b,c directions that were found.
*/
double IndexingUtils::Find_UB(DblMatrix &UB, const std::vector<V3D> &q_vectors, double min_d, double max_d,
                              double required_tolerance, double degrees_per_step, int iterations) {
  if (UB.numRows() != 3 || UB.numCols() != 3) {
    throw std::invalid_argument("Find_UB(): UB matrix NULL or not 3X3");
  }

  if (q_vectors.size() < 4) {
    throw std::invalid_argument("Find_UB(): Four or more indexed peaks needed to find UB");
  }

  if (min_d >= max_d || min_d <= 0) {
    throw std::invalid_argument("Find_UB(): Need 0 < min_d < max_d");
  }

  if (required_tolerance <= 0) {
    throw std::invalid_argument("Find_UB(): required_tolerance must be positive");
  }

  if (degrees_per_step <= 0) {
    throw std::invalid_argument("Find_UB(): degrees_per_step must be positive");
  }

  std::vector<V3D> directions;

  // NOTE: we use a somewhat higher tolerance when
  // finding individual directions since it is easier
  // to index one direction individually compared to
  // indexing three directions simultaneously.
  size_t max_indexed =
      FFTScanFor_Directions(directions, q_vectors, min_d, max_d, 0.75f * required_tolerance, degrees_per_step);

  if (max_indexed == 0) {
    throw std::invalid_argument("Find_UB(): Could not find any a,b,c vectors to index Qs");
  }

  if (directions.size() < 3) {
    throw std::invalid_argument("Find_UB(): Could not find enough a,b,c vectors");
  }

  std::sort(directions.begin(), directions.end(), V3D::compareMagnitude);

  double min_vol = min_d * min_d * min_d / 4.0;

  if (!FormUB_From_abc_Vectors(UB, directions, q_vectors, required_tolerance, min_vol)) {
    throw std::invalid_argument("Find_UB(): Could not form UB matrix from a,b,c vectors");
  }

  Matrix<double> temp_UB(3, 3, false);
  double fit_error = 0;
  if (q_vectors.size() >= 5) // repeatedly refine UB
  {
    std::vector<V3D> miller_ind;
    std::vector<V3D> indexed_qs;
    miller_ind.reserve(q_vectors.size());
    indexed_qs.reserve(q_vectors.size());

    GetIndexedPeaks(UB, q_vectors, required_tolerance, miller_ind, indexed_qs, fit_error);

    for (int counter = 0; counter < iterations; counter++) {
      try {
        fit_error = Optimize_UB(temp_UB, miller_ind, indexed_qs);
        UB = temp_UB;
        GetIndexedPeaks(UB, q_vectors, required_tolerance, miller_ind, indexed_qs, fit_error);
      } catch (std::exception &) {
        // failed to improve with all peaks, so just keep the UB we had
      }
    }
  }

  if (NiggliCell::MakeNiggliUB(UB, temp_UB))
    UB = temp_UB;

  return fit_error;
}

/**
  STATIC method Optimize_UB: Calculates the matrix that most nearly maps
  the specified hkl_vectors to the specified q_vectors.  The calculated
  UB minimizes the sum squared differences between UB*(h,k,l) and the
  corresponding (qx,qy,qz) for all of the specified hkl and Q vectors.
  The sum of the squares of the residual errors is returned.  This method is
  used to optimize the UB matrix once an initial indexing has been found.

  @param  UB           3x3 matrix that will be set to the UB matrix
  @param  hkl_vectors  std::vector of V3D objects that contains the
                       list of hkl values
  @param  q_vectors    std::vector of V3D objects that contains the list of
                       q_vectors that are indexed by the corresponding hkl
                       vectors.
  @param  sigabc      error in the crystal lattice parameter values if length
                      is at least 6. NOTE: Calculation of these errors is based
  on
                      SCD FORTRAN code base at IPNS. Contributors to the least
                      squares application(1979) are J.Marc Overhage, G.Anderson,
                      P. C. W. Leung, R. G. Teller, and  A. J. Schultz
  NOTE: The number of hkl_vectors and q_vectors must be the same, and must
        be at least 3.

  @return  This will return the sum of the squares of the residual differences
           between the Q vectors provided and the UB*hkl values, in
           reciprocal space.

  @throws  std::invalid_argument exception if there are not at least 3
                                 hkl and q vectors, or if the numbers of
                                 hkl and q vectors are not the same, or if
                                 the UB matrix is not a 3x3 matrix.

  @throws  std::runtime_error    exception if the QR factorization fails or
                                 the UB matrix can't be calculated or if
                                 UB is a singular matrix.
*/
double IndexingUtils::Optimize_UB(DblMatrix &UB, const std::vector<V3D> &hkl_vectors, const std::vector<V3D> &q_vectors,
                                  std::vector<double> &sigabc) {
  double result = 0;
  result = Optimize_UB(UB, hkl_vectors, q_vectors);

  if (sigabc.size() < 6) {
    sigabc.clear();
    return result;
  } else
    for (int i = 0; i < 6; i++)
      sigabc[i] = 0.0;

  size_t nDOF = 3 * (hkl_vectors.size() - 3);
  DblMatrix HKLTHKL(3, 3);
  for (int r = 0; r < 3; r++)
    for (int c = 0; c < 3; c++)
      for (const auto &hkl_vector : hkl_vectors) {
        HKLTHKL[r][c] += hkl_vector[r] * hkl_vector[c]; // rounded??? to nearest integer
      }

  HKLTHKL.Invert();

  double SMALL = 1.525878906E-5;
  Matrix<double> derivs(3, 7);
  std::vector<double> latOrig, latNew;
  GetLatticeParameters(UB, latOrig);

  for (int r = 0; r < 3; r++) {
    for (int c = 0; c < 3; c++) {

      UB[r][c] += SMALL;
      GetLatticeParameters(UB, latNew);
      UB[r][c] -= SMALL;

      for (size_t l = 0; l < 7; l++)
        derivs[c][l] = (latNew[l] - latOrig[l]) / SMALL;
    }

    for (size_t l = 0; l < std::min<size_t>(static_cast<size_t>(7), sigabc.size()); l++)
      for (int m = 0; m < 3; m++)
        for (int n = 0; n < 3; n++)
          sigabc[l] += (derivs[m][l] * HKLTHKL[m][n] * derivs[n][l]);
  }

  double delta = result / static_cast<double>(nDOF);

  for (size_t i = 0; i < std::min<size_t>(7, sigabc.size()); i++)
    sigabc[i] = sqrt(delta * sigabc[i]);

  return result;
}

/**
  STATIC method Optimize_UB: Calculates the matrix that most nearly maps
  the specified hkl_vectors to the specified q_vectors.  The calculated
  UB minimizes the sum squared differences between UB*(h,k,l) and the
  corresponding (qx,qy,qz) for all of the specified hkl and Q vectors.
  The sum of the squares of the residual errors is returned.  This method is
  used to optimize the UB matrix once an initial indexing has been found.

  @param  UB           3x3 matrix that will be set to the UB matrix
  @param  hkl_vectors  std::vector of V3D objects that contains the
                       list of hkl values
  @param  q_vectors    std::vector of V3D objects that contains the list of
                       q_vectors that are indexed by the corresponding hkl
                       vectors.
  NOTE: The number of hkl_vectors and q_vectors must be the same, and must
        be at least 3.

  @return  This will return the sum of the squares of the residual differences
           between the Q vectors provided and the UB*hkl values, in
           reciprocal space.

  @throws  std::invalid_argument exception if there are not at least 3
                                 hkl and q vectors, or if the numbers of
                                 hkl and q vectors are not the same, or if
                                 the UB matrix is not a 3x3 matrix.

  @throws  std::runtime_error    exception if the QR factorization fails or
                                 the UB matrix can't be calculated or if
                                 UB is a singular matrix.
*/
double IndexingUtils::Optimize_UB(DblMatrix &UB, const std::vector<V3D> &hkl_vectors,
                                  const std::vector<V3D> &q_vectors) {
  if (UB.numRows() != 3 || UB.numCols() != 3) {
    throw std::invalid_argument("Optimize_UB(): UB matrix NULL or not 3X3");
  }

  if (hkl_vectors.size() < 3) {
    throw std::invalid_argument("Optimize_UB(): Three or more indexed peaks needed to find UB");
  }

  if (hkl_vectors.size() != q_vectors.size()) {
    throw std::invalid_argument("Optimize_UB(): Number of hkl_vectors != number of q_vectors");
  }

  gsl_matrix *H_transpose = gsl_matrix_alloc(hkl_vectors.size(), 3);
  gsl_vector *tau = gsl_vector_alloc(3);

  double sum_sq_error = 0;
  // Make the H-transpose matrix from the
  // hkl vectors and form QR factorization
  for (size_t row = 0; row < hkl_vectors.size(); row++) {
    for (size_t col = 0; col < 3; col++) {
      gsl_matrix_set(H_transpose, row, col, (hkl_vectors[row])[col]);
    }
  }

  int returned_flag = gsl_linalg_QR_decomp(H_transpose, tau);

  if (returned_flag != 0) {
    gsl_matrix_free(H_transpose);
    gsl_vector_free(tau);
    throw std::runtime_error("Optimize_UB(): gsl QR_decomp failed, invalid hkl values");
  }
  // solve for each row of UB, using the
  // QR factorization of and accumulate the
  // sum of the squares of the residuals
  gsl_vector *UB_row = gsl_vector_alloc(3);
  gsl_vector *q = gsl_vector_alloc(q_vectors.size());
  gsl_vector *residual = gsl_vector_alloc(q_vectors.size());

  bool found_UB = true;

  for (size_t row = 0; row < 3; row++) {
    for (size_t i = 0; i < q_vectors.size(); i++) {
      gsl_vector_set(q, i, (q_vectors[i])[row] / (2.0 * M_PI));
    }

    returned_flag = gsl_linalg_QR_lssolve(H_transpose, tau, q, UB_row, residual);
    if (returned_flag != 0) {
      found_UB = false;
    }

    for (size_t i = 0; i < 3; i++) {
      double value = gsl_vector_get(UB_row, i);
      if (!std::isfinite(value))
        found_UB = false;
    }

    V3D row_values(gsl_vector_get(UB_row, 0), gsl_vector_get(UB_row, 1), gsl_vector_get(UB_row, 2));
    UB.setRow(row, row_values);

    for (size_t i = 0; i < q_vectors.size(); i++) {
      sum_sq_error += gsl_vector_get(residual, i) * gsl_vector_get(residual, i);
    }
  }

  gsl_matrix_free(H_transpose);
  gsl_vector_free(tau);
  gsl_vector_free(UB_row);
  gsl_vector_free(q);
  gsl_vector_free(residual);

  if (!found_UB) {
    throw std::runtime_error("Optimize_UB(): Failed to find UB, invalid hkl or Q values");
  }

  if (!CheckUB(UB)) {
    throw std::runtime_error("Optimize_UB(): The optimized UB is not valid");
  }

  return sum_sq_error;
}

/**
 STATIC method Optimize_UB: Calculates the matrix that most nearly maps
 the specified hkl_vectors to the specified q_vectors.  The calculated
 UB minimizes the sum squared differences between UB*(h,k,l) and the
 corresponding (qx,qy,qz) for all of the specified hkl and Q vectors.
 The sum of the squares of the residual errors is returned.  This method is
 used to optimize the UB matrix once an initial indexing has been found.

 @param  UB           3x3 matrix that will be set to the UB matrix
 @param  ModUB        3x3 matrix that will be set to the ModUB matrix
 @param  hkl_vectors  std::vector of V3D objects that contains the
 list of hkl values
 @param  mnp_vectors  std::vector of V3D objects that contains the
 @param  ModDim       int value from 1 to 3, defines the number of dimensions
 of modulation.
 @param  q_vectors    std::vector of V3D objects that contains the list of
 q_vectors that are indexed by the corresponding hkl
 vectors.
 @param  sigabc       error in the crystal lattice parameter values if length
 is at least 6. NOTE: Calculation of these errors is based
 on
 SCD FORTRAN code base at IPNS. Contributors to the least
 squares application(1979) are J.Marc Overhage, G.Anderson,
 P. C. W. Leung, R. G. Teller, and  A. J. Schultz
 NOTE: The number of hkl_vectors and q_vectors must be the same, and must
 be at least 3.
 @param  sigq         error in the modulation vectors.


 @return  This will return the sum of the squares of the residual differences
 between the Q vectors provided and the UB*hkl values, in
 reciprocal space.

 @throws  std::invalid_argument exception if there are not at least 3
 hkl and q vectors, or if the numbers of
 hkl and q vectors are not the same, or if
 the UB matrix is not a 3x3 matrix.

 @throws  std::runtime_error    exception if the QR factorization fails or
 the UB matrix can't be calculated or if
 UB is a singular matrix.
 */

double IndexingUtils::Optimize_6dUB(DblMatrix &UB, DblMatrix &ModUB, const std::vector<V3D> &hkl_vectors,
                                    const std::vector<V3D> &mnp_vectors, const int &ModDim,
                                    const std::vector<V3D> &q_vectors, std::vector<double> &sigabc,
                                    std::vector<double> &sigq) {

  if (ModDim != 0 && ModDim != 1 && ModDim != 2 && ModDim != 3)
    throw std::invalid_argument("invalid Value for Modulation Dimension");

  double result = 0;
  result = Optimize_6dUB(UB, ModUB, hkl_vectors, mnp_vectors, ModDim, q_vectors);

  if (sigabc.size() < static_cast<size_t>(6)) {
    sigabc.clear();
    return result;
  } else
    for (int i = 0; i < 6; i++)
      sigabc[i] = 0.0;

  size_t nDOF = 3 * (hkl_vectors.size() - 3);
  DblMatrix HKLTHKL(3, 3);
  for (int r = 0; r < 3; r++)
    for (int c = 0; c < 3; c++)
      for (const auto &hkl_vector : hkl_vectors) {
        HKLTHKL[r][c] += hkl_vector[r] * hkl_vector[c];
      }

  HKLTHKL.Invert();

  double SMALL = 1.525878906E-5;
  Matrix<double> derivs(3, 7);
  std::vector<double> latOrig, latNew;
  GetLatticeParameters(UB, latOrig);

  for (int r = 0; r < 3; r++) {
    for (int c = 0; c < 3; c++) {

      UB[r][c] += SMALL;
      GetLatticeParameters(UB, latNew);
      UB[r][c] -= SMALL;

      for (size_t l = 0; l < 7; l++)
        derivs[c][l] = (latNew[l] - latOrig[l]) / SMALL;
    }

    for (size_t l = 0; l < std::min<size_t>(static_cast<size_t>(7), sigabc.size()); l++)
      for (int m = 0; m < 3; m++)
        for (int n = 0; n < 3; n++)
          sigabc[l] += (derivs[m][l] * HKLTHKL[m][n] * derivs[n][l]);
  }

  double delta = result / static_cast<double>(nDOF);

  for (size_t i = 0; i < std::min<size_t>(7, sigabc.size()); i++)
    sigabc[i] = sqrt(delta * sigabc[i]);

  DblMatrix UBinv = UB;
  UBinv.Invert();

  if (sigq.size() < static_cast<size_t>(3)) {
    sigq.clear();
    return result;
  }

  else {
    sigq[0] = sqrt(delta) * latOrig[0];
    sigq[1] = sqrt(delta) * latOrig[1];
    sigq[2] = sqrt(delta) * latOrig[2];
  }

  return delta;
}

/**
 STATIC method Optimize_6dUB: Calculates the 6-dimensional matrix that most
 nearly maps the specified hkl_vectors and mnp_vectors to the specified
 q_vectors.  The calculated UB minimizes the sum squared differences between
 UB|ModUB*(h,k,l,m,n,p) and the corresponding (qx,qy,qz) for all of the
 specified hklmnp and Q vectors. The sum of the squares of the residual errors
 is returned.  This method is used to optimize the UB matrix and ModUB matrix
 once an initial indexing has been found.

 ModUB matrix is a 3x3 defines the modulation vectors in Q-sample. each colomn
 corresponds to a modulation vector.


 @param  UB           3x3 matrix that will be set to the UB matrix
 @param  ModUB        3x3 matrix that will be set to the ModUB matrix
 @param  hkl_vectors  std::vector of V3D objects that contains the
 list of hkl values
 @param  mnp_vectors  std::vector of V3D objects that contains the
 list of mnp values
 @param  ModDim       int value from 1 to 3, defines the number of dimensions
 of modulation.
 @param  q_vectors    std::vector of V3D objects that contains the list of
 q_vectors that are indexed by the corresponding hkl
 vectors.
 NOTE: The number of hkl_vectors and mnp_vectors and q_vectors must be the same,
 and must be at least 4.

 @return  This will return the sum of the squares of the residual differences
 between the Q vectors provided and the UB*hkl values, in
 reciprocal space.

 @throws  std::invalid_argument exception if there are not at least 3
 hkl and q vectors, or if the numbers of
 hkl and q vectors are not the same, or if
 the UB matrix is not a 3x3 matrix.
 or if the ModDim is not 1 or 2 or 3.

 @throws  std::runtime_error    exception if the QR factorization fails or
 the UB matrix can't be calculated or if
 UB is a singular matrix.

 Created by Shiyun Jin on 7/16/18.
 */

double IndexingUtils::Optimize_6dUB(DblMatrix &UB, DblMatrix &ModUB, const std::vector<V3D> &hkl_vectors,
                                    const std::vector<V3D> &mnp_vectors, const int &ModDim,
                                    const std::vector<V3D> &q_vectors) {

  if (UB.numRows() != 3 || UB.numCols() != 3) {
    throw std::invalid_argument("Optimize_6dUB(): UB matrix NULL or not 3X3");
  }

  if (ModUB.numRows() != 3 || ModUB.numCols() != 3) {
    throw std::invalid_argument("Optimize_6dUB(): ModUB matrix NULL or not 3X3");
  }

  if (hkl_vectors.size() < 3) {
    throw std::invalid_argument("Optimize_6dUB(): Three or more indexed peaks needed to find UB");
  }

  if (hkl_vectors.size() != q_vectors.size() || hkl_vectors.size() != mnp_vectors.size()) {
    throw std::invalid_argument("Optimize_6dUB(): Number of hkl_vectors "
                                "doesn't match number of mnp_vectors or number "
                                "of q_vectors");
  }

  if (ModDim != 0 && ModDim != 1 && ModDim != 2 && ModDim != 3)
    throw std::invalid_argument("invalid Value for Modulation Dimension");

  gsl_matrix *H_transpose = gsl_matrix_alloc(hkl_vectors.size(), ModDim + 3);
  gsl_vector *tau = gsl_vector_alloc(ModDim + 3);

  double sum_sq_error = 0;
  // Make the H-transpose matrix from the
  // hkl vectors and form QR factorization
  for (size_t row = 0; row < hkl_vectors.size(); row++) {
    for (size_t col = 0; col < 3; col++)
      gsl_matrix_set(H_transpose, row, col, (hkl_vectors[row])[col]);
    for (size_t col = 0; col < static_cast<size_t>(ModDim); col++)
      gsl_matrix_set(H_transpose, row, col + 3, (mnp_vectors[row])[col]);
  }

  int returned_flag = gsl_linalg_QR_decomp(H_transpose, tau);

  if (returned_flag != 0) {
    gsl_matrix_free(H_transpose);
    gsl_vector_free(tau);
    throw std::runtime_error("Optimize_UB(): gsl QR_decomp failed, invalid hkl and mnp values");
  }
  // solve for each row of UB, using the
  // QR factorization of and accumulate the
  // sum of the squares of the residuals
  gsl_vector *UB_row = gsl_vector_alloc(ModDim + 3);
  gsl_vector *q = gsl_vector_alloc(q_vectors.size());
  gsl_vector *residual = gsl_vector_alloc(q_vectors.size());

  bool found_UB = true;

  for (size_t row = 0; row < 3; row++) {
    for (size_t i = 0; i < q_vectors.size(); i++) {
      gsl_vector_set(q, i, (q_vectors[i])[row] / (2.0 * M_PI));
    }

    returned_flag = gsl_linalg_QR_lssolve(H_transpose, tau, q, UB_row, residual);

    if (returned_flag != 0) {
      found_UB = false;
    }

    for (size_t i = 0; i < static_cast<size_t>(ModDim + 3); i++) {
      double value = gsl_vector_get(UB_row, i);
      if (!std::isfinite(value))
        found_UB = false;
    }

    V3D hklrow_values(gsl_vector_get(UB_row, 0), gsl_vector_get(UB_row, 1), gsl_vector_get(UB_row, 2));
    V3D mnprow_values(0, 0, 0);

    if (ModDim == 1)
      mnprow_values(gsl_vector_get(UB_row, 3), 0, 0);
    if (ModDim == 2)
      mnprow_values(gsl_vector_get(UB_row, 3), gsl_vector_get(UB_row, 4), 0);
    if (ModDim == 3)
      mnprow_values(gsl_vector_get(UB_row, 3), gsl_vector_get(UB_row, 4), gsl_vector_get(UB_row, 5));

    UB.setRow(row, hklrow_values);
    ModUB.setRow(row, mnprow_values);

    for (size_t i = 0; i < q_vectors.size(); i++) {
      sum_sq_error += gsl_vector_get(residual, i) * gsl_vector_get(residual, i);
    }
  }

  gsl_matrix_free(H_transpose);
  gsl_vector_free(tau);
  gsl_vector_free(UB_row);
  gsl_vector_free(q);
  gsl_vector_free(residual);

  if (!found_UB) {
    throw std::runtime_error("Optimize_UB(): Failed to find UB, invalid hkl or Q values");
  }

  if (!CheckUB(UB)) {
    throw std::runtime_error("Optimize_UB(): The optimized UB is not valid");
  }

  return sum_sq_error;
}

/**
  STATIC method Optimize_Direction: Calculates the vector for which the
  dot product of the vector with each of the specified Qxyz vectors
  is most nearly the corresponding integer index.  The calculated best_vec
  minimizes the sum squared differences between best_vec dot (qx,qy,z)
  and the corresponding index for all of the specified Q vectors and
  indices.  The sum of the squares of the residual errors is returned.
  NOTE: This method is similar the Optimize_UB method, but this method only
        optimizes the plane normal in one direction.  Also, this optimizes
        the mapping from (qx,qy,qz) to one index (Q to index), while the
        Optimize_UB method optimizes the mapping from three (h,k,l) to
        (qx,qy,qz) (3 indices to Q).

  @param  best_vec     V3D vector that will be set to a vector whose
                       direction most nearly corresponds to the plane
                       normal direction and whose magnitude is d.  The
                       corresponding plane spacing in reciprocal space
                       is 1/d.
  @param  index_values std::vector of ints that contains the list of indices
  @param  q_vectors    std::vector of V3D objects that contains the list of
                       q_vectors that are indexed in one direction by the
                       corresponding index values.
  NOTE: The number of index_values and q_vectors must be the same, and must
        be at least 3.

  @return  This will return the sum of the squares of the residual errors.

  @throws  std::invalid_argument exception if there are not at least 3
                                 indices and q vectors, or if the numbers of
                                 indices and q vectors are not the same.

  @throws  std::runtime_error    exception if the QR factorization fails or
                                 the best direction can't be calculated.
*/

double IndexingUtils::Optimize_Direction(V3D &best_vec, const std::vector<int> &index_values,
                                         const std::vector<V3D> &q_vectors) {
  if (index_values.size() < 3) {
    throw std::invalid_argument("Optimize_Direction(): Three or more indexed values needed");
  }

  if (index_values.size() != q_vectors.size()) {
    throw std::invalid_argument("Optimize_Direction(): Number of index_values != number of q_vectors");
  }

  gsl_matrix *H_transpose = gsl_matrix_alloc(q_vectors.size(), 3);
  gsl_vector *tau = gsl_vector_alloc(3);

  double sum_sq_error = 0;
  // Make the H-transpose matrix from the
  // q vectors and form QR factorization

  for (size_t row = 0; row < q_vectors.size(); row++) {
    for (size_t col = 0; col < 3; col++) {
      gsl_matrix_set(H_transpose, row, col, (q_vectors[row])[col] / (2.0 * M_PI));
    }
  }
  int returned_flag = gsl_linalg_QR_decomp(H_transpose, tau);

  if (returned_flag != 0) {
    gsl_matrix_free(H_transpose);
    gsl_vector_free(tau);
    throw std::runtime_error("Optimize_Direction(): gsl QR_decomp failed, invalid hkl values");
  }
  // solve for the best_vec, using the
  // QR factorization and accumulate the
  // sum of the squares of the residuals
  gsl_vector *x = gsl_vector_alloc(3);
  gsl_vector *indices = gsl_vector_alloc(index_values.size());
  gsl_vector *residual = gsl_vector_alloc(index_values.size());

  bool found_best_vec = true;

  for (size_t i = 0; i < index_values.size(); i++) {
    gsl_vector_set(indices, i, index_values[i]);
  }

  returned_flag = gsl_linalg_QR_lssolve(H_transpose, tau, indices, x, residual);
  if (returned_flag != 0) {
    found_best_vec = false;
  }

  for (size_t i = 0; i < 3; i++) {
    double value = gsl_vector_get(x, i);
    if (!std::isfinite(value))
      found_best_vec = false;
  }

  best_vec(gsl_vector_get(x, 0), gsl_vector_get(x, 1), gsl_vector_get(x, 2));

  for (size_t i = 0; i < index_values.size(); i++) {
    sum_sq_error += gsl_vector_get(residual, i) * gsl_vector_get(residual, i);
  }

  gsl_matrix_free(H_transpose);
  gsl_vector_free(tau);
  gsl_vector_free(x);
  gsl_vector_free(indices);
  gsl_vector_free(residual);

  if (!found_best_vec) {
    throw std::runtime_error("Optimize_Direction(): Failed to find best_vec, "
                             "invalid indexes or Q values");
  }

  return sum_sq_error;
}

/**
    The method uses two passes to scan across all possible directions and
    orientations to find the direction and orientation for the unit cell
    that best fits the specified list of peaks.
    On the first pass, only those sets of directions that index the
    most peaks are kept.  On the second pass, the directions that minimize
    the sum-squared deviations from integer indices are selected from that
    smaller set of directions.  This method should be most useful if number
    of peaks is on the order of 10-20, and most of the peaks belong to the
    same crystallite.
    @param UB                 This will be set to the UB matrix that best
                              indexes the supplied list of q_vectors.
    @param q_vectors          List of locations of peaks in "Q".
    @param cell               Unit cell defining the parameters a,b,c and
                              alpha, beta, gamma.
    @param degrees_per_step   The number of degrees per step used when
                              scanning through all possible directions and
                              orientations for the unit cell. NOTE: The
                              work required rises very rapidly as the number
                              of degrees per step decreases. A value of 1
                              degree leads to about 10 seconds of compute time.
                              while a value of 2 only requires a bit more than
                              1 sec.  The required time is O(n^3) where
                              n = 1/degrees_per_step.

    @param required_tolerance The maximum distance from an integer that the
                              calculated h,k,l values can have if a peak
                              is to be considered indexed.

  @throws std::invalid_argument exception if the UB matrix is not a 3X3 matrix.
  @throws std::runtime_error exception if the matrix inversion fails and UB
                             can't be formed

 */
double IndexingUtils::ScanFor_UB(DblMatrix &UB, const std::vector<V3D> &q_vectors, const UnitCell &cell,
                                 double degrees_per_step, double required_tolerance) {
  if (UB.numRows() != 3 || UB.numCols() != 3) {
    throw std::invalid_argument("Find_UB(): UB matrix NULL or not 3X3");
  }

  auto a = cell.a();
  auto b = cell.b();
  auto c = cell.c();

  // Precompute required trigonometric functions
  const auto cosAlpha = std::cos(cell.alpha() * DEG_TO_RAD);
  const auto cosBeta = std::cos(cell.beta() * DEG_TO_RAD);
  const auto cosGamma = std::cos(cell.gamma() * DEG_TO_RAD);
  const auto sinGamma = std::sin(cell.gamma() * DEG_TO_RAD);
  const auto gamma_degrees = cell.gamma();

  V3D a_dir;
  V3D b_dir;
  V3D c_dir;

  double num_a_steps = std::round(90.0 / degrees_per_step);
  int num_b_steps = boost::math::iround(4.0 * sinGamma * num_a_steps);

  std::vector<V3D> a_dir_list = MakeHemisphereDirections(boost::numeric_cast<int>(num_a_steps));

  V3D a_dir_temp;
  V3D b_dir_temp;
  V3D c_dir_temp;

  double error;
  double dot_prod;
  double nearest_int;
  int max_indexed = 0;
  V3D q_vec;
  // first select those directions
  // that index the most peaks
  std::vector<V3D> selected_a_dirs;
  std::vector<V3D> selected_b_dirs;
  std::vector<V3D> selected_c_dirs;

  for (const auto &a_dir_num : a_dir_list) {
    a_dir_temp = a_dir_num;
    a_dir_temp = V3D(a_dir_temp);
    a_dir_temp *= a;

    std::vector<V3D> b_dir_list =
        MakeCircleDirections(boost::numeric_cast<int>(num_b_steps), a_dir_temp, gamma_degrees);

    for (const auto &b_dir_num : b_dir_list) {
      b_dir_temp = b_dir_num;
      b_dir_temp = V3D(b_dir_temp);
      b_dir_temp *= b;
      c_dir_temp = makeCDir(a_dir_temp, b_dir_temp, c, cosAlpha, cosBeta, cosGamma, sinGamma);
      int num_indexed = 0;
      for (const auto &q_vector : q_vectors) {
        bool indexes_peak = true;
        q_vec = q_vector / (2.0 * M_PI);
        dot_prod = a_dir_temp.scalar_prod(q_vec);
        nearest_int = std::round(dot_prod);
        error = fabs(dot_prod - nearest_int);
        if (error > required_tolerance)
          indexes_peak = false;
        else {
          dot_prod = b_dir_temp.scalar_prod(q_vec);
          nearest_int = std::round(dot_prod);
          error = fabs(dot_prod - nearest_int);
          if (error > required_tolerance)
            indexes_peak = false;
          else {
            dot_prod = c_dir_temp.scalar_prod(q_vec);
            nearest_int = std::round(dot_prod);
            error = fabs(dot_prod - nearest_int);
            if (error > required_tolerance)
              indexes_peak = false;
          }
        }
        if (indexes_peak)
          num_indexed++;
      }

      if (num_indexed > max_indexed) // only keep those directions that
      {                              // index the max number of peaks
        selected_a_dirs.clear();
        selected_b_dirs.clear();
        selected_c_dirs.clear();
        max_indexed = num_indexed;
      }
      if (num_indexed == max_indexed) {
        selected_a_dirs.emplace_back(a_dir_temp);
        selected_b_dirs.emplace_back(b_dir_temp);
        selected_c_dirs.emplace_back(c_dir_temp);
      }
    }
  }
  // now, for each such direction, find
  // the one that indexes closes to
  // integer values
  double min_error = 1.0e50;
  for (size_t dir_num = 0; dir_num < selected_a_dirs.size(); dir_num++) {
    a_dir_temp = selected_a_dirs[dir_num];
    b_dir_temp = selected_b_dirs[dir_num];
    c_dir_temp = selected_c_dirs[dir_num];

    double sum_sq_error = 0.0;
    for (const auto &q_vector : q_vectors) {
      q_vec = q_vector / (2.0 * M_PI);
      dot_prod = a_dir_temp.scalar_prod(q_vec);
      nearest_int = std::round(dot_prod);
      error = dot_prod - nearest_int;
      sum_sq_error += error * error;

      dot_prod = b_dir_temp.scalar_prod(q_vec);
      nearest_int = std::round(dot_prod);
      error = dot_prod - nearest_int;
      sum_sq_error += error * error;

      dot_prod = c_dir_temp.scalar_prod(q_vec);
      nearest_int = std::round(dot_prod);
      error = dot_prod - nearest_int;
      sum_sq_error += error * error;
    }

    if (sum_sq_error < min_error) {
      min_error = sum_sq_error;
      a_dir = a_dir_temp;
      b_dir = b_dir_temp;
      c_dir = c_dir_temp;
    }
  }

  if (!OrientedLattice::GetUB(UB, a_dir, b_dir, c_dir)) {
    throw std::runtime_error("UB could not be formed, invert matrix failed");
  }

  return min_error;
}

/**
   Get list of possible edge vectors for the real space unit cell.  This list
   will consist of vectors, V, for which V dot Q is essentially an integer for
   the most Q vectors.  The difference between V dot Q and an integer must be
   less than the required tolerance for it to count as an integer.
    @param  directions          Vector that will be filled with the directions
                                that may correspond to unit cell edges.
    @param  q_vectors           Vector of new Vector3D objects that contains
                                the list of q_vectors that are to be indexed.
    @param  min_d               Lower bound on shortest unit cell edge length.
                                This does not have to be specified exactly but
                                must be strictly less than the smallest edge
                                length, in Angstroms.
    @param  max_d               Upper bound on longest unit cell edge length.
                                This does not have to be specified exactly but
                                must be strictly more than the longest edge
                                length in angstroms.
    @param  required_tolerance  The maximum allowed deviation of Miller indices
                                from integer values for a peak to be indexed.
    @param  degrees_per_step    The number of degrees between directions that
                                are checked while scanning for an initial
                                indexing of the peaks with lowest |Q|.
 */

size_t IndexingUtils::ScanFor_Directions(std::vector<V3D> &directions, const std::vector<V3D> &q_vectors, double min_d,
                                         double max_d, double required_tolerance, double degrees_per_step) {
  double error;
  double fit_error;
  double dot_prod;
  double nearest_int;
  int max_indexed = 0;
  V3D q_vec;
  // first, make hemisphere of possible directions
  // with specified resolution.
  int num_steps = boost::math::iround(90.0 / degrees_per_step);
  std::vector<V3D> full_list = MakeHemisphereDirections(num_steps);
  // Now, look for possible real-space unit cell edges
  // by checking for vectors with length between
  // min_d and max_d that would index the most peaks,
  // in some direction, keeping the shortest vector
  // for each direction where the max peaks are indexed
  double delta_d = 0.1f;
  int n_steps = boost::math::iround(1.0 + (max_d - min_d) / delta_d);

  std::vector<V3D> selected_dirs;

  for (const auto &current_dir : full_list) {
    for (int step = 0; step <= n_steps; step++) {
      V3D dir_temp = current_dir;
      dir_temp *= (min_d + step * delta_d); // increasing size

      int num_indexed = 0;
      for (const auto &q_vector : q_vectors) {
        q_vec = q_vector / (2.0 * M_PI);
        dot_prod = dir_temp.scalar_prod(q_vec);
        nearest_int = std::round(dot_prod);
        error = fabs(dot_prod - nearest_int);
        if (error <= required_tolerance)
          num_indexed++;
      }

      if (num_indexed > max_indexed) // only keep those directions that
      {                              // index the max number of peaks
        selected_dirs.clear();
        max_indexed = num_indexed;
      }
      if (num_indexed >= max_indexed) {
        selected_dirs.emplace_back(dir_temp);
      }
    }
  }
  // Now, optimize each direction and discard possible
  // unit cell edges that are duplicates, putting the
  // new smaller list in the vector "directions"
  std::vector<int> index_vals;
  std::vector<V3D> indexed_qs;
  index_vals.reserve(q_vectors.size());
  indexed_qs.reserve(q_vectors.size());

  directions.clear();
  for (const auto &selected_dir : selected_dirs) {
    V3D current_dir = selected_dir;

    GetIndexedPeaks_1D(current_dir, q_vectors, required_tolerance, index_vals, indexed_qs, fit_error);

    Optimize_Direction(current_dir, index_vals, indexed_qs);

    double length = current_dir.norm();
    if (length >= min_d && length <= max_d) // only keep if within range
    {
      bool duplicate = false;
      for (const auto &direction : directions) {
        V3D diff = current_dir - direction;
        // discard same direction
        if (diff.norm() < 0.001) {
          duplicate = true;
        } else {
          diff = current_dir + direction;
          // discard opposite direction
          if (diff.norm() < 0.001) {
            duplicate = true;
          }
        }
      }
      if (!duplicate) {
        directions.emplace_back(current_dir);
      }
    }
  }
  return max_indexed;
}

/**
   Get list of possible edge vectors for the real space unit cell.  This
   method uses FFTs to find directions for which projections of the peaks
   on those directions have repetitive patterns.  This list of directions found
   will consist of vectors, V, for which V dot Q is essentially an integer for
   the most Q vectors.  The difference between V dot Q and an integer must be
   less than the required tolerance for it to count as an integer.
    @param  directions          Vector that will be filled with the directions
                                that may correspond to unit cell edges.
    @param  q_vectors           Vector of new Vector3D objects that contains
                                the list of q_vectors that are to be indexed.
    @param  min_d               Lower bound on shortest unit cell edge length.
                                This does not have to be specified exactly but
                                must be strictly less than the smallest edge
                                length, in Angstroms.
    @param  max_d               Upper bound on longest unit cell edge length.
                                This does not have to be specified exactly but
                                must be strictly more than the longest edge
                                length in angstroms.
    @param  required_tolerance  The maximum allowed deviation of Miller indices
                                from integer values for a peak to be indexed.
    @param  degrees_per_step    The number of degrees between directions that
                                are checked while scanning for an initial
                                indexing of the peaks with lowest |Q|.
 */

size_t IndexingUtils::FFTScanFor_Directions(std::vector<V3D> &directions, const std::vector<V3D> &q_vectors,
                                            double min_d, double max_d, double required_tolerance,
                                            double degrees_per_step) {
  constexpr size_t N_FFT_STEPS = 512;
  constexpr size_t HALF_FFT_STEPS = 256;

  double fit_error;
  int max_indexed = 0;

  // first, make hemisphere of possible directions
  // with specified resolution.
  int num_steps = boost::math::iround(90.0 / degrees_per_step);
  std::vector<V3D> full_list = MakeHemisphereDirections(num_steps);

  // find the maximum magnitude of Q to set range
  // needed for FFT
  double max_mag_Q = 0;
  for (size_t q_num = 1; q_num < q_vectors.size(); q_num++) {
    double mag_Q = q_vectors[q_num].norm() / (2.0 * M_PI);
    if (mag_Q > max_mag_Q)
      max_mag_Q = mag_Q;
  }

  max_mag_Q *= 1.1f; // allow for a little "headroom" for FFT range

  // apply the FFT to each of the directions, and
  // keep track of their maximum magnitude past DC
  double max_mag_fft;
  std::vector<double> max_fft_val;
  max_fft_val.resize(full_list.size());

  double projections[N_FFT_STEPS];
  double magnitude_fft[HALF_FFT_STEPS];

  double index_factor = N_FFT_STEPS / max_mag_Q; // maps |proj Q| to index

  for (size_t dir_num = 0; dir_num < full_list.size(); dir_num++) {
    V3D current_dir = full_list[dir_num];
    max_mag_fft = GetMagFFT(q_vectors, current_dir, N_FFT_STEPS, projections, index_factor, magnitude_fft);

    max_fft_val[dir_num] = max_mag_fft;
  }
  // find the directions with the 500 largest
  // fft values, and place them in temp_dirs vector
  int N_TO_TRY = 500;

  std::vector<double> max_fft_copy;
  max_fft_copy.resize(full_list.size());
  for (size_t i = 0; i < max_fft_copy.size(); i++) {
    max_fft_copy[i] = max_fft_val[i];
  }

  std::sort(max_fft_copy.begin(), max_fft_copy.end());

  size_t index = max_fft_copy.size() - 1;
  max_mag_fft = max_fft_copy[index];

  double threshold = max_mag_fft;
  while ((index > max_fft_copy.size() - N_TO_TRY) && threshold >= max_mag_fft / 2) {
    index--;
    threshold = max_fft_copy[index];
  }

  std::vector<V3D> temp_dirs;
  for (size_t i = 0; i < max_fft_val.size(); i++) {
    if (max_fft_val[i] >= threshold) {
      temp_dirs.emplace_back(full_list[i]);
    }
  }
  // now scan through temp_dirs and use the
  // FFT to find the cell edge length that
  // corresponds to the max_mag_fft.  Only keep
  // directions with length nearly in bounds
  V3D temp;
  std::vector<V3D> temp_dirs_2;

  for (const auto &temp_dir : temp_dirs) {
    GetMagFFT(q_vectors, temp_dir, N_FFT_STEPS, projections, index_factor, magnitude_fft);

    double position = GetFirstMaxIndex(magnitude_fft, HALF_FFT_STEPS, threshold);
    if (position > 0) {
      double q_val = max_mag_Q / position;
      double d_val = 1 / q_val;
      if (d_val >= 0.8 * min_d && d_val <= 1.2 * max_d) {
        temp = temp_dir * d_val;
        temp_dirs_2.emplace_back(temp);
      }
    }
  }
  // look at how many peaks were indexed
  // for each of the initial directions
  max_indexed = 0;
  int num_indexed;
  V3D current_dir;
  for (const auto &dir_num : temp_dirs_2) {
    num_indexed = NumberIndexed_1D(dir_num, q_vectors, required_tolerance);
    if (num_indexed > max_indexed)
      max_indexed = num_indexed;
  }

  // only keep original directions that index
  // at least 50% of max num indexed
  temp_dirs.clear();
  for (const auto &dir_num : temp_dirs_2) {
    current_dir = dir_num;
    num_indexed = NumberIndexed_1D(current_dir, q_vectors, required_tolerance);
    if (num_indexed >= 0.50 * max_indexed)
      temp_dirs.emplace_back(current_dir);
  }
  // refine directions and again find the
  // max number indexed, for the optimized
  // directions
  max_indexed = 0;
  std::vector<int> index_vals;
  std::vector<V3D> indexed_qs;
  for (auto &temp_dir : temp_dirs) {
    num_indexed = GetIndexedPeaks_1D(temp_dir, q_vectors, required_tolerance, index_vals, indexed_qs, fit_error);
    try {
      int count = 0;
      while (count < 5) // 5 iterations should be enough for
      {                 // the optimization to stabilize
        Optimize_Direction(temp_dir, index_vals, indexed_qs);

        num_indexed = GetIndexedPeaks_1D(temp_dir, q_vectors, required_tolerance, index_vals, indexed_qs, fit_error);
        if (num_indexed > max_indexed)
          max_indexed = num_indexed;

        count++;
      }
    } catch (...) {
      // don't continue to refine if the direction fails to optimize properly
    }
  }
  // discard those with length out of bounds
  temp_dirs_2.clear();
  for (const auto &temp_dir : temp_dirs) {
    current_dir = temp_dir;
    double length = current_dir.norm();
    if (length >= 0.8 * min_d && length <= 1.2 * max_d)
      temp_dirs_2.emplace_back(current_dir);
  }
  // only keep directions that index at
  // least 75% of the max number of peaks
  temp_dirs.clear();
  for (const auto &dir_num : temp_dirs_2) {
    current_dir = dir_num;
    num_indexed = NumberIndexed_1D(current_dir, q_vectors, required_tolerance);
    if (num_indexed > max_indexed * 0.75)
      temp_dirs.emplace_back(current_dir);
  }

  std::sort(temp_dirs.begin(), temp_dirs.end(), V3D::compareMagnitude);

  // discard duplicates:
  double len_tol = 0.1; // 10% tolerance for lengths
  double ang_tol = 5.0; // 5 degree tolerance for angles
  DiscardDuplicates(directions, temp_dirs, q_vectors, required_tolerance, len_tol, ang_tol);

  return max_indexed;
}

/**
    Fill an array with the magnitude of the FFT of the
    projections of the specified q_vectors on the specified direction.
    The largest value in the magnitude FFT that occurs at index 5 or more
    is returned as the value of the function.

    @param q_vectors     The list of Q vectors to project on the specified
                         direction.
    @param current_dir   The direction the Q vectors will be projected on.
    @param N             The size of the projections[] array.  This MUST BE
                         a power of 2.  The magnitude_fft[] array must be
                         half the size.
    @param projections   Array to hold the projections of the Q vectors.  This
                         must be long enough so that all projected values map
                         map to a valid index, after they are multiplied by the
                         index_factor.
    @param index_factor  Factor that when multiplied by a projected Q vector
                         will give a valid index into the projections array.
    @param magnitude_fft Array that will be filled out with the magnitude of
                         the FFT of the projections.
    @return The largest value in the magnitude_fft, that is stored in position
            5 or more.
 */
double IndexingUtils::GetMagFFT(const std::vector<V3D> &q_vectors, const V3D &current_dir, const size_t N,
                                double projections[], double index_factor, double magnitude_fft[]) {
  for (size_t i = 0; i < N; i++) {
    projections[i] = 0.0;
  }
  // project onto direction
  V3D q_vec;
  for (const auto &q_vector : q_vectors) {
    q_vec = q_vector / (2.0 * M_PI);
    double dot_prod = current_dir.scalar_prod(q_vec);
    auto index = static_cast<size_t>(fabs(index_factor * dot_prod));
    if (index < N)
      projections[index] += 1;
    else
      projections[N - 1] += 1; // This should not happen, but trap it in
  } // case of rounding errors.

  // get the |FFT|
  gsl_fft_real_radix2_transform(projections, 1, N);
  for (size_t i = 1; i < N / 2; i++) {
    magnitude_fft[i] = sqrt(projections[i] * projections[i] + projections[N - i] * projections[N - i]);
  }

  magnitude_fft[0] = fabs(projections[0]);

  size_t dc_end = 5; // we may need a better estimate of this
  double max_mag_fft = 0.0;
  for (size_t i = dc_end; i < N / 2; i++)
    if (magnitude_fft[i] > max_mag_fft)
      max_mag_fft = magnitude_fft[i];

  return max_mag_fft;
}

/**
   Scan the FFT array for the first maximum that exceeds
   the specified threshold and is beyond the initial DC term/interval.
   @param magnitude_fft   The array containing the magnitude of the
                          FFT values.
   @param N               The size of the FFT array.
   @param threshold       The required threshold for the first peak.  This
                          must be positive.
   @return The centroid (index) where the first maximum occurs, or -1
           if no point in the FFT (beyond the DC term) equals or exceeds
           the required threshold.
 */
double IndexingUtils::GetFirstMaxIndex(const double magnitude_fft[], size_t N, double threshold) {
  // find first local min below threshold
  size_t i = 2;
  bool found_min = false;
  double val;
  while (i < N - 1 && !found_min) {
    val = magnitude_fft[i];
    if (val < threshold && val <= magnitude_fft[i - 1] && val <= magnitude_fft[i + 1])
      found_min = true;
    i++;
  }

  if (!found_min)
    return -1;
  // find next local max above threshold
  bool found_max = false;
  while (i < N - 1 && !found_max) {
    val = magnitude_fft[i];
    if (val >= threshold && val >= magnitude_fft[i - 1] && val >= magnitude_fft[i + 1])
      found_max = true;
    else
      i++;
  }

  if (found_max) {
    double sum = 0;
    double w_sum = 0;
    for (size_t j = i - 2; j < std::min(N, i + 3); j++) {
      sum += static_cast<double>(j) * magnitude_fft[j];
      w_sum += magnitude_fft[j];
    }
    return sum / w_sum;
  } else
    return -1;
}

/**
    Form a UB matrix from the given list of possible directions, using the
    direction at the specified index for the "a" direction.  The "b" and "c"
    directions are chosen so that
     1) |a| < |b| < |c|,
     2) the angle between the a, b, c, vectors is at least a minimum
        angle based on min_d/max_d
     3) c is not in the same plane as a and b.

    @param UB           The calculated UB matrix will be returned in this
                        parameter
    @param directions   List of possible vectors for a, b, c.  This list MUST
                        be sorted in order of increasing magnitude.
    @param a_index      The index to use for the a vector.  The b and c
                        vectors will be choosen from LATER positions in the
                        directions list.
    @param min_d        Minimum possible real space unit cell edge length.
    @param max_d        Maximum possible real space unit cell edge length.

    @return true if a UB matrix was set, and false if it not possible to
            choose a,b,c (i.e. UB) from the list of directions, starting
            with the specified a_index.

  @throws std::invalid_argument exception if the UB matrix is not a 3X3 matrix.
  @throws std::runtime_error exception if the matrix inversion fails and UB
                             can't be formed
 */
bool IndexingUtils::FormUB_From_abc_Vectors(DblMatrix &UB, const std::vector<V3D> &directions, size_t a_index,
                                            double min_d, double max_d) {
  if (UB.numRows() != 3 || UB.numCols() != 3) {
    throw std::invalid_argument("Find_UB(): UB matrix NULL or not 3X3");
  }

  size_t index = a_index;
  V3D a_dir = directions[index];
  index++;
  // the possible range of d-values
  // implies a bound on the minimum
  // angle between a,b, c vectors.
  double min_deg = (RAD_TO_DEG)*atan(2.0 * std::min(0.2, min_d / max_d));

  double epsilon = 5; //  tolerance on right angle (degrees)
  V3D b_dir;
  bool b_found = false;
  while (!b_found && index < directions.size()) {
    V3D vec = directions[index];
    double gamma = a_dir.angle(vec) * RAD_TO_DEG;

    if (gamma >= min_deg && (180. - gamma) >= min_deg) {
      b_dir = vec;
      if (gamma > 90 + epsilon) // try for Nigli cell with angles <= 90
        b_dir *= -1.0;
      b_found = true;
      index++;
    } else
      index++;
  }

  if (!b_found)
    return false;

  V3D c_dir;
  bool c_found = false;

  const V3D perp = normalize(a_dir.cross_prod(b_dir));
  double perp_ang;
  double alpha;
  double beta;
  while (!c_found && index < directions.size()) {
    V3D vec = directions[index];
    int factor = 1;
    while (!c_found && factor >= -1) // try c in + or - direction
    {
      c_dir = vec;
      c_dir *= factor;
      perp_ang = perp.angle(c_dir) * RAD_TO_DEG;
      alpha = b_dir.angle(c_dir) * RAD_TO_DEG;
      beta = a_dir.angle(c_dir) * RAD_TO_DEG;
      // keep a,b,c right handed by choosing
      // c in general directiion of a X b
      if (perp_ang < 90. - epsilon && alpha >= min_deg && (180. - alpha) >= min_deg && beta >= min_deg &&
          (180. - beta) >= min_deg) {
        c_found = true;
      }
      factor -= 2;
    }
    if (!c_found)
      index++;
  }

  if (!c_found) {
    return false;
  }
  // now build the UB matrix from a,b,c
  if (!OrientedLattice::GetUB(UB, a_dir, b_dir, c_dir)) {
    throw std::runtime_error("UB could not be formed, invert matrix failed");
  }

  return true;
}

/**
    Form a UB matrix from the given list of possible directions, using the
    three directions that correspond to a unit cell with the smallest volume
    (greater than or equal to the specified minimum volume) that indexes at
    least 80% of the maximum number of peaks indexed by any set of three
    distinct vectors chosen from the list.

    @param UB            The calculated UB matrix will be returned in this
                         parameter
    @param directions    List of possible vectors for a, b, c.  This list MUST
                         be sorted in order of increasing magnitude.
    @param q_vectors     The list of q_vectors that should be indexed
    @param req_tolerance The required tolerance on h,k,l to consider a peak
                         to be indexed.
    @param min_vol       The smallest possible unit cell volume.

    @return true if a UB matrix was set, and false if it not possible to
            choose a,b,c (i.e. UB) from the list of directions, starting
            with the specified a_index.

    @throws std::invalid_argument exception if the UB matrix is not a 3X3
   matrix.
    @throws std::runtime_error exception if the matrix inversion fails and UB
                               can't be formed

 */
bool IndexingUtils::FormUB_From_abc_Vectors(DblMatrix &UB, const std::vector<V3D> &directions,
                                            const std::vector<V3D> &q_vectors, double req_tolerance, double min_vol) {
  if (UB.numRows() != 3 || UB.numCols() != 3) {
    throw std::invalid_argument("Find_UB(): UB matrix NULL or not 3X3");
  }

  int max_indexed = 0;
  V3D a_dir(0, 0, 0);
  V3D b_dir(0, 0, 0);
  V3D c_dir(0, 0, 0);
  V3D a_temp;
  V3D b_temp;
  V3D c_temp;
  V3D acrossb;
  double vol;

  for (size_t i = 0; i < directions.size() - 2; i++) {
    a_temp = directions[i];
    for (size_t j = i + 1; j < directions.size() - 1; j++) {
      b_temp = directions[j];
      acrossb = a_temp.cross_prod(b_temp);
      for (size_t k = j + 1; k < directions.size(); k++) {
        c_temp = directions[k];
        vol = fabs(acrossb.scalar_prod(c_temp));
        if (vol > min_vol) {
          const auto num_indexed = NumberIndexed_3D(a_temp, b_temp, c_temp, q_vectors, req_tolerance);

          // Requiring 20% more indexed with longer edge lengths, favors
          // the smaller unit cells.
          if (num_indexed > 1.20 * max_indexed) {
            max_indexed = num_indexed;
            a_dir = a_temp;
            b_dir = b_temp;
            c_dir = c_temp;
          }
        }
      }
    }
  }

  if (max_indexed <= 0) {
    return false;
  }
  // force a,b,c to be right handed
  acrossb = a_dir.cross_prod(b_dir);
  if (acrossb.scalar_prod(c_dir) < 0) {
    c_dir = c_dir * (-1.0);
  }
  // now build the UB matrix from a,b,c
  if (!OrientedLattice::GetUB(UB, a_dir, b_dir, c_dir)) {
    throw std::runtime_error("UB could not be formed, invert matrix failed");
  }

  return true;
}

/**
    For a rotated unit cell, calculate the vector in the direction of edge
    "c" given two vectors a_dir and b_dir in the directions of edges "a"
    and "b", with lengths a and b, and the cell angles.  The calculation is
    explained in the Mantid document UBMatriximplementationnotes.pdf, pg 3,
    Andre Savici.
    @param  a_dir   V3D object with length "a" in the direction of the rotated
                    cell edge "a"
    @param  b_dir   V3D object with length "b" in the direction of the rotated
                    cell edge "b"
    @param  c       The length of the third cell edge, c.
    @param  cosAlpha   cos angle between edges b and c in radians.
    @param  cosBeta    cos angle between edges c and a in radians.
    @param  cosGamma   cos angle between edges a and b in radians.
    @param  sinGamma   sin angle between edges a and b in radians.

    @return A new V3D object with length "c", in the direction of the third
            rotated unit cell edge, "c".
 */
V3D IndexingUtils::makeCDir(const V3D &a_dir, const V3D &b_dir, const double c, const double cosAlpha,
                            const double cosBeta, const double cosGamma, const double sinGamma) {

  double c1 = c * cosBeta;
  double c2 = c * (cosAlpha - cosGamma * cosBeta) / sinGamma;
  double V =
      sqrt(1 - cosAlpha * cosAlpha - cosBeta * cosBeta - cosGamma * cosGamma + 2 * cosAlpha * cosBeta * cosGamma);
  double c3 = c * V / sinGamma;

  auto basis_1 = Mantid::Kernel::toVector3d(a_dir).normalized();
  auto basis_3 = Mantid::Kernel::toVector3d(a_dir).cross(Mantid::Kernel::toVector3d(b_dir)).normalized();
  auto basis_2 = basis_3.cross(basis_1).normalized();

  return Mantid::Kernel::toV3D(basis_1 * c1 + basis_2 * c2 + basis_3 * c3);
}

/**
    Construct a sublist of the specified list of a,b,c directions, by removing
   all directions that seem to be duplicates.  If several directions all have
   the same length (within the specified length tolerance) and have the
   same direction (within the specified angle tolerange) then only one of
   those directions will be recorded in the sublist.  The one that indexes
   the most peaks, within the specified tolerance will be kept.

   @param  new_list            This vector will be cleared and filled with the
                               vectors from the directions list that are not
                               duplicates of other vectors in the list.
   @param  directions          Input list of possible a,b,c directions, listed
                               in order of increasing vector norm.  This
                               list will be cleared by this method.
   @param  q_vectors           List of q_vectors that should be indexed
   @param  required_tolerance  The tolerance for indexing
   @param  len_tol             The tolerance on the relative difference in
                               length for two directions to be considered
                               equal.  Eg. if relative differences must be
                               less than 5% for two lengths to be considered
                               the same, pass in .05 for the len_tol.
   @param  ang_tol             The tolerance for the difference in directions,
                               specified in degrees.

 */
void IndexingUtils::DiscardDuplicates(std::vector<V3D> &new_list, std::vector<V3D> &directions,
                                      const std::vector<V3D> &q_vectors, double required_tolerance, double len_tol,
                                      double ang_tol) {
  new_list.clear();
  std::vector<V3D> temp;

  V3D current_dir;
  V3D next_dir;
  V3D zero_vec(0, 0, 0);

  double next_length;
  double length_diff;
  double angle;
  size_t dir_num = 0;
  size_t check_index;
  bool new_dir;

  while (dir_num < directions.size()) // put sequence of similar vectors
  {                                   // in list temp
    current_dir = directions[dir_num];
    double current_length = current_dir.norm();
    dir_num++;

    if (current_length > 0) // skip any zero vectors
    {
      temp.clear();
      temp.emplace_back(current_dir);
      check_index = dir_num;
      new_dir = false;
      while (check_index < directions.size() && !new_dir) {
        next_dir = directions[check_index];
        next_length = next_dir.norm();
        if (next_length > 0) {
          length_diff = fabs(next_dir.norm() - current_length);
          if ((length_diff / current_length) < len_tol) // continue scan
          {
            angle = current_dir.angle(next_dir) * RAD_TO_DEG;
            if ((std::isnan)(angle))
              angle = 0;
            if ((angle < ang_tol) || (angle > (180.0 - ang_tol))) {
              temp.emplace_back(next_dir);
              directions[check_index] = zero_vec; // mark off this direction
            } // since it was duplicate

            check_index++; // keep checking all vectors with
          } // essentially the same length
          else
            new_dir = true; // we only know we have a new
                            // direction if the length is
                            // different, since list is
        } // sorted by length !
        else
          check_index++; // just move on
      }
      // now scan through temp list to
      int max_indexed = 0; // find the one that indexes most

      long max_i = -1;
      for (size_t i = 0; i < temp.size(); i++) {
        int num_indexed = NumberIndexed_1D(temp[i], q_vectors, required_tolerance);
        if (num_indexed > max_indexed) {
          max_indexed = num_indexed;
          max_i = static_cast<long>(i);
        }
      }

      if (max_indexed > 0) // don't bother to add any direction
      {                    // that doesn't index anything
        new_list.emplace_back(temp[max_i]);
      }
    }
  }

  directions.clear();
}

/**
  Round all of the components of the V3D to the nearest integer.
  @param hkl V3D object whose components will be rounded.
*/
void IndexingUtils::RoundHKL(Mantid::Kernel::V3D &hkl) {
  for (size_t i = 0; i < 3; i++) {
    hkl[i] = std::round(hkl[i]);
  }
}

/**
  Round all of the components of all vectors to the nearest integer.  This
  is useful when the vectors in the list represent Miller indices.  Since
  the PeaksWorkspace records the Miller indices as sets of three doubles,
  there is no guarantee that the Miller indices will be integers.

  @param hkl_list   Vector of V3D objects whose components will be rounded.

 */
void IndexingUtils::RoundHKLs(std::vector<V3D> &hkl_list) {
  for (auto &entry : hkl_list) {
    RoundHKL(entry);
  }
}

/**
 * @param value The value to check.
 * @param tolerance Allowable distance from integer.
 * @return True if the value is within tolerance of a whole number
 */
inline bool withinTol(const double value, const double tolerance) {
  double myVal = fabs(value);
  if (myVal - floor(myVal) < tolerance)
    return true;
  if (floor(myVal + 1.) - myVal < tolerance)
    return true;
  return false;
}

/**
  Check whether or not the components of the specified vector are within
  the specified tolerance of integer values, other than (0,0,0).

  @param hkl        A V3D object containing what may be valid Miller indices
                    for a peak.
  @param tolerance  The maximum acceptable deviation from integer values for
                    the Miller indices.

  @return true if all components of the vector are within the tolerance of
               integer values (h,k,l) and (h,k,l) is NOT (0,0,0)
 */

bool IndexingUtils::ValidIndex(const V3D &hkl, double tolerance) {
  if ((hkl[0] == 0.) && (hkl[1] == 0.) && (hkl[2] == 0.))
    return false;

  return (withinTol(hkl[0], tolerance) && withinTol(hkl[1], tolerance) && withinTol(hkl[2], tolerance));
}

/**
  Find number of valid HKLs and average error for the valid Miller indices,
  in a list of HKLs.

  @param hkls          List of V3D objects containing hkl values
  @param tolerance     The maximum acceptable deviation from integer values for
                       the Miller indices.
  @param average_error This is set to the average error in the hkl values for
                       the hkl values that are valid Miller indices.
*/
int IndexingUtils::NumberOfValidIndexes(const std::vector<V3D> &hkls, double tolerance, double &average_error) {

  double h_error;
  double k_error;
  double l_error;
  double total_error = 0;
  int count = 0;
  for (const auto &hkl : hkls) {
    if (ValidIndex(hkl, tolerance)) {
      count++;
      h_error = fabs(round(hkl[0]) - hkl[0]);
      k_error = fabs(round(hkl[1]) - hkl[1]);
      l_error = fabs(round(hkl[2]) - hkl[2]);
      total_error += h_error + k_error + l_error;
    }
  }

  if (count > 0)
    average_error = total_error / (3.0 * static_cast<double>(count));
  else
    average_error = 0.0;

  return count;
}

/// Find the average indexing error for UB with the specified q's and hkls
double IndexingUtils::IndexingError(const DblMatrix &UB, const std::vector<V3D> &hkls,
                                    const std::vector<V3D> &q_vectors) {
  DblMatrix UB_inverse(UB);
  if (CheckUB(UB)) {
    UB_inverse.Invert();
  } else {
    throw std::runtime_error("The UB in IndexingError() is not valid");
  }
  if (hkls.size() != q_vectors.size()) {
    throw std::runtime_error("Different size hkl and q_vectors in IndexingError()");
  }

  double total_error = 0;
  V3D hkl;
  for (size_t i = 0; i < hkls.size(); i++) {
    hkl = UB_inverse * q_vectors[i] / (2.0 * M_PI);

    double h_error = fabs(hkl[0] - round(hkl[0]));
    double k_error = fabs(hkl[1] - round(hkl[1]));
    double l_error = fabs(hkl[2] - round(hkl[2]));
    total_error += h_error + k_error + l_error;
  }

  if (!hkls.empty())
    return total_error / (3.0 * static_cast<double>(hkls.size()));
  else
    return 0;
}

/**
  Check whether or not the specified matrix is reasonable for an orientation
  matrix.  In particular, check that it is a 3x3 matrix without any nan or
  infinite values and that its determinant is within a reasonable range, for
  an orientation matrix.

  @param UB  A 3x3 matrix of doubles holding the UB matrix

  @return true if this could be a valid UB matrix.
 */

bool IndexingUtils::CheckUB(const DblMatrix &UB) {
  if (UB.numRows() != 3 || UB.numCols() != 3)
    return false;

  for (size_t row = 0; row < 3; row++)
    for (size_t col = 0; col < 3; col++) {
      if (!std::isfinite(UB[row][col])) {
        return false;
      }
    }

  double det = UB.determinant();

  double abs_det = fabs(det);

  return !(abs_det > 10 || abs_det < 1e-12);
}

/**
  Calculate the number of Q vectors that are mapped to integer h,k,l
  values by UB.  Each of the Miller indexes, h, k and l must be within
  the specified tolerance of an integer, in order to count the peak
  as indexed.  Also, if (h,k,l) = (0,0,0) the peak will NOT be counted
  as indexed, since (0,0,0) is not a valid index of any peak.

  @param UB           A 3x3 matrix of doubles holding the UB matrix.
                      The UB matrix must not be singular.
  @param q_vectors    std::vector of V3D objects that contains the list of
                      q_vectors that are indexed by the corresponding hkl
                      vectors.
  @param tolerance    The maximum allowed distance between each component
                      of UB^(-1)*Q and the nearest integer value, required to
                      to count the peak as indexed by UB.

  @return A non-negative integer giving the number of peaks indexed by UB.

  @throws std::invalid_argument exception if the UB matrix is not a 3X3 matrix,
                                or if UB is singular.
 */
int IndexingUtils::NumberIndexed(const DblMatrix &UB, const std::vector<V3D> &q_vectors, double tolerance) {
  int count = 0;

  DblMatrix UB_inverse(UB);
  if (CheckUB(UB)) {
    UB_inverse.Invert();
  } else {
    throw std::runtime_error("The UB in NumberIndexed() is not valid");
  }

  V3D hkl;
  for (const auto &q_vector : q_vectors) {
    hkl = UB_inverse * q_vector / (2.0 * M_PI);
    if (ValidIndex(hkl, tolerance)) {
      count++;
    }
  }

  return count;
}

/**
  Calculate the number of Q vectors that are mapped to a integer index
  value by taking the dot product with the specified direction vector.  The
  direction vector represents a possible unit cell edge vector in real space.
  The dot product must be within the specified tolerance of an integer,
  in order to count as indexed.

  @param direction    A V3D specifying a possible edge vector in real space.
  @param q_vectors    std::vector of V3D objects that contains the list of
                      q_vectors that are indexed by the corresponding hkl
                      vectors.
  @param tolerance    The maximum allowed distance to an integer from the dot
                      products of peaks with the specified direction.

  @return A non-negative integer giving the number of q-vectors indexed in
          one direction by the specified direction vector.
 */
int IndexingUtils::NumberIndexed_1D(const V3D &direction, const std::vector<V3D> &q_vectors, double tolerance) {
  if (direction.norm() == 0)
    return 0;

  int count = 0;

  for (const auto &q_vector : q_vectors) {
    double proj_value = direction.scalar_prod(q_vector) / (2.0 * M_PI);
    double error = fabs(proj_value - std::round(proj_value));
    if (error <= tolerance) {
      count++;
    }
  }

  return count;
}

/**
   Calculate the number of Q vectors for which the dot product with three
   specified direction vectors is an integer triple, NOT equal to (0,0,0) to
   within the specified tolerance.  This give the number of peaks that would
   be indexed by the UB matrix formed from the specified those three real
   space unit cell edge vectors.
   NOTE: This method assumes that the three edge vectors are linearly
         independent and could be used to form a valid UB matrix.

   @param a_dir        A Vector3D representing unit cell edge vector a
   @param b_dir        A Vector3D representing unit cell edge vector b
   @param c_dir        A Vector3D representing unit cell edge vector c
   @param q_vectors    Vector of Vector3D objects that contains the list of
                       q_vectors that are indexed by the corresponding hkl
                       vectors.
   @param tolerance    The maximum allowed distance to an integer from the dot
                       products of peaks with the specified direction.

   @return A non-negative integer giving the number of peaks simultaneously
           indexed in all three directions by the specified direction vectors.
 */
int IndexingUtils::NumberIndexed_3D(const V3D &a_dir, const V3D &b_dir, const V3D &c_dir,
                                    const std::vector<V3D> &q_vectors, double tolerance) {
  if (a_dir.norm() == 0 || b_dir.norm() == 0 || c_dir.norm() == 0)
    return 0;

  V3D hkl_vec;
  int count = 0;

  for (const auto &q_vector : q_vectors) {
    hkl_vec[0] = a_dir.scalar_prod(q_vector) / (2.0 * M_PI);
    hkl_vec[1] = b_dir.scalar_prod(q_vector) / (2.0 * M_PI);
    hkl_vec[2] = c_dir.scalar_prod(q_vector) / (2.0 * M_PI);
    if (ValidIndex(hkl_vec, tolerance)) {
      count++;
    }
  }

  return count;
}

/**
  Calculate the Miller Indices for each of the specified Q vectors, using the
  inverse of the specified UB matrix.  The Miller Indices will be set to
  0, 0, 0 for any peak for which h, k or l differs from an intenger by more
  than the specified tolerance.  Also (h,k,l) = (0,0,0) the peak will NOT be
  counted as indexed, since (0,0,0) is not a valid index of any peak.

  @param UB             A 3x3 matrix of doubles holding the UB matrix
  @param q_vectors      std::vector of V3D objects that contains the list of
                        q_vectors that are to be indexed.
  @param tolerance      The maximum allowed distance between each component
                        of UB^(-1)*Q and the nearest integer value, required to
                        to count the peak as indexed by UB.
  @param miller_indices This vector returns a list of Miller Indices, with
                        one entry for each given Q vector.
  @param ave_error      The average error from all lattice directions.

  @return A non-negative integer giving the number of peaks indexed by UB,
          within the specified tolerance on h,k,l.

  @throws std::invalid_argument exception if the UB matrix is not a 3X3 matrix,
                                or if UB is singular.
 */
int IndexingUtils::CalculateMillerIndices(const DblMatrix &UB, const std::vector<V3D> &q_vectors, double tolerance,
                                          std::vector<V3D> &miller_indices, double &ave_error) {
  DblMatrix UB_inverse(UB);

  if (CheckUB(UB)) {
    UB_inverse.Invert();
  } else {
    throw std::runtime_error("The UB in CalculateMillerIndices() is not valid");
  }

  miller_indices.clear();
  miller_indices.reserve(q_vectors.size());

  int count = 0;
  ave_error = 0.0;
  for (const auto &q_vector : q_vectors) {
    V3D hkl;
    if (CalculateMillerIndices(UB_inverse, q_vector, tolerance, hkl)) {
      count++;
      ave_error += hkl.hklError();
    }
    miller_indices.emplace_back(hkl);
  }

  if (count > 0) {
    ave_error /= (3.0 * count);
  }

  return count;
}

/**
  Calculate the Miller Indices for the specified Q vector, using the
  inverse of the specified UB matrix. If the peak could not be indexed it is
  set to (0,0,0)

  @param inverseUB A 3x3 matrix of doubles holding the inverse UB matrix.
                   The matrix is not checked for validity
  @param q_vector  std::vector of V3D objects that contains the list of
                   q_vectors that are to be indexed.
  @param tolerance The maximum allowed distance between each component
                   of UB^(-1)*Q and the nearest integer value, required to
                   to count the peak as indexed by UB.
  @param miller_indices This vector returns a list of Miller Indices, with
                        one entry for each given Q vector.

  @return True if the peak was index, false otherwise

 */

bool IndexingUtils::CalculateMillerIndices(const DblMatrix &inverseUB, const V3D &q_vector, double tolerance,
                                           V3D &miller_indices) {
  miller_indices = CalculateMillerIndices(inverseUB, q_vector);
  if (ValidIndex(miller_indices, tolerance)) {
    return true;
  } else {
    miller_indices = V3D(0, 0, 0);
    return false;
  }
}

/**
  Calculate the Miller Indices for the specified Q vector, using the
  inverse of the specified UB matrix.

  @param inverseUB A 3x3 matrix of doubles holding the inverse UB matrix.
                   The matrix is not checked for validity
  @param q_vector V3D object containing Q vector in sample frame

  @return The indexes of the given peak. They have not been tested for validity

 */
V3D IndexingUtils::CalculateMillerIndices(const DblMatrix &inverseUB, const V3D &q_vector) {
  return inverseUB * q_vector / (2.0 * M_PI);
}

/**
  Given one plane normal direction for a family of parallel planes in
  reciprocal space, find the peaks that lie on these planes to within the
  specified tolerance.  The direction is specified as a vector with length
  "a" if the plane spacing in reciprocal space is 1/a.  In that way, the
  dot product of a peak Qxyz with the direction vector will be an integer
  if the peak lies on one of the planes.

  @param direction           Direction vector in the direction of the
                             normal vector for a family of parallel planes
                             in reciprocal space.  The length of this vector
                             must be the reciprocal of the plane spacing.
  @param q_vectors           List of V3D peaks in reciprocal space
  @param required_tolerance  The maximum allowed error (as a faction of
                             the corresponding Miller index) for a peak
                             q_vector to be counted as indexed.
  @param index_vals          List of the one-dimensional Miller indices peaks
                             that were indexed in the specified direction.
  @param indexed_qs          List of Qxyz value for the peaks that were
                             indexed indexed in the specified direction.
  @param fit_error           The sum of the squares of the distances from
                             integer values for the projections of the
                             indexed q_vectors on the specified direction.
                             This is a measure of the error in HKL space.

  @return The number of q_vectors that are indexed to within the specified
          tolerance, in the specified direction.
 */
int IndexingUtils::GetIndexedPeaks_1D(const V3D &direction, const std::vector<V3D> &q_vectors,
                                      double required_tolerance, std::vector<int> &index_vals,
                                      std::vector<V3D> &indexed_qs, double &fit_error) {
  int num_indexed = 0;
  index_vals.clear();
  indexed_qs.clear();
  index_vals.reserve(q_vectors.size());
  indexed_qs.reserve(q_vectors.size());

  fit_error = 0;

  if (direction.norm() == 0) // special case, zero vector will NOT index
    return 0;                // any peaks, even though dot product
                             // with Q vectors is always an integer!

  for (const auto &q_vector : q_vectors) {
    double proj_value = direction.scalar_prod(q_vector) / (2.0 * M_PI);
    double nearest_int = std::round(proj_value);
    double error = fabs(proj_value - nearest_int);
    if (error < required_tolerance) {
      fit_error += error * error;
      indexed_qs.emplace_back(q_vector);
      index_vals.emplace_back(boost::numeric_cast<int>(nearest_int));
      num_indexed++;
    }
  }

  return num_indexed;
}

/**
  Given three plane normal directions for three families of parallel planes in
  reciprocal space, find the peaks that lie on these planes to within the
  specified tolerance.  The three directions are specified as vectors with
  lengths that are the reciprocals of the corresponding plane spacings.  In
  that way, the dot product of a peak Qxyz with one of the direction vectors
  will be an integer if the peak lies on one of the planes corresponding to
  that direction.  If the three directions are properly chosen to correspond
  to the unit cell edges, then the resulting indices will be proper Miller
  indices for the peaks.  This method is similar to GetIndexedPeaks_1D, but
  checks three directions simultaneously and requires that the peak lies
  on all three families of planes simultaneously and does NOT index as
  (0,0,0).

  @param direction_1         Direction vector in the direction of the normal
                             vector for the first family of parallel planes.
  @param direction_2         Direction vector in the direction of the normal
                             vector for the second family of parallel planes.
  @param direction_3         Direction vector in the direction of the normal
                             vector for the third family of parallel planes.
  @param q_vectors           List of V3D peaks in reciprocal space
  @param required_tolerance  The maximum allowed error (as a faction of
                             the corresponding Miller index) for a peak
                             q_vector to be counted as indexed.
  @param miller_indices      List of the Miller indices (h,k,l) of peaks
                             that were indexed in all specified directions.
  @param indexed_qs          List of Qxyz value for the peaks that were
                             indexed indexed in all specified directions.
  @param fit_error           The sum of the squares of the distances from
                             integer values for the projections of the
                             indexed q_vectors on the specified directions.
                             This is a measure of the error in HKL space.

  @return The number of q_vectors that are indexed to within the specified
          tolerance, in the specified direction.
 */
int IndexingUtils::GetIndexedPeaks_3D(const V3D &direction_1, const V3D &direction_2, const V3D &direction_3,
                                      const std::vector<V3D> &q_vectors, double required_tolerance,
                                      std::vector<V3D> &miller_indices, std::vector<V3D> &indexed_qs,
                                      double &fit_error) {
  V3D hkl;
  int num_indexed = 0;

  miller_indices.clear();
  miller_indices.reserve(q_vectors.size());

  indexed_qs.clear();
  indexed_qs.reserve(q_vectors.size());

  fit_error = 0;

  for (const auto &q_vector : q_vectors) {
    double projected_h = direction_1.scalar_prod(q_vector) / (2.0 * M_PI);
    double projected_k = direction_2.scalar_prod(q_vector) / (2.0 * M_PI);
    double projected_l = direction_3.scalar_prod(q_vector) / (2.0 * M_PI);

    hkl(projected_h, projected_k, projected_l);

    if (ValidIndex(hkl, required_tolerance)) {
      double h_int = std::round(projected_h);
      double k_int = std::round(projected_k);
      double l_int = std::round(projected_l);

      double h_error = fabs(projected_h - h_int);
      double k_error = fabs(projected_k - k_int);
      double l_error = fabs(projected_l - l_int);

      fit_error += h_error * h_error + k_error * k_error + l_error * l_error;

      indexed_qs.emplace_back(q_vector);

      V3D miller_ind(h_int, k_int, l_int);
      miller_indices.emplace_back(miller_ind);

      num_indexed++;
    }
  }

  return num_indexed;
}

/**
  Given a list of peak positions and a UB matrix, get the list of Miller
  indices and corresponding peak positions for the peaks that are indexed
  to within a specified tolerance, by the UB matrix.  This method is similar
  to GetIndexedPeaks_3D, but directly uses the inverse of the UB matrix to
  map Q -> hkl.

  @param UB                  The UB matrix that determines the indexing of
                             the peaks.
  @param q_vectors           List of V3D peaks in reciprocal space
  @param required_tolerance  The maximum allowed error (as a faction of
                             the corresponding Miller index) for a peak
                             q_vector to be counted as indexed.
  @param miller_indices      List of the Miller indices (h,k,l) of peaks
                             that were indexed in all specified directions.
  @param indexed_qs          List of Qxyz value for the peaks that were
                             indexed indexed in all specified directions.
  @param fit_error           The sum of the squares of the distances from
                             integer values for the UB*Q for the specified
                             UB matrix and the specified q_vectors.
                             This is a measure of the error in HKL space.

  @return The number of q_vectors that are indexed to within the specified
          tolerance, in the specified direction.
 */
int IndexingUtils::GetIndexedPeaks(const DblMatrix &UB, const std::vector<V3D> &q_vectors, double required_tolerance,
                                   std::vector<V3D> &miller_indices, std::vector<V3D> &indexed_qs, double &fit_error) {
  double error;
  int num_indexed = 0;
  V3D hkl;

  miller_indices.clear();
  miller_indices.reserve(q_vectors.size());

  indexed_qs.clear();
  indexed_qs.reserve(q_vectors.size());

  fit_error = 0;

  DblMatrix UB_inverse(UB);
  if (CheckUB(UB)) {
    UB_inverse.Invert();
  } else {
    throw std::runtime_error("The UB in GetIndexedPeaks() is not valid");
  }

  for (const auto &q_vector : q_vectors) {
    hkl = UB_inverse * q_vector / (2.0 * M_PI);

    if (ValidIndex(hkl, required_tolerance)) {
      for (int i = 0; i < 3; i++) {
        error = hkl[i] - std::round(hkl[i]);
        fit_error += error * error;
      }

      indexed_qs.emplace_back(q_vector);

      V3D miller_ind(round(hkl[0]), round(hkl[1]), round(hkl[2]));
      miller_indices.emplace_back(miller_ind);

      num_indexed++;
    }
  }

  return num_indexed;
}

/**
  Make a list of directions, approximately uniformly distributed over a
  hemisphere, with the angular separation between direction vectors
  approximately 90 degrees/n_steps.
  NOTE: This method provides a list of possible directions for plane
        normals for reciprocal lattice planes.  This facilitates a
        brute force search for lattice planes with a specific spacing
        between planes.  This will be used for finding the UB matrix,
        given the lattice parameters.

  @param n_steps   The number of subdivisions in latitude in the upper
                   hemisphere.

  @return A std::vector containing directions distributed over the hemisphere
          with y-coordinate at least zero.

  @throws std::invalid_argument exception if the number of steps is <= 0.
 */
std::vector<V3D> IndexingUtils::MakeHemisphereDirections(int n_steps) {
  if (n_steps <= 0) {
    throw std::invalid_argument("MakeHemisphereDirections(): n_steps must be greater than 0");
  }

  std::vector<V3D> direction_list;

  double angle_step = M_PI / (2 * n_steps);

  for (int iPhi = 0; iPhi < n_steps + 1; ++iPhi) {
    double phi = static_cast<double>(iPhi) * angle_step;
    double r = sin(phi);

    int n_theta = boost::math::iround(2. * M_PI * r / angle_step);

    double theta_step;
    if (n_theta == 0) {            // n = ( 0, 1, 0 ).  Just
      theta_step = 2. * M_PI + 1.; // use one vector at the pole
      n_theta = 1;
    } else {
      theta_step = 2. * M_PI / static_cast<double>(n_theta);
    }

    // use half the equator to avoid vectors that are the negatives of other
    // vectors in the list.
    if (fabs(phi - M_PI / 2.) < angle_step / 2.) {
      n_theta /= 2;
    }

    for (int jTheta = 0; jTheta < n_theta; ++jTheta) {
      double theta = static_cast<double>(jTheta) * theta_step;
      direction_list.emplace_back(r * cos(theta), cos(phi), r * sin(theta));
    }
  }
  return direction_list;
}

/**
  Make a list of directions, uniformly distributed around a circle, all of
  which form the specified angle with the specified axis.

  @param n_steps   The number of vectors to generate around the circle.
  @param axis      The specified axis
  @param angle_degrees  The specified angle

  @return A std::vector containing direction vectors forming the same angle
          with the axis.

  @throws std::invalid_argument exception if the number of steps is <= 0, or
                                if the axix length is 0.
 */
std::vector<V3D> IndexingUtils::MakeCircleDirections(int n_steps, const Mantid::Kernel::V3D &axis,
                                                     double angle_degrees) {
  if (n_steps <= 0) {
    throw std::invalid_argument("MakeCircleDirections(): n_steps must be greater than 0");
  }
  // first get a vector perpendicular to axis
  double max_component = fabs(axis[0]);
  double min_component = max_component;
  size_t min_index = 0;
  for (size_t i = 1; i < 3; i++) {
    if (fabs(axis[i]) < min_component) {
      min_component = fabs(axis[i]);
      min_index = i;
    }
    if (fabs(axis[i]) > max_component) {
      max_component = fabs(axis[i]);
    }
  }

  if (max_component == 0) {
    throw std::invalid_argument("MakeCircleDirections(): Axis vector must be non-zero!");
  }

  V3D second_vec(0, 0, 0);
  second_vec[min_index] = 1;

  const V3D perp_vec = normalize(second_vec.cross_prod(axis));

  // next get a vector that is the specified
  // number of degrees away from the axis
  Quat rotation_1(angle_degrees, perp_vec);
  V3D vector_at_angle(axis);
  rotation_1.rotate(vector_at_angle);
  vector_at_angle.normalize();

  // finally, form the circle of directions
  // consisting of vectors that are at the
  // specified angle from the original axis
  double angle_step = 360.0 / n_steps;
  Quat rotation_2(0, axis);
  std::vector<V3D> directions;
  for (int i = 0; i < n_steps; i++) {
    V3D vec(vector_at_angle);
    rotation_2.setAngleAxis(i * angle_step, axis);
    rotation_2.rotate(vec);
    directions.emplace_back(vec);
  }

  return directions;
}

/**
  Choose the direction vector that most nearly corresponds to a family of
  planes in the list of q_vectors, with spacing equal to the specified
  plane_spacing.  The direction is chosen from the specified direction_list.

  @param  best_direction      This will be set to the direction that minimizes
                              the sum squared distances of projections of
  peaks from integer multiples of the specified plane spacing.
  @param  q_vectors           List of peak positions, specified according to
                              the convention that |q| = 1/d.  (i.e. Q/2PI)
  @param  direction_list      List of possible directions for plane normals.
                              Initially, this will be a long list of possible
                              directions from MakeHemisphereDirections().
  @param  plane_spacing       The required spacing between planes in
  reciprocal space.
  @param  required_tolerance  The maximum deviation of the component of a
                              peak Qxyz in the direction of the best_direction
                              vector for that peak to count as being indexed.
                              NOTE: The tolerance is specified in terms of
                              Miller Index.  That is, the distance between
                              adjacent planes is effectively normalized to one
                              for measuring the error in the computed index.

  @return The number of peaks that lie within the specified tolerance of the
          family of planes with normal direction = best_direction and with
          spacing given by plane_spacing.

  @throws invalid_argument exception of no Q vectors or directions are
                           specified.
 */
int IndexingUtils::SelectDirection(V3D &best_direction, const std::vector<V3D> &q_vectors,
                                   const std::vector<V3D> &direction_list, double plane_spacing,
                                   double required_tolerance) {
  if (q_vectors.empty()) {
    throw std::invalid_argument("SelectDirection(): No Q vectors specified");
  }

  if (direction_list.empty()) {
    throw std::invalid_argument("SelectDirection(): List of possible directions has zero length");
  }

  double error;
  double min_sum_sq_error = 1.0e100;

  for (auto direction : direction_list) {
    double sum_sq_error = 0;
    direction /= plane_spacing;
    for (const auto &q_vector : q_vectors) {
      double dot_product = direction.scalar_prod(q_vector) / (2.0 * M_PI);
      error = std::abs(dot_product - std::round(dot_product));
      sum_sq_error += error * error;
    }

    if (sum_sq_error < min_sum_sq_error + DBL_EPSILON) {
      min_sum_sq_error = sum_sq_error;
      best_direction = direction;
    }
  }

  int num_indexed = 0;
  for (const auto &q_vector : q_vectors) {
    double proj_value = best_direction.scalar_prod(q_vector) / (2.0 * M_PI);
    error = std::abs(proj_value - std::round(proj_value));
    if (error < required_tolerance)
      num_indexed++;
  }

  best_direction.normalize();

  return num_indexed;
}

/**
 *  Get the lattice parameters, a, b, c, alpha, beta, gamma and cell volume
 *  for the specified orientation matrix.
 *
 *  @param UB           A non-singular matrix representing an orientation
 *                      matrix.
 *  @param lattice_par  std::vector of doubles that will contain the lattice
 *                      parameters and cell volume as it's first seven
 * entries.
 *  @return true if the lattice_par vector was filled with the lattice
 *          parameters and false if the matrix could not be inverted.
 */
bool IndexingUtils::GetLatticeParameters(const DblMatrix &UB, std::vector<double> &lattice_par) {
  OrientedLattice o_lattice;
  o_lattice.setUB(UB);

  lattice_par.clear();
  lattice_par.emplace_back(o_lattice.a());
  lattice_par.emplace_back(o_lattice.b());
  lattice_par.emplace_back(o_lattice.c());

  lattice_par.emplace_back(o_lattice.alpha());
  lattice_par.emplace_back(o_lattice.beta());
  lattice_par.emplace_back(o_lattice.gamma());

  lattice_par.emplace_back(o_lattice.volume()); // keep volume > 0 even if
                                                // cell is left handed
  return true;
}

int IndexingUtils::GetModulationVectors(const DblMatrix &UB, const DblMatrix &ModUB, V3D &ModVec1, V3D &ModVec2,
                                        V3D &ModVec3) {
  OrientedLattice o_lattice;
  o_lattice.setUB(UB);
  o_lattice.setModUB(ModUB);

  ModVec1 = o_lattice.getModVec(0);
  ModVec2 = o_lattice.getModVec(1);
  ModVec3 = o_lattice.getModVec(2);

  int ModDim = 0;
  if (o_lattice.getdh(0) != 0.0 || o_lattice.getdk(0) != 0.0 || o_lattice.getdl(0) != 0.0)
    ModDim = 1;
  if (o_lattice.getdh(1) != 0.0 || o_lattice.getdk(1) != 0.0 || o_lattice.getdl(1) != 0.0)
    ModDim = 2;
  if (o_lattice.getdh(2) != 0.0 || o_lattice.getdk(2) != 0.0 || o_lattice.getdl(2) != 0.0)
    ModDim = 3;

  return ModDim;
}

bool IndexingUtils::GetModulationVector(const DblMatrix &UB, const DblMatrix &ModUB, V3D &ModVec, const int j) {
  OrientedLattice o_lattice;
  o_lattice.setUB(UB);
  o_lattice.setModUB(ModUB);

  ModVec = o_lattice.getModVec(j);

  if (ModVec[0] != 0.0 || ModVec[1] != 0.0 || ModVec[2] != 0.0)
    return true;
  else
    return false;
}

/**
 *  Get a nicely formatted string listing the lattice parameters and cell
 *  volume.
 *
 *  @return a string listing the cell parameters.
 */
std::string IndexingUtils::GetLatticeParameterString(const DblMatrix &UB) {
  std::vector<double> lat_par;
  IndexingUtils::GetLatticeParameters(UB, lat_par);

  char buffer[100];

  sprintf(buffer, std::string(" %8.4f %8.4f %8.4f  %8.3f %8.3f %8.3f  %9.2f").c_str(), lat_par[0], lat_par[1],
          lat_par[2], lat_par[3], lat_par[4], lat_par[5], lat_par[6]);

  std::string result(buffer);
  return result;
}
