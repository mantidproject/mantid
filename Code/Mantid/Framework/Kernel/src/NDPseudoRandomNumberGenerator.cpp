//-------------------------------------------------------------------
// Includes
//-------------------------------------------------------------------
#include "MantidKernel/NDPseudoRandomNumberGenerator.h"

namespace Mantid
{
  namespace Kernel
  {
    /**
     * Constructor taking the number of dimensions & an existing generator
     * @param ndims :: The number of dimensions the point should return
     * @param singleValueGen :: An existing generator that is capable of
     * producing single random numbers. It is called ndims times for each
     * call to nextPoint
     */
    NDPseudoRandomNumberGenerator::
    NDPseudoRandomNumberGenerator(const unsigned int ndims, SingleValueGenerator singleValueGen)
      : NDRandomNumberGenerator(ndims), m_singleValueGen(singleValueGen)
    {
    }

    /**
     * Set the random number seed
     * @param seedValue :: (Re-)seed the generator
     */
    void NDPseudoRandomNumberGenerator::setSeed(const size_t seedValue)
    {
      m_singleValueGen->setSeed(seedValue);
    }

    /// Generates the next point
    void NDPseudoRandomNumberGenerator::generateNextPoint()
    {
      for(unsigned int i = 0; i < numberOfDimensions(); ++i)
      {
        this->cacheGeneratedValue(i, m_singleValueGen->nextValue());
      }
    }

    /**
     * Resets the underlying generator
     */
    void NDPseudoRandomNumberGenerator::restart()
    {
      m_singleValueGen->restart();
    }
  }
}

