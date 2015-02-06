//-------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------
#include "MantidKernel/SobolSequence.h"
#include <stdexcept>

namespace Mantid {
namespace Kernel {
/**
 * Constructor taking the number of dimensions for the sequence
 */
SobolSequence::SobolSequence(const unsigned int ndims)
    : QuasiRandomNumberSequence(ndims), m_gslGenerator(NULL),
      m_savedGenerator(NULL) {
  setNumberOfDimensions(ndims);
}

/**
 * Destructor
 */
SobolSequence::~SobolSequence() { deleteCurrentGenerator(); }

/**
 * Returns the next number in the sequence
 * @returns A double giving the next number in the Sobol sequence for the
 * current dimension
 */
void SobolSequence::generateNextPoint() {
  std::vector<double> &point = getNextPointCache();
  gsl_qrng_get(m_gslGenerator, point.data());
}

/**
 * Reset state back to the start of the sequence
 */
void SobolSequence::restart() { gsl_qrng_init(m_gslGenerator); }

/// Saves the current state of the generator
void SobolSequence::save() {
  m_savedGenerator = gsl_qrng_clone(m_gslGenerator);
}

/// Restores the generator to the last saved point, or the beginning if nothing
/// has been saved
void SobolSequence::restore() {
  if (m_savedGenerator) {
    gsl_qrng_memcpy(m_gslGenerator, m_savedGenerator);
  } else {
    restart();
  }
}

/**
 * Sets the number of dimensions for the generator. Note this destroys
 * any previous state information including any saved state
 */
void SobolSequence::setNumberOfDimensions(const unsigned int ndims) {
  gsl_qrng *generator =
      gsl_qrng_alloc(gsl_qrng_sobol, static_cast<unsigned int>(ndims));
  if (generator) {
    deleteCurrentGenerator();
    m_gslGenerator = generator;
  } else {
    throw std::invalid_argument(
        "SobolSequence::setNumberOfDimensions - "
        "Error initializing sequence, insufficient memory.");
  }
}

/**
 * Frees resources allocated by current generator
 */
void SobolSequence::deleteCurrentGenerator() {
  gsl_qrng_free(m_gslGenerator);
  if (m_savedGenerator) {
    gsl_qrng_free(m_savedGenerator);
  }
}
}
}
