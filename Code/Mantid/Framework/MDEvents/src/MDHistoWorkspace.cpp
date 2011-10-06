#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Utils.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidMDEvents/MDHistoWorkspaceIterator.h"
#include "MantidKernel/VMD.h"

using namespace Mantid::Kernel;
using Mantid::Geometry::IMDDimension_sptr;
using Mantid::Geometry::IMDDimension;

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
  /** Destructor
   */
  MDHistoWorkspace::~MDHistoWorkspace()
  {
    delete [] m_signals;
    delete [] m_errors;
    delete [] indexMultiplier;
    delete [] m_vertexesArray;
    delete [] m_boxLength;
    delete [] m_indexMaker;
    delete [] m_indexMax;
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor helper method
   * @param dimensions :: vector of MDHistoDimension; no limit to how many.
   */
  void MDHistoWorkspace::init(std::vector<Mantid::Geometry::MDHistoDimension_sptr> & dimensions)
  {
    std::vector<IMDDimension_sptr> dim2;
    for (size_t i=0; i<dimensions.size(); i++) dim2.push_back(boost::dynamic_pointer_cast<IMDDimension>(dimensions[i]));
    MDGeometry::initGeometry(dim2);

    // Copy the dimensions array
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
      volume *= dimensions[i]->getBinWidth();
    m_inverseVolume = 1.0 / volume;

    // Continue with the vertexes array
    this->initVertexesArray();
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
  void MDHistoWorkspace::applyImplicitFunction(Mantid::Geometry::MDImplicitFunction * function, signal_t signal, signal_t error)
  {
    if (numDimensions<3) throw std::invalid_argument("Need 3 dimensions for ImplicitFunction.");
    Mantid::coord_t coord[3];
    for (size_t x=0; x<m_dimensions[0]->getNBins(); x++)
    {
      coord[0] = m_dimensions[0]->getX(x);
      for (size_t y=0; y<m_dimensions[1]->getNBins(); y++)
      {
        coord[1] = m_dimensions[1]->getX(y);
        for (size_t z=0; z<m_dimensions[2]->getNBins(); z++)
        {
          coord[2] = m_dimensions[2]->getX(z);
          
          if (!function->isPointContained(coord))
          {
            m_signals[x + indexMultiplier[0]*y + indexMultiplier[1]*z] = signal;
            m_errors[x + indexMultiplier[0]*y + indexMultiplier[1]*z] = error;
          }
        }
      }
    }
  }

  //----------------------------------------------------------------------------------------------
  /** After initialization, call this to initialize the vertexes array
   * to the vertexes of the 0th box.
   * Will be used by getVertexesArray()
   * */
  void MDHistoWorkspace::initVertexesArray()
  {
    size_t nd = numDimensions;
    // How many vertices does one box have? 2^nd, or bitwise shift left 1 by nd bits
    size_t numVertices = 1 << numDimensions;

    // Allocate the array of the right size
    m_vertexesArray = new coord_t[nd*numVertices];

    // For each vertex, increase an integeer
    for (size_t i=0; i<numVertices; ++i)
    {
      // Start point index in the output array;
      size_t outIndex = i * nd;

      // Coordinates of the vertex
      for (size_t d=0; d<nd; d++)
      {
        // Use a bit mask to look at each bit of the integer we are iterating through.
        size_t mask = 1 << d;
        if ((i & mask) > 0)
        {
          // Bit is 1, use the max of the dimension
          m_vertexesArray[outIndex + d] = m_dimensions[d]->getX(1);
        }
        else
        {
          // Bit is 0, use the min of the dimension
          m_vertexesArray[outIndex + d] = m_dimensions[d]->getX(0);
        }
      } // (for each dimension)
    }

    // Now set the m_boxLength
    m_boxLength = new coord_t[nd];
    for (size_t d=0; d<nd; d++)
      m_boxLength[d] = m_dimensions[d]->getX(1) - m_dimensions[d]->getX(0);

    // The index calculator
    m_indexMax = new size_t[numDimensions];
    for (size_t d=0; d<nd; d++)
      m_indexMax[d] = m_dimensions[d]->getNBins();
    m_indexMaker = new size_t[numDimensions];
    Utils::NestedForLoop::SetUpIndexMaker(numDimensions, m_indexMaker, m_indexMax);
  }

  //----------------------------------------------------------------------------------------------
  /** For the volume at the given linear index,
   * Return the vertices of every corner of the box, as
   * a bare array of length numVertices * nd
   *
   * @param linearIndex :: index into the workspace. Same as for getSignalAt(index)
   * @param[out] numVertices :: returns the number of vertices in the array.
   * @return the bare array. This should be deleted by the caller!
   * */
  coord_t * MDHistoWorkspace::getVertexesArray(size_t linearIndex, size_t & numVertices) const
  {
    // How many vertices does one box have? 2^nd, or bitwise shift left 1 by nd bits
    numVertices = 1 << numDimensions;

    // Index into each dimension. Built from the linearIndex.
    size_t dimIndexes[10];
    Utils::NestedForLoop::GetIndicesFromLinearIndex(numDimensions, linearIndex, m_indexMaker, m_indexMax, dimIndexes);

    // The output vertexes coordinates
    coord_t * out = new coord_t[numDimensions * numVertices];
    for (size_t i=0; i<numVertices; ++i)
    {
      size_t outIndex = i * numDimensions;
      // Offset the 0th box by the position of this linear index, in each dimension
      for (size_t d=0; d<numDimensions; d++)
        out[outIndex+d] = m_vertexesArray[outIndex+d] + m_boxLength[d] * coord_t(dimIndexes[d]);
    }

    return out;
  }

  //----------------------------------------------------------------------------------------------
  /** Return the position of the center of a bin at a given position
   *
   * @param linearIndex :: linear index into the workspace
   * @return VMD vector of the center position
   */
  Mantid::Kernel::VMD MDHistoWorkspace::getCenter(size_t linearIndex) const
  {
    // Index into each dimension. Built from the linearIndex.
    size_t dimIndexes[10];
    Utils::NestedForLoop::GetIndicesFromLinearIndex(numDimensions, linearIndex, m_indexMaker, m_indexMax, dimIndexes);

    // The output vertexes coordinates
    VMD out(numDimensions);
    // Offset the 0th box by the position of this linear index, in each dimension, plus a half
    for (size_t d=0; d<numDimensions; d++)
      out[d] = m_vertexesArray[d] + m_boxLength[d] * (coord_t(dimIndexes[d]) + 0.5);
    return out;
  }


  //----------------------------------------------------------------------------------------------
  /// Creates a new iterator pointing to the first cell in the workspace
  Mantid::API::IMDIterator* MDHistoWorkspace::createIterator() const
  {
//    return new MDHistoWorkspaceIterator(this);
    return NULL;
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


  /*
  Get non-collapsed dimensions
  @return vector of collapsed dimensions in the workspace geometry.
  */
  Mantid::Geometry::VecIMDDimension_const_sptr MDHistoWorkspace::getNonIntegratedDimensions() const
  {
    using namespace Mantid::Geometry;
    VecIMDDimension_const_sptr vecCollapsedDimensions;
    std::vector<Mantid::Geometry::IMDDimension_sptr>::const_iterator it = this->m_dimensions.begin();
    for(; it != this->m_dimensions.end(); ++it)
    {
      IMDDimension_sptr current = (*it);
      if(!current->getIsIntegrated())
      {
        vecCollapsedDimensions.push_back(current);
      }
    }
    return vecCollapsedDimensions;
  }


} // namespace Mantid
} // namespace MDEvents

