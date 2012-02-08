#include "MantidMDEvents/IMDBox.h"
#include "MantidKernel/System.h"
#include "MantidNexusCPP/NeXusFile.hpp"
#include "MantidKernel/VMD.h"

using NeXus::File;

namespace Mantid
{
namespace MDEvents
{


  //-----------------------------------------------------------------------------------------------
  /** Default constructor.
   */
  TMDE(
  IMDBox)::IMDBox()
    : m_signal(0.0), m_errorSquared(0.0),
      m_inverseVolume(1.0),
      m_depth(0),
      m_parent(NULL)
  {
#ifdef MDBOX_TRACK_CENTROID
    // Clear the running total of the centroid
    for (size_t d=0; d<nd; d++)
      m_centroid[d] = 0;
#endif
  }


  //-----------------------------------------------------------------------------------------------
  /** Constructor with extents
   */
  TMDE(
  IMDBox)::IMDBox(const std::vector<Mantid::Geometry::MDDimensionExtents> & extentsVector)
    : m_signal(0.0), m_errorSquared(0.0),
      m_inverseVolume(1.0),
      m_depth(0),
      m_parent(NULL)
  {
#ifdef MDBOX_TRACK_CENTROID
    // Clear the running total of the centroid
    for (size_t d=0; d<nd; d++)
      m_centroid[d] = 0;
#endif
    // Set the extents
    if (extentsVector.size() != nd) throw std::invalid_argument("IMDBox::ctor(): extentsVector.size() must be == nd.");
    for (size_t d=0; d<nd; d++)
      this->extents[d] = extentsVector[d];
  }


  //-----------------------------------------------------------------------------------------------
  /** Copy constructor. Copies the extents, depth, etc.
   * and recalculates the boxes' volume.
   * @param box :: incoming box to copy.
   */
  TMDE(
  IMDBox)::IMDBox(const IMDBox<MDE,nd> & box)
  : ISaveable(box),
    m_signal(box.m_signal), m_errorSquared(box.m_errorSquared),
    m_inverseVolume(box.m_inverseVolume), m_depth(box.m_depth),
    m_parent(box.m_parent)
  {
    // Save the controller in this object.
    this->m_BoxController = box.m_BoxController;
    // Copy the extents
    for (size_t d=0; d<nd; d++)
      this->extents[d] = box.extents[d];
    // Copy the depth
    this->m_depth = box.getDepth();
#ifdef MDBOX_TRACK_CENTROID
    // Clear the running total of the centroid
    for (size_t d=0; d<nd; d++)
      m_centroid[d] = 0;
#endif
    // Re-calculate the volume of the box
    this->calcVolume(); //TODO: Is this necessary or should we copy the volume?

  }

  //-----------------------------------------------------------------------------------------------
  /** Add several events, starting and stopping at particular point in a vector.
   * Bounds checking IS performed, and events outside the range are rejected.
   *
   * NOTE: You must call refreshCache() after you are done, to calculate the
   *  nPoints, signal and error.
   *
   * @param events :: vector of events to be copied.
   * @param start_at :: begin at this index in the array
   * @param stop_at :: stop at this index in the array
   * @return the number of events that were rejected (because of being out of bounds)
   */
  TMDE(
  size_t IMDBox)::addEvents(const std::vector<MDE> & events, const size_t start_at, const size_t stop_at)
  {
    size_t numBad = 0;
    // --- Go event by event and add them ----
    typename std::vector<MDE>::const_iterator it = events.begin() + start_at;
    typename std::vector<MDE>::const_iterator it_end = events.begin() + stop_at;
    for (; it != it_end; ++it)
    {
      //Check out-of-bounds-ness
      bool badEvent = false;
      for (size_t d=0; d<nd; d++)
      {
        double x = it->getCenter(d);
        if ((x < this->extents[d].min) || (x >= this->extents[d].max))
        {
          badEvent = true;
          break;
        }
      }

      if (badEvent)
        // Event was out of bounds. Count it
        ++numBad;
      else
        // Event was in bounds; add it
        addEvent(*it);
    }

    return numBad;
  }

  //---------------------------------------------------------------------------------------------------
  /** Transform the dimensions contained in this box
   * x' = x*scaling + offset
   *
   * @param scaling :: multiply each coordinate by this value.
   * @param offset :: after multiplying, add this offset.
   */
  TMDE(
  void IMDBox)::transformDimensions(std::vector<double> & scaling, std::vector<double> & offset)
  {
    for (size_t d=0; d<nd; d++)
    {
      extents[d].min = (extents[d].min * scaling[d]) + offset[d];
      extents[d].max = (extents[d].max * scaling[d]) + offset[d];
    }
    // Re-calculate the volume of the box
    this->calcVolume();
  }


