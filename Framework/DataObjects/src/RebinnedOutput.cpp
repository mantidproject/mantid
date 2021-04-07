// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/RebinnedOutput.h"

#include "MantidAPI/WorkspaceFactory.h"

#include <algorithm>
#include <iterator>
#include <sstream>

namespace Mantid {
namespace DataObjects {

DECLARE_WORKSPACE(RebinnedOutput)

/**
 * Gets the name of the workspace type.
 * @return Standard string name
 */
const std::string RebinnedOutput::id() const { return "RebinnedOutput"; }

/**
 * Sets the size of the workspace and initializes arrays to zero
 * @param NVectors :: The number of vectors/histograms/detectors in the
 * workspace
 * @param XLength :: The number of X data points/bin boundaries in each vector
 * (must all be the same)
 * @param YLength :: The number of data/error points in each vector (must all be
 * the same)
 */
void RebinnedOutput::init(const std::size_t &NVectors, const std::size_t &XLength, const std::size_t &YLength) {
  Workspace2D::init(NVectors, XLength, YLength);
  std::size_t nHist = this->getNumberHistograms();
  this->fracArea.resize(nHist);
  for (std::size_t i = 0; i < nHist; ++i) {
    this->fracArea[i].resize(YLength);
  }
}

void RebinnedOutput::init(const HistogramData::Histogram &histogram) {
  Workspace2D::init(histogram);
  std::size_t nHist = this->getNumberHistograms();
  this->fracArea.resize(nHist);
  for (std::size_t i = 0; i < nHist; ++i) {
    this->fracArea[i].resize(histogram.size());
  }
}

/**
 * Function that returns a fractional area array for a given index.
 * @param index :: the array to fetch
 * @return the requested fractional area array
 */
MantidVec &RebinnedOutput::dataF(const std::size_t index) { return this->fracArea[index]; }

/**
 * Function that returns a fractional area array for a given index. This
 * returns an unmodifiable array.
 * @param index :: the array to fetch
 * @return the requested fractional area array
 */
const MantidVec &RebinnedOutput::dataF(const std::size_t index) const { return this->fracArea[index]; }

/**
 * Function that returns a fractional area array for a given index. This
 * returns a const array.
 * @param index :: the array to fetch
 * @return the requested fractional area array
 */
const MantidVec &RebinnedOutput::readF(const std::size_t index) const { return this->fracArea[index]; }

/**
 * Function that sets the fractional area array for a given index.
 * @param index :: the particular array to set
 * @param F :: the array contained the information
 */
void RebinnedOutput::setF(const std::size_t index, const MantidVecPtr &F) { this->fracArea[index] = *F; }

/**
 * Function that scales the fractional area arrays.
 * @param scale :: the scale factor
 */
void RebinnedOutput::scaleF(const double scale) {
  std::size_t nHist = this->getNumberHistograms();
  for (std::size_t i = 0; i < nHist; ++i) {
    MantidVec &frac = this->dataF(i);
    std::transform(frac.begin(), frac.end(), frac.begin(), [scale](double x) { return scale * x; });
  }
}

/**
 * The functions checks if the fractional area data is non zero.
 */
bool RebinnedOutput::nonZeroF() const {

  // Checks that the fractions are not all zeros.
  auto nHist = static_cast<int>(this->getNumberHistograms());
  bool frac_all_zeros = true;
  for (int i = 0; i < nHist; ++i) {
    const MantidVec &frac = this->readF(i);
    if (std::accumulate(frac.begin(), frac.end(), 0.) != 0) {
      frac_all_zeros = false;
      break;
    }
  }
  return !frac_all_zeros;
}

/**
 * This function takes the data/error arrays and divides them by the
 * corresponding fractional area array. This creates a representation that
 * is easily visualized. The Rebin and Integration algorithms will have to
 * undo this in order to properly treat the data. If the fractional area
 * is all zero skip the operation as there will be no meaningful data.
 * @param hasSqrdErrs :: does the workspace have squared errors?
 */
void RebinnedOutput::finalize(bool hasSqrdErrs) {
  if (m_finalized && hasSqrdErrs == m_hasSqrdErrs)
    return;

  // Checks that the fractions are not all zeros.
  if (!this->nonZeroF())
    return;

  // Fix the squared error representation before returning
  auto nHist = static_cast<int>(this->getNumberHistograms());
  if (m_finalized) {
    PARALLEL_FOR_IF(Kernel::threadSafe(*this))
    for (int i = 0; i < nHist; ++i) {
      MantidVec &err = this->dataE(i);
      MantidVec &frac = this->dataF(i);
      if (hasSqrdErrs) {
        std::transform(err.begin(), err.end(), frac.begin(), err.begin(), std::divides<double>());
      } else {
        std::transform(err.begin(), err.end(), frac.begin(), err.begin(), std::multiplies<double>());
      }
    }

    // Sets flag so subsequent algorithms know to correctly treat data
    m_hasSqrdErrs = hasSqrdErrs;
    return;
  }

  PARALLEL_FOR_IF(Kernel::threadSafe(*this))
  for (int i = 0; i < nHist; ++i) {
    MantidVec &data = this->dataY(i);
    MantidVec &err = this->dataE(i);
    MantidVec &frac = this->dataF(i);
    std::transform(data.begin(), data.end(), frac.begin(), data.begin(), std::divides<double>());
    std::transform(err.begin(), err.end(), frac.begin(), err.begin(), std::divides<double>());
    if (hasSqrdErrs) {
      std::transform(err.begin(), err.end(), frac.begin(), err.begin(), std::divides<double>());
    }
  }
  // Sets flags so subsequent algorithms know to correctly treat data
  m_finalized = true;
  m_hasSqrdErrs = hasSqrdErrs;
}

/**
 * This function "unfinalizes" the workspace by taking the data/error arrays
 * and multiplying them by the corresponding fractional area array if the array
 * is non zero. If all the values are zero skip the process as all data will
 * will be lost.
 */
void RebinnedOutput::unfinalize() {
  if (!m_finalized || !this->nonZeroF())
    return;

  auto nHist = static_cast<int>(this->getNumberHistograms());
  PARALLEL_FOR_IF(Kernel::threadSafe(*this))
  for (int i = 0; i < nHist; ++i) {
    MantidVec &data = this->dataY(i);
    MantidVec &err = this->dataE(i);
    MantidVec &frac = this->dataF(i);
    std::transform(data.begin(), data.end(), frac.begin(), data.begin(), std::multiplies<double>());
    std::transform(err.begin(), err.end(), frac.begin(), err.begin(), std::multiplies<double>());
    if (this->m_hasSqrdErrs) {
      std::transform(err.begin(), err.end(), frac.begin(), err.begin(), std::multiplies<double>());
    }
  }
  // Sets flag so subsequent algorithms know to correctly treat data
  m_finalized = false;
}

} // namespace DataObjects
} // namespace Mantid
