#include "MantidMDEvents/IMDBox.h"
#include "MantidKernel/System.h"
#include "MantidNexus/NeXusFile.hpp"

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
      m_depth(0)
  {
    // Clear the running total of the centroid
    for (size_t d=0; d<nd; d++)
      m_centroid[d] = 0;
  }

  //-----------------------------------------------------------------------------------------------
  /** Copy constructor. Copies the extents, depth, etc.
   * and recalculates the boxes' volume.
   * @param box :: incoming box to copy.
   */
  TMDE(
  IMDBox)::IMDBox(IMDBox<MDE,nd> * box)
  : m_signal(box->getSignal()), m_errorSquared(box->getErrorSquared()),
    m_inverseVolume(box->m_inverseVolume), m_depth(box->getDepth())
  {
    if (!box)
      throw std::runtime_error("IMDBox::ctor(): box is NULL.");
    // Save the controller in this object.
    this->m_BoxController = box->m_BoxController;
    // Copy the extents
    for (size_t d=0; d<nd; d++)
      this->extents[d] = box->extents[d];
    // Copy the depth
    this->m_depth = box->getDepth();
    // Clear the running total of the centroid
    for (size_t d=0; d<nd; d++)
      m_centroid[d] = 0;
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
    for (; it != it_end; it++)
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

  //-----------------------------------------------------------------------------------------------
  /** Return the vertices of corners of the box
   *
   * @return a vector of Coordinate objects
   */
  TMDE(
  std::vector<Mantid::Geometry::Coordinate> IMDBox)::getVertexes() const
  {
    if (nd > 4)
      throw std::runtime_error("IMDBox::getVertexes(): At this time, cannot return vertexes for > 4 dimensions.");
    std::vector<Mantid::Geometry::Coordinate> out;

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
      out.push_back( Mantid::Geometry::Coordinate(coords, nd) );
    }

    return out;
  }




} // namespace Mantid
} // namespace MDEvents

