#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"

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
    std::vector<Mantid::Geometry::MDHistoDimension_sptr> dimensions;
    if (dimX) dimensions.push_back(dimX);
    if (dimY) dimensions.push_back(dimY);
    if (dimZ) dimensions.push_back(dimZ);
    if (dimT) dimensions.push_back(dimT);
    this->init(dimensions);
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor given a vector of dimensions
   * @param dimensions :: vector of MDHistoDimension; no limit to how many.
   */
  MDHistoWorkspace::MDHistoWorkspace(std::vector<Mantid::Geometry::MDHistoDimension_sptr> & dimensions)
  : numDimensions(0)
  {
    this->init(dimensions);
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor helper method
   * @param dimensions :: vector of MDHistoDimension; no limit to how many.
   */
  void MDHistoWorkspace::init(std::vector<Mantid::Geometry::MDHistoDimension_sptr> & dimensions)
  {
    if (dimensions.size() == 0)
      throw std::invalid_argument("0 valid dimensions were given to the MDHistoWorkspace constructor!");

    // Copy the dimensions array
    m_dimensions = dimensions;
    numDimensions = m_dimensions.size();

    // For indexing.
    if (numDimensions < 4)
      indexMultiplier = new size_t[4];
    else
      indexMultiplier = new size_t[numDimensions];

    // For quick indexing, accumulate these values
    // First multiplier
    indexMultiplier[0] = m_dimensions[0]->getNBins();
    for (size_t d=1; d<numDimensions; d++)
      indexMultiplier[d] = indexMultiplier[d-1] * m_dimensions[d]->getNBins();

    // This is how many dense data points
    m_length = indexMultiplier[numDimensions-1];

    // Now fix things for < 4 dimensions. Indices > the number of dimensions will be ignored (*0)
    for (size_t d=numDimensions-1; d<4; d++)
      indexMultiplier[d] = 0;

    // Allocate the linear arrays
    m_signals = new double[m_length];
    m_errors = new double[m_length];

    // Initialize them to NAN (quickly)
    double nan = std::numeric_limits<double>::quiet_NaN();
    for (size_t i=0; i < m_length; i++)
    {
      m_signals[i] = nan;
      m_errors[i] = nan;
    }

    // Compute the volume of each cell.
    double volume = 1.0;
    for (size_t i=0; i < m_dimensions.size(); ++i)
      volume *= m_dimensions[i]->getBinWidth();
    m_inverseVolume = 1.0 / volume;
  }


  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MDHistoWorkspace::~MDHistoWorkspace()
  {
    delete [] m_signals;
    delete [] m_errors;
    delete [] indexMultiplier;
  }
  
  //----------------------------------------------------------------------------------------------
  /** Return the memory used, in bytes */
  size_t MDHistoWorkspace::getMemorySize() const
  {
    return m_length * 2 * sizeof(double);
  }

  //----------------------------------------------------------------------------------------------
  /// Return a vector containing a copy of the signal data in the workspace.
  std::vector<double> MDHistoWorkspace::getSignalDataVector() const
  {
    // TODO: Make this more efficient if needed.
    std::vector<double> out;
    out.resize(m_length, 0.0);
    for (size_t i=0; i<m_length; ++i)
      out[i] = m_signals[i];
    // This copies again! :(
    return out;
  }

  /// Return a vector containing a copy of the error data in the workspace.
  std::vector<double> MDHistoWorkspace::getErrorDataVector() const
  {
    // TODO: Make this more efficient if needed.
    std::vector<double> out;
    out.resize(m_length, 0.0);
    for (size_t i=0; i<m_length; ++i)
        out[i] = m_errors[i];
    // This copies again! :(
    return out;
  }


  //---------------------------------------------------------------------------------------------------
  std::string MDHistoWorkspace::getGeometryXML() const
  {
    using Mantid::Geometry::MDGeometryBuilderXML;
    MDGeometryBuilderXML xmlBuilder;
    // Add all dimensions.
    for(size_t i = 0; i <this->m_dimensions.size(); i++)
    {
      xmlBuilder.addOrdinaryDimension(this->m_dimensions[i]);
    }
    // Add mapping dimensions
    const size_t nDimensions = m_dimensions.size();
    if(nDimensions > 0)
    {
     xmlBuilder.addXDimension(this->getXDimension());
    }
    if(nDimensions > 1)
    {
     xmlBuilder.addYDimension(this->getYDimension());
    }
    if(nDimensions > 2)
    {
     xmlBuilder.addZDimension(this->getZDimension());
    }
    if(nDimensions > 3)
    {
     xmlBuilder.addTDimension(this->getTDimension());
    }
     // Create the xml.
    return xmlBuilder.create();
  }


} // namespace Mantid
} // namespace MDEvents

