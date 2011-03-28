#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace MDEvents
{


  //----------------------------------------------------------------------------------------------
  /** Constructor given the 4 dimensions
   * @param dimX :: X dimension binning parameters
   * @param dimY :: Y dimension binning parameters
   * @param dimZ :: Z dimension binning parameters
   * @param dimT :: T (time) dimension binning parameters
   */
  MDHistoWorkspace::MDHistoWorkspace(Mantid::Geometry::MDHistoDimension_sptr dimX, Mantid::Geometry::MDHistoDimension_sptr dimY,
      Mantid::Geometry::MDHistoDimension_sptr dimZ, Mantid::Geometry::MDHistoDimension_sptr dimT)
  : numDimensions(0)
  {
    // For quick indexing, accumulate these values
    indexMultiplier[0] = dimX->getNBins();
    indexMultiplier[1] = dimY->getNBins() * indexMultiplier[0];
    indexMultiplier[2] = dimZ->getNBins() * indexMultiplier[1];
    m_length = dimT->getNBins() * indexMultiplier[2];
    // Allocate the linear arrays
    m_signals = new double[m_length];
    m_errors = new double[m_length];
    // Initialize them to zero (quickly)
    memset(m_signals, 0, sizeof(double)*m_length);
    memset(m_errors, 0, sizeof(double)*m_length);
    // Add all the dimensions
    m_dimensions.clear();
    addDimension(dimX);
    addDimension(dimY);
    addDimension(dimZ);
    addDimension(dimT);
  }


  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MDHistoWorkspace::~MDHistoWorkspace()
  {
    delete [] m_signals;
    delete [] m_errors;
  }
  
  //----------------------------------------------------------------------------------------------
  /** Return the memory used, in bytes */
  size_t MDHistoWorkspace::getMemorySize() const
  {
    return m_length * 2 * sizeof(double);
  }


  //----------------------------------------------------------------------------------------------
  /** Add a dimension to the workspace. These must be added in order of X, Y, Z, time
   *
   * @param dim :: sptr to the dimension object.
   */
  void MDHistoWorkspace::addDimension(Mantid::Geometry::MDHistoDimension_sptr dim)
  {
    if (numDimensions >= 4)
      throw std::runtime_error("MDHistoWorkspace can only have a maximum of 4 dimensions.");
    this->m_dimensions.push_back(dim);
    numDimensions = m_dimensions.size();
  }

} // namespace Mantid
} // namespace MDEvents