  //-----------------------------------------------------------------------------------------------
  /** Return the vertices of corners of the box
   *
   * @return a vector of VMD objects
   */
  TMDE(
  std::vector<Mantid::Kernel::VMD> IMDBox)::getVertexes() const
  {
    if (nd > 4)
      throw std::runtime_error("IMDBox::getVertexes(): At this time, cannot return vertexes for > 4 dimensions.");
    std::vector<Mantid::Kernel::VMD> out;

    // How many vertices does one box have? 2^nd, or bitwise shift left 1 by nd bits
    size_t maxVertices = 1 << nd;

    // For each vertex, increase an integeer
    for (size_t i=0; i<maxVertices; ++i)
    {
      // Coordinates of the vertex
      coord_t coords[nd];
      for (size_t d=0; d<nd; d++)
      {
        // Use a bit mask to look at each bit of the integer we are iterating through.
        size_t mask = 1 << d;
        if ((i & mask) > 0)
        {
          // Bit is 1, use the max of the dimension
          coords[d] = extents[d].max;
        }
        else
        {
          // Bit is 0, use the min of the dimension
          coords[d] = extents[d].min;
        }
      } // (for each dimension)

      // Create the coordinate object and add it to the vector
      out.push_back( Mantid::Kernel::VMD(nd, coords) );
    }

    return out;
  }


  //-----------------------------------------------------------------------------------------------
  /** Return the vertices of every corner of the box, but as
   * a bare array of length numVertices * nd
   *
   * @param[out] numVertices :: returns the number of vertices in the array.
   * @return the bare array. This should be deleted by the caller!
   * */
  TMDE(
  coord_t * IMDBox)::getVertexesArray(size_t & numVertices) const
  {
    // How many vertices does one box have? 2^nd, or bitwise shift left 1 by nd bits
    numVertices = 1 << nd;

    // Allocate the array of the right size
    coord_t * out = new coord_t[nd * numVertices];

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
          out[outIndex + d] = extents[d].max;
        }
        else
        {
          // Bit is 0, use the min of the dimension
          out[outIndex + d] = extents[d].min;
        }
      } // (for each dimension)
    }
    return out;
  }


  //-----------------------------------------------------------------------------------------------
  /** Return the vertices of every corner of the box, but to a reduced
   * number of dimensions.
   *
   * Essentially, this is a projection of the vertexes from nd down to a lower
   * (or equal) number of dimensions. Redundant vertexes are NOT included.
   *
   * @param[out] numVertices :: returns the number of vertices in the array.
   * @param outDimensions :: the number of dimensions in the output array (must be > 0).
   * @param maskDim :: nd-sized array of a mask, with TRUE for the dimensions to KEEP.
   *        The number of TRUEs in maskDim MUST be equal to outDimensions, and this is
   *        not checked for here (for performance).
   * @return the bare array, size of outDimensions. This should be deleted by the caller!
   * @throw if outDimensions == 0
   * */
  TMDE(
  coord_t * IMDBox)::getVertexesArray(size_t & numVertices, const size_t outDimensions, const bool * maskDim) const
  {
    if (outDimensions == 0)
      throw std::invalid_argument("IMDBox::getVertexesArray(): Must have > 0 output dimensions.");

    // How many vertices does one box have? 2^numOutputDimensions
    numVertices = (size_t)1 << outDimensions;

    // Allocate the array of the right size
    coord_t * out = new coord_t[outDimensions * numVertices];

    // For each vertex, increase an integeer
    for (size_t i=0; i<numVertices; ++i)
    {
      // Start point index in the output array;
      size_t outIndex = i * outDimensions;

      // The OUTPUT dimension index
      size_t outd = 0;

      // Coordinates of the vertex
      for (size_t ind=0; ind<nd; ind++)
      {
        if (maskDim[ind])
        {
          // Use a bit mask to look at each bit of the integer we are iterating through.
          size_t mask = (size_t)1 << outd;
          if ((i & mask) > 0)
          {
            // Bit is 1, use the max of the dimension
            out[outIndex + outd] = extents[ind].max;
          }
          else
          {
            // Bit is 0, use the min of the dimension
            out[outIndex + outd] = extents[ind].min;
          }
          outd++;
        } // the dimensions is used in the output.
      } // (for each INPUT dimension)
    }
    return out;
  }





  //-----------------------------------------------------------------------------------------------
  /** Helper method for sorting IMDBoxes by file position.
   * MDGridBoxes return 0 for file position and so aren't sorted.
   *
   * @param a :: an IMDBox pointer
   * @param b :: an IMDBox pointer
   * @return
   */
  template<typename MDE, size_t nd>
  bool CompareMDBoxFilePosition (const IMDBox<MDE,nd> * a, const IMDBox<MDE,nd> * b)
  {
    return (a->getFilePosition() < b->getFilePosition());
  }

  //-----------------------------------------------------------------------------------------------
  /** Static method for sorting a list of IMDBox pointers by their file position,
   * ascending. This should optimize the speed of loading a bit by
   * reducing the amount of disk seeking.
   *
   * @param boxes :: ref to a vector of boxes. It will be sorted in-place.
   */
  TMDE(
  void IMDBox)::sortBoxesByFilePos(std::vector<IMDBox<MDE,nd> *> & boxes)
  {
    std::sort( boxes.begin(), boxes.end(), CompareMDBoxFilePosition<MDE,nd>);
  }



} // namespace Mantid
} // namespace MDEvents

