#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"
#include "MantidAPI/Point3D.h"
#include "MantidKernel/Utils.h"
#include "MantidAPI/ImplicitFunction.h"

using Mantid::API::Point3D;
using Mantid::Kernel::Utils::NestedForLoop::SetUp;
using namespace Mantid::Kernel;

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
    m_signals = new signal_t[m_length];
    m_errors = new signal_t[m_length];

    // Initialize them to NAN (quickly)
    signal_t nan = std::numeric_limits<signal_t>::quiet_NaN();
    this->setTo(nan, nan);

    // Compute the volume of each cell.
    coord_t volume = 1.0;
    for (size_t i=0; i < numDimensions; ++i)
      volume *= m_dimensions[i]->getBinWidth();
    m_inverseVolume = 1.0 / volume;
  }


  //----------------------------------------------------------------------------------------------
  /** Sets all signals/errors in the workspace to the given values
   *
   * @param signal :: signal value to set
   * @param error :: error value to set
   */
  void MDHistoWorkspace::setTo(signal_t signal, signal_t error)
  {
    for (size_t i=0; i < m_length; i++)
    {
      m_signals[i] = signal;
      m_errors[i] = error;
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Apply an implicit function to each point; if false, set to the given value.
   *
  * @param signal :: signal value to set when function evaluates to false
  * @param error :: error value to set when function evaluates to false
  */
  void MDHistoWorkspace::applyImplicitFunction(Mantid::API::ImplicitFunction * function, signal_t signal, signal_t error)
  {
    if (numDimensions<3) throw std::invalid_argument("Need 3 dimensions for ImplicitFunction.");

    for (size_t x=0; x<m_dimensions[0]->getNBins(); x++)
    {
      double xPos = m_dimensions[0]->getX(x);
      for (size_t y=0; y<m_dimensions[1]->getNBins(); y++)
      {
        double yPos = m_dimensions[1]->getX(y);
        for (size_t z=0; z<m_dimensions[2]->getNBins(); z++)
        {
          double zPos = m_dimensions[2]->getX(z);
          Point3D p(xPos,yPos,zPos);
          if (!function->evaluate(&p))
          {
            m_signals[x + indexMultiplier[0]*y + indexMultiplier[1]*z] = signal;
            m_errors[x + indexMultiplier[0]*y + indexMultiplier[1]*z] = error;
          }
        }
      }
    }
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
    return m_length * 2 * sizeof(signal_t);
  }

  //----------------------------------------------------------------------------------------------
  /// @return a vector containing a copy of the signal data in the workspace.
  std::vector<signal_t> MDHistoWorkspace::getSignalDataVector() const
  {
    // TODO: Make this more efficient if needed.
    std::vector<signal_t> out;
    out.resize(m_length, 0.0);
    for (size_t i=0; i<m_length; ++i)
      out[i] = m_signals[i];
    // This copies again! :(
    return out;
  }

  /// @return a vector containing a copy of the error data in the workspace.
  std::vector<signal_t> MDHistoWorkspace::getErrorDataVector() const
  {
    // TODO: Make this more efficient if needed.
    std::vector<signal_t> out;
    out.resize(m_length, 0.0);
    for (size_t i=0; i<m_length; ++i)
        out[i] = m_errors[i];
    // This copies again! :(
    return out;
  }


  //---------------------------------------------------------------------------------------------------
  /** @return a XML representation of the geometry of the workspace */
  std::string MDHistoWorkspace::getGeometryXML() const
  {
    using Mantid::Geometry::MDGeometryBuilderXML;
    using Mantid::Geometry::StrictDimensionPolicy;
    MDGeometryBuilderXML<StrictDimensionPolicy> xmlBuilder;
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

  /*
  Get non-collapsed dimensions
  @return vector of collapsed dimensions in the workspace geometry.
  */
  Mantid::Geometry::VecIMDDimension_const_sptr MDHistoWorkspace::getNonIntegratedDimensions() const
  {
    using namespace Mantid::Geometry;
    typedef std::vector<Mantid::Geometry::MDHistoDimension_sptr> VecMDHistoDimension_sptr;
    VecIMDDimension_const_sptr vecCollapsedDimensions;
    VecMDHistoDimension_sptr::const_iterator it = this->m_dimensions.begin();
    for(; it != this->m_dimensions.end(); ++it)
    {
      MDHistoDimension_sptr current = (*it);
      if(!current->getIsIntegrated())
      {
        vecCollapsedDimensions.push_back(current);
      }
    }
    return vecCollapsedDimensions;
  }


} // namespace Mantid
} // namespace MDEvents

