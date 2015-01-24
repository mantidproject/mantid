#include "MantidDataObjects/RebinnedOutput.h"

#include "MantidAPI/WorkspaceFactory.h"

#include <algorithm>
#include <iostream>

namespace Mantid {
namespace DataObjects {
namespace {
/// static logger
Kernel::Logger g_log("RebinnedOutput");
}

DECLARE_WORKSPACE(RebinnedOutput)

RebinnedOutput::RebinnedOutput() : Workspace2D() {}

RebinnedOutput::~RebinnedOutput() {}

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
void RebinnedOutput::init(const std::size_t &NVectors,
                          const std::size_t &XLength,
                          const std::size_t &YLength) {
  Workspace2D::init(NVectors, XLength, YLength);
  std::size_t nHist = this->getNumberHistograms();
  this->fracArea.resize(nHist);
  for (std::size_t i = 0; i < nHist; ++i) {
    this->fracArea[i].resize(YLength);
  }
}

/**
 * Function that returns a fractional area array for a given index.
 * @param index :: the array to fetch
 * @return the requested fractional area array
 */
MantidVec &RebinnedOutput::dataF(const std::size_t index) {
  return this->fracArea[index];
}

/**
 * Function that returns a fractional area array for a given index. This
 * returns an unmodifiable array.
 * @param index :: the array to fetch
 * @return the requested fractional area array
 */
const MantidVec &RebinnedOutput::dataF(const std::size_t index) const {
  return this->fracArea[index];
}

/**
 * Function that returns a fractional area array for a given index. This
 * returns a const array.
 * @param index :: the array to fetch
 * @return the requested fractional area array
 */
const MantidVec &RebinnedOutput::readF(const std::size_t index) const {
  return this->fracArea[index];
}

/**
 * Function that sets the fractional area arrat for a given index.
 * @param index :: the particular array to set
 * @param F :: the array contained the information
 */
void RebinnedOutput::setF(const std::size_t index, const MantidVecPtr &F) {
  this->fracArea[index] = *F;
}

/**
 * This function takes the data/error arrays and divides them by the
 * corresponding fractional area array. This creates a representation that
 * is easily visualized. The Rebin and Integration algorithms will have to
 * undo this in order to properly treat the data.
 * @param hasSqrdErrs :: does the workspace have squared errors?
 */
void RebinnedOutput::finalize(bool hasSqrdErrs) {
  g_log.debug() << "Starting finalize procedure." << std::endl;
  std::size_t nHist = this->getNumberHistograms();
  g_log.debug() << "Number of histograms: " << nHist << std::endl;
  for (std::size_t i = 0; i < nHist; ++i) {
    MantidVec &data = this->dataY(i);
    MantidVec &err = this->dataE(i);
    MantidVec &frac = this->dataF(i);

    g_log.debug() << "Data (" << i << "): ";
    std::copy(data.begin(), data.end(),
              std::ostream_iterator<double>(g_log.debug(), " "));
    g_log.debug() << std::endl;

    std::transform(data.begin(), data.end(), frac.begin(), data.begin(),
                   std::divides<double>());
    if (hasSqrdErrs) {
      MantidVec frac_sqr(frac.size());
      std::transform(frac.begin(), frac.end(), frac.begin(), frac_sqr.begin(),
                     std::multiplies<double>());
      std::transform(err.begin(), err.end(), frac_sqr.begin(), err.begin(),
                     std::divides<double>());
    } else {
      std::transform(err.begin(), err.end(), frac.begin(), err.begin(),
                     std::divides<double>());
    }
    g_log.debug() << "Data Final(" << i << "): ";
    std::copy(data.begin(), data.end(),
              std::ostream_iterator<double>(g_log.debug(), " "));
    g_log.debug() << std::endl;
    g_log.debug() << "FArea (" << i << "): ";
    std::copy(frac.begin(), frac.end(),
              std::ostream_iterator<double>(g_log.debug(), " "));
    g_log.debug() << std::endl;
  }
}

} // namespace Mantid
} // namespace DataObjects
