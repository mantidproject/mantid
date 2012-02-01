#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Utils.h"
#include "MantidKernel/VMD.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidMDEvents/MDHistoWorkspaceIterator.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/MDDimensionExtents.h"

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;

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
  /** Copy constructor
   *
   * @param other :: MDHistoWorkspace to copy from.
   */
  MDHistoWorkspace::MDHistoWorkspace(const MDHistoWorkspace & other)
  : IMDHistoWorkspace(other)
  {
    std::vector<Mantid::Geometry::MDHistoDimension_sptr> dimensions;
    for (size_t d=0; d < other.getNumDims(); d++)
    {
      // Copy the dimension
      MDHistoDimension_sptr dim(new MDHistoDimension( other.getDimension(d).get() ) );
      dimensions.push_back(dim);
    }
    this->init(dimensions);
    // Now copy all the data
    for (size_t i=0; i<m_length; ++i)
    {
      m_signals[i] = other.m_signals[i];
      m_errorsSquared[i] = other.m_errorsSquared[i];
    }
  }


  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MDHistoWorkspace::~MDHistoWorkspace()
  {
    delete [] m_signals;
    delete [] m_errorsSquared;
    delete [] indexMultiplier;
    delete [] m_vertexesArray;
    delete [] m_boxLength;
    delete [] m_indexMaker;
    delete [] m_indexMax;
    delete [] m_origin;
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
    m_errorsSquared = new signal_t[m_length];

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
   * @param errorSquared :: error (squared) value to set
   */
  void MDHistoWorkspace::setTo(signal_t signal, signal_t errorSquared)
  {
    for (size_t i=0; i < m_length; i++)
    {
      m_signals[i] = signal;
      m_errorsSquared[i] = errorSquared;
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Apply an implicit function to each point; if false, set to the given value.
   *
  * @param function :: the implicit function to apply
  * @param signal :: signal value to set when function evaluates to false
  * @param errorSquared :: error value to set when function evaluates to false
  */
  void MDHistoWorkspace::applyImplicitFunction(Mantid::Geometry::MDImplicitFunction * function, signal_t signal, signal_t errorSquared)
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
            m_errorsSquared[x + indexMultiplier[0]*y + indexMultiplier[1]*z] = errorSquared;
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

    // Now set the m_boxLength and origin
    m_boxLength = new coord_t[nd];
    m_origin = new coord_t[nd];
    for (size_t d=0; d<nd; d++)
    {
      m_boxLength[d] = m_dimensions[d]->getX(1) - m_dimensions[d]->getX(0);
      m_origin[d] = m_dimensions[d]->getX(0);
    }

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
    numVertices = (size_t)(1) << numDimensions; // Cast avoids warning about result of 32-bit shift implicitly converted to 64 bits on MSVC

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
  /** Get the signal at a particular coordinate in the workspace.
   *
   * @param coords :: numDimensions-sized array of the coordinates to look at
   * @return the (normalized) signal at a given coordinates.
   *         NaN if outside the range of this workspace
   */
  signal_t MDHistoWorkspace::getSignalAtCoord(const coord_t * coords) const
  {
    // Build up the linear index, dimension by dimension
    size_t linearIndex = 0;
    for (size_t d=0; d<numDimensions; d++)
    {
      coord_t x = coords[d] - m_origin[d];
      size_t ix = size_t(x / m_boxLength[d]);
      if (ix >= m_indexMax[d] || (x<0))
        return std::numeric_limits<signal_t>::quiet_NaN();
      linearIndex += ix * m_indexMaker[d];
    }
    return m_signals[linearIndex] * m_inverseVolume;
  }


  //----------------------------------------------------------------------------------------------
  /// Creates a new iterator pointing to the first cell in the workspace
  Mantid::API::IMDIterator* MDHistoWorkspace::createIterator(Mantid::Geometry::MDImplicitFunction * function) const
  {
    return new MDHistoWorkspaceIterator(this,function);
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
        out[i] = m_errorsSquared[i];
    // This copies again! :(
    return out;
  }



  //----------------------------------------------------------------------------------------------
  /** Helper method to find the intersection between a point and the edges
   * of a box
   *
   * @param boxExtents :: min/max of the box
   * @param pos :: current position
   * @param dirSign :: +1, -1 or 0 depending on the sign of travel direction
   * @param nd :: number of dimensions
   * @param bestDistance :: set to the best distance found (along the direction)
   * @param closestDimension :: set to the index of the closest dimension
   * @param closestIsMin :: set to true if the closest bound is the MIN edge. MAX otherwise
   * @return the distance traveled along the line
   */
  void findClosestEdgeDimension(MDDimensionExtents * boxExtents,
      VMD & pos, VMD & dirSign, size_t nd,
      coord_t & bestDistance, size_t & closestDimension, bool & closestIsMin)
  {
    // Find the extents of the current box
    bestDistance = 1e100;
    closestDimension = 0;
    closestIsMin = false;
    for (size_t d=0; d<nd; d++)
    {
      coord_t dist;
      // How close are we to the min edge?
      dist = dirSign[d] * (boxExtents[d].min - pos[d]);
      if (dist > 0 && dist < bestDistance)
      {
        // Distance > 0 = pointing in the right direction
        // and this is the closes of any one so far.
        bestDistance = dist;
        closestDimension = d;
        closestIsMin = true;
      }

      // How close are we to the max edge?
      dist = dirSign[d] * (boxExtents[d].max - pos[d]);
      if (dist > 0 && dist < bestDistance)
      {
        bestDistance = dist;
        closestDimension = d;
        closestIsMin = false;
      }
    }
  }


  //----------------------------------------------------------------------------------------------
  /** Obtain coordinates for a line plot through a MDWorkspace.
   * Cross the workspace from start to end points, recording the signal along the line.
   * Sets the x,y vectors to the histogram bin boundaries and counts
   *
   * @param start :: coordinates of the start point of the line
   * @param end :: coordinates of the end point of the line
   * @param normalize :: how to normalize the signal
   * @param x :: is set to the boundaries of the bins, relative to start of the line.
   * @param y :: is set to the normalized signal for each bin. Length = length(x) - 1
   */
  void MDHistoWorkspace::getLinePlot(const Mantid::Kernel::VMD & start, const Mantid::Kernel::VMD & end,
      Mantid::API::MDNormalization normalize, std::vector<coord_t> & x, std::vector<signal_t> & y, std::vector<signal_t> & e) const
  {
    size_t nd = this->getNumDims();
    if (start.getNumDims() != nd)
      throw std::runtime_error("Start point must have the same number of dimensions as the workspace.");
    if (end.getNumDims() != nd)
      throw std::runtime_error("End point must have the same number of dimensions as the workspace.");
    x.clear();
    y.clear();
    e.clear();

    // Unit-vector of the direction
    VMD dir = end - start;
    double length = dir.normalize();

    // Vector with +1 where direction is positive, -1 where negative
    #define sgn(x) ((x<0)?-1:((x>0)?1:0))
    VMD dirSign(nd);
    for (size_t d=0; d<nd; d++)
      dirSign[d] = sgn(dir[d]);

    // Dimensions of the workspace
    VMD min(nd), max(nd), step(nd);
    MDDimensionExtents * boxExtents = new MDDimensionExtents[nd];
    size_t * index = new size_t[nd];
    bool startOutsideRange = false;
    for (size_t d=0; d<nd; d++)
    {
      IMDDimension_const_sptr dim = this->getDimension(d);
      min[d] = dim->getMinimum();
      max[d] = dim->getMaximum();
      step[d] = dim->getBinWidth();
      boxExtents[d].min = min[d];
      boxExtents[d].max = max[d];
      index[d] = 0;
      startOutsideRange = startOutsideRange || start[d] < min[d];
      startOutsideRange = startOutsideRange || start[d] > max[d];
    }

    // Start point
    VMD pos = start;
    coord_t thisX = 0;

    // If the start/end are outside the boundaries, then
    // move them to the edges
    if (startOutsideRange)
    {
      // Find the closest edge of the overall workspace
      coord_t bestDistance = 1e100;
      size_t closestDimension = 0;
      bool closestIsMin = false;
      findClosestEdgeDimension(boxExtents, pos, dirSign, nd, bestDistance, closestDimension, closestIsMin);
      coord_t movedBy = fabs(bestDistance / dir[closestDimension]);
      if (bestDistance > 1e99)
      {
        // Nothing is close = whole line is out of the extents!
        // Make a single bin with NAN
        x.push_back(0);  x.push_back(length);
        y.push_back(std::numeric_limits<double>::quiet_NaN());
        e.push_back(std::numeric_limits<double>::quiet_NaN());
        return;
      }
      // Move the point so it is at the edge instad.
      pos += dir * movedBy;
      thisX = movedBy;
    }

    // Initial index into each dimension
    for (size_t d=0; d<nd; d++)
    {
      index[d] = size_t((pos[d] - min[d]) / step[d]);
      if (index[d] >= this->getDimension(d)->getNBins())
      {
        // Whole line is out of the extents!
        // Make a single bin with NAN
        x.push_back(0);  x.push_back(length);
        y.push_back(std::numeric_limits<double>::quiet_NaN());
        e.push_back(std::numeric_limits<double>::quiet_NaN());
        return;
      }
    }

    // First boundary (is 0 unless the start is outside the range)
    x.push_back(thisX);

    // Set to true when we go outside the range of the workspace, to break out of while loop
    bool outside = false;

    // Keep adding points till the end of the line or going out of bounds.
    while (thisX < length && !outside)
    {
      // Find the extents of the current box
      for (size_t d=0; d<nd; d++)
      {
        boxExtents[d].min = double(index[d]) * step[d];
        boxExtents[d].max = boxExtents[d].min + step[d];
      }

      // Find the closest edge
      coord_t bestDistance = 1e100;
      size_t closestDimension = 0;
      bool closestIsMin = false;
      findClosestEdgeDimension(boxExtents, pos, dirSign, nd, bestDistance, closestDimension, closestIsMin);

      // Index into the array of this box
      size_t linearIndex = this->getLinearIndex(index);

      // This is the distance ALONG the line (not along the dimension alone)
      coord_t movedBy = fabs(bestDistance / dir[closestDimension]);

      // Is the edge of the box PAST the end of the line?
      if (movedBy > (length-thisX))
      {
        // Then go to the end
        outside = true;
        pos = end;
      }
      else
      {
        // Move to the next closest box
        if (closestIsMin)
        {
          // Decrease the index in that dimension
          if (index[closestDimension] == 0)
            outside = true;
          else
            index[closestDimension]--;
        }
        else
        {
          // Increase the index in that dimension
          index[closestDimension]++;
          // Gone outside the range?
          if (index[closestDimension] >= this->getDimension(closestDimension)->getNBins())
            outside = true;
        }
        // Move the point by the distance that makes it intersect the edge
        pos = pos + dir * movedBy;
      } // (moving to the edge of a box)

      // Position along the line
      thisX = coord_t((pos - start).norm());
      // Add it to the list
      x.push_back(thisX);

      // What is our normalization factor?
      signal_t normalizer = 1.0;
      switch (normalize)
      {
      case NoNormalization:
        break;
      case VolumeNormalization:
        normalizer = m_inverseVolume;
        break;
      case NumEventsNormalization:
        // TODO: Implement when we track # of events in MDHisto.
        normalizer = 1.0;
        break;
      }

      // And add the normalized signal/error to the list too
      y.push_back( this->getSignalAt(linearIndex) * normalizer );
      e.push_back( this->getErrorAt(linearIndex) * normalizer );

    } // (while thisX < length, i.e. while the line is being drawn)

    delete [] index;
    delete boxExtents;
  }


  //==============================================================================================
  //============================== ARITHMETIC OPERATIONS =========================================
  //==============================================================================================

  //----------------------------------------------------------------------------------------------
  /** Check if the two workspace's sizes match (for comparison or element-by-element operation
   *
   * @param other :: the workspace to compare to
   * @param operation :: descriptive string (for the error message)
   * @throw an error if they don't match
   */
  void MDHistoWorkspace::checkWorkspaceSize(const MDHistoWorkspace & other, std::string operation)
  {
    if (other.getNumDims() != this->getNumDims())
      throw std::invalid_argument("Cannot perform the " + operation + " operation on this MDHistoWorkspace. The number of dimensions does not match.");
    if (other.m_length != this->m_length)
      throw std::invalid_argument("Cannot perform the " + operation + " operation on this MDHistoWorkspace. The length of the signals vector does not match.");
  }


  //----------------------------------------------------------------------------------------------
  /** Perform the += operation, element-by-element, for two MDHistoWorkspace's
   *
   * @param b :: workspace on the RHS of the operation
   * @return *this after operation */
  MDHistoWorkspace & MDHistoWorkspace::operator+=(const MDHistoWorkspace & b)
  {
    add(b);
    return *this;
  }

  //----------------------------------------------------------------------------------------------
  /** Perform the += operation, element-by-element, for two MDHistoWorkspace's
   *
   * @param b :: workspace on the RHS of the operation
   * */
  void MDHistoWorkspace::add(const MDHistoWorkspace & b)
  {
    checkWorkspaceSize(b, "add");
    for (size_t i=0; i<m_length; ++i)
    {
      m_signals[i] += b.m_signals[i];
      m_errorsSquared[i] += b.m_errorsSquared[i];
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Perform the += operation with a scalar as the RHS argument
   *
   * @param signal :: signal to apply
   * @param error :: error (not squared) to apply
   * */
  void MDHistoWorkspace::add(const signal_t signal, const signal_t error)
  {
    signal_t errorSquared = error * error;
    for (size_t i=0; i<m_length; ++i)
    {
      m_signals[i] += signal;
      m_errorsSquared[i] += errorSquared;
    }
  }


  //----------------------------------------------------------------------------------------------
  /** Perform the -= operation, element-by-element, for two MDHistoWorkspace's
   *
   * @param b :: workspace on the RHS of the operation
   * @return *this after operation */
  MDHistoWorkspace & MDHistoWorkspace::operator-=(const MDHistoWorkspace & b)
  {
    subtract(b);
    return *this;
  }

  //----------------------------------------------------------------------------------------------
  /** Perform the -= operation, element-by-element, for two MDHistoWorkspace's
   *
   * @param b :: workspace on the RHS of the operation
   * */
  void MDHistoWorkspace::subtract(const MDHistoWorkspace & b)
  {
    checkWorkspaceSize(b, "subtract");
    for (size_t i=0; i<m_length; ++i)
    {
      m_signals[i] -= b.m_signals[i];
      m_errorsSquared[i] += b.m_errorsSquared[i];
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Perform the -= operation with a scalar as the RHS argument
   *
   * @param signal :: signal to apply
   * @param error :: error (not squared) to apply
   * */
  void MDHistoWorkspace::subtract(const signal_t signal, const signal_t error)
  {
    signal_t errorSquared = error * error;
    for (size_t i=0; i<m_length; ++i)
    {
      m_signals[i] -= signal;
      m_errorsSquared[i] += errorSquared;
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Perform the *= operation, element-by-element, for two MDHistoWorkspace's
   *
   * Error propagation of \f$ f = a * b \f$  is given by:
   * \f$ df^2 = f^2 * (da^2 / a^2 + db^2 / b^2) \f$
   *
   * @param b_ws :: workspace on the RHS of the operation
   * @return *this after operation */
  MDHistoWorkspace & MDHistoWorkspace::operator*=(const MDHistoWorkspace & b_ws)
  {
    multiply(b_ws);
    return *this;
  }

  //----------------------------------------------------------------------------------------------
  /** Perform the *= operation, element-by-element, for two MDHistoWorkspace's
   *
   * Error propagation of \f$ f = a * b \f$  is given by:
   * \f$ df^2 = f^2 * (da^2 / a^2 + db^2 / b^2) \f$
   *
   * @param b_ws :: workspace on the RHS of the operation
   * */
  void MDHistoWorkspace::multiply(const MDHistoWorkspace & b_ws)
  {
    checkWorkspaceSize(b_ws, "multiply");
    for (size_t i=0; i<m_length; ++i)
    {
      signal_t a = m_signals[i];
      signal_t da2 = m_errorsSquared[i];

      signal_t b = b_ws.m_signals[i];
      signal_t db2 = b_ws.m_errorsSquared[i];

      signal_t f = a * b;
      signal_t df2 = (f * f) * ( da2 / (a*a) + db2 / (b*b) );

      m_signals[i] = f;
      m_errorsSquared[i] = df2;
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Perform the *= operation with a scalar as the RHS argument
   *
   * Error propagation of \f$ f = a * b \f$  is given by:
   * \f$ df^2 = f^2 * (da^2 / a^2 + db^2 / b^2) \f$
   *
   * @param signal :: signal to apply
   * @param error :: error (not squared) to apply
   * @return *this after operation */
  void MDHistoWorkspace:: multiply(const signal_t signal, const signal_t error)
  {
    signal_t b = signal;
    signal_t db2 = error * error;
    signal_t db2_relative = db2 / (b*b);
    for (size_t i=0; i<m_length; ++i)
    {
      signal_t a = m_signals[i];
      signal_t da2 = m_errorsSquared[i];

      signal_t f = a * b;
      signal_t df2 = (f * f) * ( da2 / (a*a) + db2_relative );

      m_signals[i] = f;
      m_errorsSquared[i] = df2;
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Perform the /= operation, element-by-element, for two MDHistoWorkspace's
   *
   * Error propagation of \f$ f = a / b \f$  is given by:
   * \f$ df^2 = f^2 * (da^2 / a^2 + db^2 / b^2) \f$
   *
   * @param b_ws :: workspace on the RHS of the operation
   * @return *this after operation */
  MDHistoWorkspace & MDHistoWorkspace::operator/=(const MDHistoWorkspace & b_ws)
  {
    divide(b_ws);
    return *this;
  }


  //----------------------------------------------------------------------------------------------
  /** Perform the /= operation, element-by-element, for two MDHistoWorkspace's
   *
   * Error propagation of \f$ f = a / b \f$  is given by:
   * \f$ df^2 = f^2 * (da^2 / a^2 + db^2 / b^2) \f$
   *
   * @param b_ws :: workspace on the RHS of the operation
   **/
  void MDHistoWorkspace::divide(const MDHistoWorkspace & b_ws)
  {
    checkWorkspaceSize(b_ws, "divide");
    for (size_t i=0; i<m_length; ++i)
    {
      signal_t a = m_signals[i];
      signal_t da2 = m_errorsSquared[i];

      signal_t b = b_ws.m_signals[i];
      signal_t db2 = b_ws.m_errorsSquared[i];

      signal_t f = a / b;
      signal_t df2 = (f * f) * ( da2 / (a*a) + db2 / (b*b) );

      m_signals[i] = f;
      m_errorsSquared[i] = df2;
    }
  }



  //----------------------------------------------------------------------------------------------
  /** Perform the /= operation with a scalar as the RHS argument
   *
   * Error propagation of \f$ f = a / b \f$  is given by:
   * \f$ df^2 = f^2 * (da^2 / a^2 + db^2 / b^2) \f$
   *
   * @param signal :: signal to apply
   * @param error :: error (not squared) to apply
   **/
  void MDHistoWorkspace::divide(const signal_t signal, const signal_t error)
  {
    signal_t b = signal;
    signal_t db2 = error * error;
    signal_t db2_relative = db2 / (b*b);
    for (size_t i=0; i<m_length; ++i)
    {
      signal_t a = m_signals[i];
      signal_t da2 = m_errorsSquared[i];

      signal_t f = a / b;
      signal_t df2 = (f * f) * ( da2 / (a*a) + db2_relative );

      m_signals[i] = f;
      m_errorsSquared[i] = df2;
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Perform the natural logarithm on each signal in the workspace.
   *
   * Error propagation of \f$ f = ln(a) \f$  is given by:
   * \f$ df^2 = a^2 / da^2 \f$
   */
  void MDHistoWorkspace::log(double filler)
  {
    for (size_t i=0; i<m_length; ++i)
    {
      signal_t a = m_signals[i];
      signal_t da2 = m_errorsSquared[i];
      if (a <= 0)
      {
        m_signals[i] = filler;
        m_errorsSquared[i] = 0;
      }
      else
      {
        m_signals[i] = std::log(a);
        m_errorsSquared[i] = da2 / (a*a);
      }
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Perform the base-10 logarithm on each signal in the workspace.
   *
   * Error propagation of \f$ f = ln(a) \f$  is given by:
   * \f$ df^2 = (ln(10)^-2) * a^2 / da^2 \f$
   */
  void MDHistoWorkspace::log10(double filler)
  {
    for (size_t i=0; i<m_length; ++i)
    {
      signal_t a = m_signals[i];
      signal_t da2 = m_errorsSquared[i];
      if (a <= 0)
      {
        m_signals[i] = filler;
        m_errorsSquared[i] = 0;
      }
      else
      {
        m_signals[i] = std::log10(a);
        m_errorsSquared[i] = 0.1886117 * da2 / (a*a); // 0.1886117  = ln(10)^-2
      }
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Perform the exp() function on each signal in the workspace.
   *
   * Error propagation of \f$ f = exp(a) \f$  is given by:
   * \f$ df^2 = f^2 * da^2 \f$
   */
  void MDHistoWorkspace::exp()
  {
    for (size_t i=0; i<m_length; ++i)
    {
      signal_t f = std::exp(m_signals[i]);
      signal_t da2 = m_errorsSquared[i];
      m_signals[i] = f;
      m_errorsSquared[i] = f*f * da2;
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Perform the power function (signal^exponent) on each signal S in the workspace.
   *
   * Error propagation of \f$ f = a^b \f$  is given by:
   * \f$ df^2 = f^2 * b^2 * (da^2 / a^2) \f$
   */
  void MDHistoWorkspace::power(double exponent)
  {
    double exponent_squared = exponent * exponent;
    for (size_t i=0; i<m_length; ++i)
    {
      signal_t a = m_signals[i];
      signal_t f = std::pow(a, exponent);
      signal_t da2 = m_errorsSquared[i];
      m_signals[i] = f;
      m_errorsSquared[i] = f*f * exponent_squared * da2 / (a*a);
    }
  }

  //==============================================================================================
  //============================== BOOLEAN OPERATIONS ============================================
  //==============================================================================================

  //----------------------------------------------------------------------------------------------
  /** A boolean &= (and) operation, element-by-element, for two MDHistoWorkspace's.
   *
   * 0.0 is "false", all other values are "true". All errors are set to 0.
   *
   * @param b :: workspace on the RHS of the operation
   * @return *this after operation */
  MDHistoWorkspace & MDHistoWorkspace::operator&=(const MDHistoWorkspace & b)
  {
    checkWorkspaceSize(b, "&= (and)");
    for (size_t i=0; i<m_length; ++i)
    {
      m_signals[i] = ((m_signals[i] != 0) && (b.m_signals[i] != 0)) ? 1.0 : 0.0;
      m_errorsSquared[i] = 0;
    }
    return *this;
  }

  //----------------------------------------------------------------------------------------------
  /** A boolean |= (or) operation, element-by-element, for two MDHistoWorkspace's.
   *
   * 0.0 is "false", all other values are "true". All errors are set to 0.
   *
   * @param b :: workspace on the RHS of the operation
   * @return *this after operation */
  MDHistoWorkspace & MDHistoWorkspace::operator|=(const MDHistoWorkspace & b)
  {
    checkWorkspaceSize(b, "|= (or)");
    for (size_t i=0; i<m_length; ++i)
    {
      m_signals[i] = ((m_signals[i] != 0) || (b.m_signals[i] != 0)) ? 1.0 : 0.0;
      m_errorsSquared[i] = 0;
    }
    return *this;
  }

  //----------------------------------------------------------------------------------------------
  /** A boolean ^= (xor) operation, element-by-element, for two MDHistoWorkspace's.
   *
   * 0.0 is "false", all other values are "true". All errors are set to 0.
   *
   * @param b :: workspace on the RHS of the operation
   * @return *this after operation */
  MDHistoWorkspace & MDHistoWorkspace::operator^=(const MDHistoWorkspace & b)
  {
    checkWorkspaceSize(b, "^= (xor)");
    for (size_t i=0; i<m_length; ++i)
    {
      m_signals[i] = ((m_signals[i] != 0) ^ (b.m_signals[i] != 0)) ? 1.0 : 0.0;
      m_errorsSquared[i] = 0;
    }
    return *this;
  }

  //----------------------------------------------------------------------------------------------
  /** A boolean not operation, performed in-place.
   * All errors are set to 0.
   *
   * 0.0 is "false", all other values are "true". All errors are set to 0.
   */
  void MDHistoWorkspace::operatorNot()
  {
    for (size_t i=0; i<m_length; ++i)
    {
      m_signals[i] = (m_signals[i] == 0.0);
      m_errorsSquared[i] = 0;
    }
  }


  //----------------------------------------------------------------------------------------------
  /** Turn this workspace into a boolean workspace, where
   * signal[i] -> becomes true (1.0) if it is < b[i].
   * signal[i] -> becomes false (0.0) otherwise
   * Errors are set to 0.
   *
   * @param b :: workspace on the RHS of the comparison.
   */
  void MDHistoWorkspace::lessThan(const MDHistoWorkspace & b)
  {
    checkWorkspaceSize(b, "lessThan");
    for (size_t i=0; i<m_length; ++i)
    {
      m_signals[i] = (m_signals[i] < b.m_signals[i]) ? 1.0 : 0.0;
      m_errorsSquared[i] = 0;
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Turn this workspace into a boolean workspace, where
   * signal[i] -> becomes true (1.0) if it is < signal.
   * signal[i] -> becomes false (0.0) otherwise
   * Errors are set to 0.
   *
   * @param signal :: signal value on the RHS of the comparison.
   */
  void MDHistoWorkspace::lessThan(const signal_t signal)
  {
    for (size_t i=0; i<m_length; ++i)
    {
      m_signals[i] = (m_signals[i] < signal) ? 1.0 : 0.0;
      m_errorsSquared[i] = 0;
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Turn this workspace into a boolean workspace, where
   * signal[i] -> becomes true (1.0) if it is > b[i].
   * signal[i] -> becomes false (0.0) otherwise
   * Errors are set to 0.
   *
   * @param b :: workspace on the RHS of the comparison.
   */
  void MDHistoWorkspace::greaterThan(const MDHistoWorkspace & b)
  {
    checkWorkspaceSize(b, "greaterThan");
    for (size_t i=0; i<m_length; ++i)
    {
      m_signals[i] = (m_signals[i] > b.m_signals[i]) ? 1.0 : 0.0;
      m_errorsSquared[i] = 0;
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Turn this workspace into a boolean workspace, where
   * signal[i] -> becomes true (1.0) if it is > signal.
   * signal[i] -> becomes false (0.0) otherwise
   * Errors are set to 0.
   *
   * @param signal :: signal value on the RHS of the comparison.
   */
  void MDHistoWorkspace::greaterThan(const signal_t signal)
  {
    for (size_t i=0; i<m_length; ++i)
    {
      m_signals[i] = (m_signals[i] > signal) ? 1.0 : 0.0;
      m_errorsSquared[i] = 0;
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Turn this workspace into a boolean workspace, where
   * signal[i] -> becomes true (1.0) if it is == b[i].
   * signal[i] -> becomes false (0.0) otherwise
   * Errors are set to 0.
   *
   * @param b :: workspace on the RHS of the comparison.
   * @param tolerance :: accept this deviation from a perfect equality
   */
  void MDHistoWorkspace::equalTo(const MDHistoWorkspace & b, const signal_t tolerance)
  {
    checkWorkspaceSize(b, "equalTo");
    for (size_t i=0; i<m_length; ++i)
    {
      signal_t diff = fabs(m_signals[i] - b.m_signals[i]);
      m_signals[i] = (diff < tolerance) ? 1.0 : 0.0;
      m_errorsSquared[i] = 0;
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Turn this workspace into a boolean workspace, where
   * signal[i] -> becomes true (1.0) if it is == signal.
   * signal[i] -> becomes false (0.0) otherwise
   * Errors are set to 0.
   *
   * @param signal :: signal value on the RHS of the comparison.
   * @param tolerance :: accept this deviation from a perfect equality
   */
  void MDHistoWorkspace::equalTo(const signal_t signal, const signal_t tolerance)
  {
    for (size_t i=0; i<m_length; ++i)
    {
      signal_t diff = fabs(m_signals[i] - signal);
      m_signals[i] = (diff < tolerance) ? 1.0 : 0.0;
      m_errorsSquared[i] = 0;
    }
  }


  //----------------------------------------------------------------------------------------------
  /** Copy the values from another workspace onto this workspace, but only
   * where a mask is true (non-zero)
   *
   * For example, in matlab or numpy python, you might write something like:
   *  "mask = (array < 5.0); array[mask] = other[mask];"
   *
   * The equivalent here is:
   *  mask = array;
   *  mask.lessThan(5.0);
   *  array.setUsingMask(mask, other);
   *
   * @param mask :: MDHistoWorkspace where (signal == 0.0) means false, and (signal != 0.0) means true.
   * @param values :: MDHistoWorkspace of values to copy.
   */
  void MDHistoWorkspace::setUsingMask(const MDHistoWorkspace & mask, const MDHistoWorkspace & values)
  {
    checkWorkspaceSize(mask, "setUsingMask");
    checkWorkspaceSize(values, "setUsingMask");
    for (size_t i=0; i<m_length; ++i)
    {
      if (mask.m_signals[i] != 0.0)
      {
        m_signals[i] = values.m_signals[i];
        m_errorsSquared[i] = values.m_errorsSquared[i];
      }
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Copy the values from another workspace onto this workspace, but only
   * where a mask is true (non-zero)
   *
   * For example, in matlab or numpy python, you might write something like:
   *  "mask = (array < 5.0); array[mask] = other[mask];"
   *
   * The equivalent here is:
   *  mask = array;
   *  mask.lessThan(5.0);
   *  array.setUsingMask(mask, other);
   *
   * @param mask :: MDHistoWorkspace where (signal == 0.0) means false, and (signal != 0.0) means true.
   * @param signal :: signal to set everywhere mask is true
   * @param error :: error (not squared) to set everywhere mask is true
   */
  void MDHistoWorkspace::setUsingMask(const MDHistoWorkspace & mask, const signal_t signal, const signal_t error)
  {
    signal_t errorSquared = error * error;
    checkWorkspaceSize(mask, "setUsingMask");
    for (size_t i=0; i<m_length; ++i)
    {
      if (mask.m_signals[i] != 0.0)
      {
        m_signals[i] = signal;
        m_errorsSquared[i] = errorSquared;
      }
    }
  }


} // namespace Mantid
} // namespace MDEvents

