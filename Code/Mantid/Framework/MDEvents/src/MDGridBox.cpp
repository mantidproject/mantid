#include "MantidKernel/Task.h"
#include "MantidKernel/Utils.h"
#include "MantidKernel/FunctionTask.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/ThreadPool.h"
#include "MantidKernel/ThreadScheduler.h"
#include "MantidKernel/ThreadSchedulerMutexes.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDLeanEvent.h"
#include "MantidMDEvents/MDGridBox.h"
#include <ostream>
#include "MantidKernel/Strings.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;

// These pragmas ignores the warning in the ctor where "d<nd-1" for nd=1.
// This is okay (though would be better if it were for only that function
#if (defined(__INTEL_COMPILER))
#pragma warning disable 186
#elif defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wtype-limits"
#endif


namespace Mantid
{
namespace MDEvents
{

  //===============================================================================================
  //===============================================================================================
  //-----------------------------------------------------------------------------------------------
  /** Empty constructor. Used when loading from NXS files.
   * */
  TMDE(MDGridBox)::MDGridBox()
   : MDBoxBase<MDE, nd>(), numBoxes(0), nPoints(0)
  {
  }

  //-----------------------------------------------------------------------------------------------
  /** Constructor with a box controller.
   * @param bc :: BoxController
   * @param depth :: recursive split depth
   * @param extentsVector :: size of the box
   */
  TMDE(MDGridBox)::MDGridBox(BoxController_sptr bc, const size_t depth, const std::vector<Mantid::Geometry::MDDimensionExtents> & extentsVector)
   : MDBoxBase<MDE, nd>(extentsVector),
     numBoxes(0), nPoints(0)
  {
    this->m_depth = depth;
    if (!bc)
      throw std::runtime_error("MDGridBox::ctor(): No BoxController specified in box.");
    this->m_BoxController = bc;

    // How many is it split?
    for (size_t d=0; d<nd; d++)
      split[d] = this->m_BoxController->getSplitInto(d);

    // Compute sizes etc.
    size_t tot = computeSizesFromSplit();
    if (tot == 0)
      throw std::runtime_error("MDGridBox::ctor(): Invalid splitting criterion (one was zero).");
  }


  //-----------------------------------------------------------------------------------------------
  /** Constructor
   * @param box :: MDBox containing the events to split */
  TMDE(MDGridBox)::MDGridBox(MDBox<MDE, nd> * box)
   : MDBoxBase<MDE, nd>(*box),
     nPoints(0)
  {
    BoxController_sptr bc = box->getBoxController();
    if (!bc)
      throw std::runtime_error("MDGridBox::ctor(): No BoxController specified in box.");

//    std::cout << "Splitting MDBox ID " << box->getId() << " with " << box->getNPoints() << " events into MDGridBox" << std::endl;

    // Steal the ID from the parent box that is being split.
    this->setId( box->getId() );

    // How many is it split?
    for (size_t d=0; d<nd; d++)
      split[d] = bc->getSplitInto(d);

    // Compute sizes etc.
    size_t tot = computeSizesFromSplit();
    if (tot == 0)
      throw std::runtime_error("MDGridBox::ctor(): Invalid splitting criterion (one was zero).");

    // Calculate the volume
    coord_t volume = 1;
    for (size_t d=0; d<nd; d++)
      volume *= boxSize[d];
    coord_t inverseVolume = 1.0f / volume;

    // Create the array of MDBox contents.
    boxes.clear();
    boxes.reserve(tot);
    numBoxes = tot;

    size_t indices[nd];
    for (size_t d=0; d<nd; d++) indices[d] = 0;

    // Splitting an input MDBox requires creating a bunch of children
    // But the IDs of these children MUST be sequential. Hence the critical block
    // to avoid interleaved IDs when splitting boxes in parallel.
    bc->getIdMutex().lock();
    {
      for (size_t i=0; i<tot; i++)
      {
        // Create the box
        // (Increase the depth of this box to one more than the parent (this))
        MDBox<MDE,nd> * myBox = new MDBox<MDE,nd>(bc, this->m_depth + 1);
        // This MDGridBox is the parent of the new child.
        myBox->setParent(this);

        // Set the extents of this box.
        for (size_t d=0; d<nd; d++)
        {
          coord_t min = this->extents[d].min + boxSize[d] * static_cast<coord_t>(indices[d]);
          myBox->setExtents(d, min, min + boxSize[d]);
        }
        myBox->setInverseVolume(inverseVolume); // Set the cached inverse volume
        boxes.push_back(myBox);

        // Increment the indices, rolling back as needed
        indices[0]++;
        for (size_t d=0; d<nd-1; d++) //This is not run if nd=1; that's okay, you can ignore the warning
        {
          if (indices[d] >= split[d])
          {
            indices[d] = 0;
            indices[d+1]++;
          }
        }
      } // for each box
    }
    bc->getIdMutex().unlock();

    // Now distribute the events that were in the box before
    const std::vector<MDE> & events = box->getConstEvents();

    // Add all the events, with no bounds checking
    typename std::vector<MDE>::const_iterator it = events.begin();
    typename std::vector<MDE>::const_iterator it_end = events.end();
    for (; it != it_end; ++it)
        addEvent(*it);

    // Copy the cached numbers from the incoming box. This is quick - don't need to refresh cache
    this->nPoints = box->getNPoints();

    // Clear the old box.
    box->clear();
    box->setDataBusy(false);
  }


  //-----------------------------------------------------------------------------------------------
  /** Copy constructor
   * @param other :: MDGridBox to copy */
  TMDE(MDGridBox)::MDGridBox(const MDGridBox<MDE, nd> & other)
   : MDBoxBase<MDE, nd>(other),
     numBoxes(other.numBoxes),
     diagonalSquared(other.diagonalSquared),
     nPoints(other.nPoints)
  {
    for (size_t d=0; d<nd; d++)
    {
      split[d] = other.split[d];
      splitCumul[d] = other.splitCumul[d];
      boxSize[d] = other.boxSize[d];
    }
    // Copy all the boxes
    boxes.clear();
    for (size_t i=0; i<other.boxes.size(); i++)
    {
      MDBoxBase<MDE, nd>* otherBox = other.boxes[i];
      const MDBox<MDE, nd>* otherMDBox = dynamic_cast<const MDBox<MDE, nd>* >(otherBox);
      const MDGridBox<MDE, nd>* otherMDGridBox = dynamic_cast<const MDGridBox<MDE, nd>* >(otherBox);
      if (otherMDBox)
      {
        MDBox<MDE, nd> * newBox = new MDBox<MDE, nd>(*otherMDBox);
        newBox->setParent(this);
        boxes.push_back( newBox );
      }
      else if (otherMDGridBox)
      {
        MDGridBox<MDE, nd> * newBox = new MDGridBox<MDE, nd>(*otherMDGridBox);
        newBox->setParent(this);
        boxes.push_back( newBox );
      }
      else
      {
        throw std::runtime_error("MDGridBox::copy_ctor(): an unexpected child box type was found.");
      }
    }
  }

  //-----------------------------------------------------------------------------------------------
  /** Transform the dimensions contained in this box
   * x' = x*scaling + offset.
   * NON-RECURSIVE!
   *
   * @param scaling :: multiply each coordinate by this value.
   * @param offset :: after multiplying, add this offset.
   */
  TMDE(
  void MDGridBox)::transformDimensions(std::vector<double> & scaling, std::vector<double> & offset)
  {
    MDBoxBase<MDE,nd>::transformDimensions(scaling, offset);
    this->computeSizesFromSplit();
  }

  //-----------------------------------------------------------------------------------------------
  /** Compute some data from the split[] array and the extents.
   *
   * @return :: the total number of boxes */
  TMDE(
  size_t MDGridBox)::computeSizesFromSplit()
  {
    // Do some computation based on how many splits per each dim.
    size_t tot = 1;
    diagonalSquared = 0;
    for (size_t d=0; d<nd; d++)
    {
      // Cumulative multiplier, for indexing
      splitCumul[d] = tot;
      tot *= split[d];
      // Length of the side of a box in this dimension
      boxSize[d] = (this->extents[d].max - this->extents[d].min) / static_cast<coord_t>(split[d]);
      // Accumulate the squared diagonal length.
      diagonalSquared += boxSize[d] * boxSize[d];
    }

    return tot;

  }

  //-----------------------------------------------------------------------------------------------
  /// Destructor
  TMDE(MDGridBox)::~MDGridBox()
  {
    // Delete all contained boxes (this should fire the MDGridBox destructors recursively).
    typename boxVector_t::iterator it;
    for (it = boxes.begin(); it != boxes.end(); ++it)
      delete *it;
    boxes.clear();
  }


  //-----------------------------------------------------------------------------------------------
  /** Clear any points contained. */
  TMDE(
  void MDGridBox)::clear()
  {
    this->m_signal = 0.0;
    this->m_errorSquared = 0.0;
    typename boxVector_t::iterator it;
    for (it = boxes.begin(); it != boxes.end(); ++it)
    {
      (*it)->clear();
    }
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the number of dimensions in this box */
  TMDE(
  size_t MDGridBox)::getNumDims() const
  {
    return nd;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the total number of points (events) in this box */
  TMDE(uint64_t MDGridBox)::getNPoints() const
  {
    //Use the cached value
    return nPoints;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the number of un-split MDBoxes in this box (recursively including all children)
   * @return :: the total # of MDBoxes in all children */
  TMDE(
  size_t MDGridBox)::getNumMDBoxes() const
  {
    size_t total = 0;
    typename boxVector_t::const_iterator it;
    for (it = boxes.begin(); it != boxes.end(); ++it)
    {
      total += (*it)->getNumMDBoxes();
    }
    return total;
  }


  //-----------------------------------------------------------------------------------------------
  /** @return The number of children of the MDGridBox, not recursively
   */
  TMDE(
  size_t MDGridBox)::getNumChildren() const
  {
    return numBoxes;
  }

  //-----------------------------------------------------------------------------------------------
  /** Get a child box
   * @param index :: index into the array, within range 0..getNumChildren()-1
   * @return the child MDBoxBase pointer.
   */
  template <typename MDE, size_t nd>
  MDBoxBase<MDE,nd> * MDGridBox<MDE,nd>::getChild(size_t index)
  {
    return boxes[index];
  }

  //-----------------------------------------------------------------------------------------------
  /** Directly set the children of the MDGridBox. Used in file loading.
   * Should not be called on a box with children; the existing children are NOT deleted.
   *
   * @param otherBoxes:: reference to a vector of boxes containing the children
   * @param indexStart :: start point in the vector
   * @param indexEnd :: end point in the vector, not-inclusive
   */
  TMDE(
  void MDGridBox)::setChildren(const std::vector<MDBoxBase<MDE,nd> *> & otherBoxes, const size_t indexStart, const size_t indexEnd)
  {
    boxes.clear();
    boxes.assign( otherBoxes.begin()+indexStart, otherBoxes.begin()+indexEnd);
    // Set the parent of each new child box.
    for (size_t i=0; i<boxes.size(); i++)
      boxes[i]->setParent(this);
    numBoxes = boxes.size();
  }


  //-----------------------------------------------------------------------------------------------
  /** Helper function to get the index into the linear array given
   * an array of indices for each dimension (0 to nd)
   * @param indices :: array of size[nd]
   * @return size_t index into boxes[].
   */
  TMDE(
  inline size_t MDGridBox)::getLinearIndex(size_t * indices) const
  {
    size_t out_linear_index = 0;
    for (size_t d=0; d<nd; d++)
      out_linear_index += (indices[d] * splitCumul[d]);
    return out_linear_index;
  }


  //-----------------------------------------------------------------------------------------------
  /** Refresh the cache of nPoints, signal and error,
   * by adding up all boxes (recursively).
   * MDBoxes' totals are used directly.
   *
   * @param ts :: ThreadScheduler pointer to perform the caching
   *  in parallel. If NULL, it will be performed in series.
   */
  TMDE(
  void MDGridBox)::refreshCache(ThreadScheduler * ts)
  {
    // Clear your total
    nPoints = 0;
    this->m_signal = 0;
    this->m_errorSquared = 0;
    this->m_totalWeight = 0;

#ifdef MDBOX_TRACK_CENTROID
    for (size_t d=0; d<nd; d++)
      this->m_centroid[d] = 0;
#endif

    typename boxVector_t::iterator it;
    typename boxVector_t::iterator it_end = boxes.end();

    if (!ts)
    {
      //--------- Serial -----------
      for (it = boxes.begin(); it != it_end; ++it)
      {
        MDBoxBase<MDE,nd> * ibox = *it;

        // Refresh the cache (does nothing for MDBox)
        ibox->refreshCache();

        // Add up what's in there
        nPoints += ibox->getNPoints();
        this->m_signal += ibox->getSignal();
        this->m_errorSquared += ibox->getErrorSquared();
        this->m_totalWeight += ibox->getTotalWeight();

#ifdef MDBOX_TRACK_CENTROID
        // And track the centroid
        for (size_t d=0; d<nd; d++)
          this->m_centroid[d] += ibox->getCentroid(d) * ibox->getSignal();
#endif
      }
    }
    else
    {
      //---------- Parallel refresh --------------
      throw std::runtime_error("Not implemented");
    }

  }


  // -------------------------------------------------------------------------------------------
  /** Cache the centroid of this box and all sub-boxes.
   * @param ts :: ThreadScheduler for parallel processing.
   */
  TMDE(
  void MDGridBox)::refreshCentroid(Kernel::ThreadScheduler * ts)
  {
    UNUSED_ARG(ts);
#ifdef MDBOX_TRACK_CENTROID

    // Start at 0.0
    for (size_t d=0; d<nd; d++)
      this->m_centroid[d] = 0;

    // Signal was calculated before (when adding)
    // Keep 0.0 if the signal is null. This avoids dividing by 0.0
    if (this->m_signal == 0) return;

    typename boxVector_t::iterator it;
    typename boxVector_t::iterator it_end = boxes.end();

    if (!ts)
    {
      //--------- Serial -----------
      for (it = boxes.begin(); it != it_end; ++it)
      {
        MDBoxBase<MDE,nd> * ibox = *it;

        // Refresh the centroid of all sub-boxes.
        ibox->refreshCentroid();

        signal_t iBoxSignal = ibox->getSignal();
        // And track the centroid
        for (size_t d=0; d<nd; d++)
          this->m_centroid[d] += ibox->getCentroid(d) * iBoxSignal;
      }
    }
    else
    {
      //---------- Parallel refresh --------------
      throw std::runtime_error("Not implemented");
    }

    // Normalize centroid by the total signal
    for (size_t d=0; d<nd; d++)
      this->m_centroid[d] /= this->m_signal;
#endif
  }


  //-----------------------------------------------------------------------------------------------
  /** Allocate and return a vector with a copy of all events contained
   */
  TMDE(
  std::vector< MDE > * MDGridBox)::getEventsCopy()
  {
    std::vector< MDE > * out = new std::vector<MDE>();
    //Make the copy
    //out->insert(out->begin(), data.begin(), data.end());
    return out;
  }

  //-----------------------------------------------------------------------------------------------
  /** Return all boxes contained within.
   *
   * @param outBoxes :: vector to fill
   * @param maxDepth :: max depth value of the returned boxes.
   * @param leafOnly :: if true, only add the boxes that are no more subdivided (leaves on the tree)
   */
  TMDE(
  void MDGridBox)::getBoxes(std::vector<MDBoxBase<MDE,nd> *> & outBoxes, size_t maxDepth, bool leafOnly)
  {
    // Add this box, unless we only want the leaves
    if (!leafOnly)
      outBoxes.push_back(this);

    if (this->getDepth() + 1 <= maxDepth)
    {
      for (size_t i=0; i<numBoxes; i++)
      {
        // Recursively go deeper, if needed
        boxes[i]->getBoxes(outBoxes, maxDepth, leafOnly);
      }
    }
    else
    {
      // Oh, we reached the max depth and want only leaves.
      // ... so we consider this box to be a leaf too.
      if (leafOnly)
        outBoxes.push_back(this);
    }

  }



  //-----------------------------------------------------------------------------------------------
  /** Return all boxes contained within, limited by an implicit function.
   *
   * This method evaluates each vertex to see how it is contained by the implicit function.
   * For example, if there are 4x4 boxes, there are 5x5 vertices to evaluate.
   *
   * All boxes that might be touching the implicit function are returned (including ones that
   * overlap without any point actually in the function).
   *
   * @param outBoxes :: vector to fill
   * @param maxDepth :: max depth value of the returned boxes.
   * @param leafOnly :: if true, only add the boxes that are no more subdivided (leaves on the tree)
   * @param function :: implicitFunction pointer
   */
  TMDE(
  void MDGridBox)::getBoxes(std::vector<MDBoxBase<MDE,nd> *> & outBoxes, size_t maxDepth, bool leafOnly, Mantid::Geometry::MDImplicitFunction * function)
  {
    // Add this box, unless we only want the leaves
    if (!leafOnly)
      outBoxes.push_back(this);

    if (this->getDepth() + 1 <= maxDepth)
    {
      // OK, let's look for children that are either touching or completely contained by the implicit function.

      // The number of vertices in each dimension is the # split[d] + 1
      size_t vertices_max[nd];  Utils::NestedForLoop::SetUp(nd, vertices_max, 0);

      // Total number of vertices for all the boxes
      size_t numVertices = 1;
      for (size_t d=0; d<nd; ++d)
      {
        vertices_max[d] = split[d]+1;
        numVertices *= vertices_max[d];
      }

      // The function is limited by this many planes
      size_t numPlanes = function->getNumPlanes();

      // This array will hold whether each vertex is contained by each plane.
      bool * vertexContained = new bool[numVertices * numPlanes];

      // The index to the vertex in each dimension
      size_t vertexIndex[nd]; Utils::NestedForLoop::SetUp(nd, vertexIndex, 0);
      // To get indexes in the array of vertexes
      size_t vertexIndexMaker[nd]; Utils::NestedForLoop::SetUpIndexMaker(nd, vertexIndexMaker, vertices_max);
      // To get indexes in the array of BOXES
      size_t boxIndexMaker[nd]; Utils::NestedForLoop::SetUpIndexMaker(nd, boxIndexMaker, split);

      size_t linearVertexIndex = 0;
      for (linearVertexIndex = 0; linearVertexIndex < numVertices; linearVertexIndex++)
      {
        // Get the nd-dimensional index
        Utils::NestedForLoop::GetIndicesFromLinearIndex(nd, linearVertexIndex, vertexIndexMaker, vertices_max, vertexIndex);

        // Coordinates of this vertex
        coord_t vertexCoord[nd];
        for (size_t d=0; d<nd; ++d)
          vertexCoord[d] = static_cast<coord_t>(vertexIndex[d]) * boxSize[d] + this->extents[d].min;

        // Now check each plane to see if the vertex is bounded by it
        for (size_t p=0; p<numPlanes; p++)
        {
          // Save whether this vertex is contained by this plane
          vertexContained[p*numVertices + linearVertexIndex] =
            function->getPlane(p).isPointBounded(vertexCoord);
        }
      }

      // OK, now we have an array saying which vertex is contained by which plane.

      // This is the number of vertices for each box, e.g. 8 in 3D
      size_t verticesPerBox = 1 << nd;

      /* There is a fixed relationship betwen a vertex (in a linear index) and its
       * neighbors for a given box. This array calculates this:  */
      size_t * vertexNeighborsOffsets = new size_t[verticesPerBox];

      for (size_t i=0; i < verticesPerBox; i++)
      {
        // Index (in n-dimensions) of this neighbor)
        size_t vertIndex[nd];
        for (size_t d=0; d<nd; d++)
        {
          vertIndex[d] = 0;
          // Use a bit mask to iterate through the 2^nd neighbor options
          size_t mask = 1 << d;
          if (i & mask)
            vertIndex[d] = 1;
        }
        size_t linIndex = Utils::NestedForLoop::GetLinearIndex(nd, vertIndex, vertexIndexMaker);
        vertexNeighborsOffsets[i] = linIndex;
      }

      // Go through all the boxes
      size_t boxIndex[nd]; Utils::NestedForLoop::SetUp(nd, boxIndex, 0);

      bool allDone = false;
      while (!allDone)
      {
        // Find the linear index into the BOXES array.
        size_t boxLinearIndex = Utils::NestedForLoop::GetLinearIndex(nd, boxIndex, boxIndexMaker);
        MDBoxBase<MDE,nd> * box = boxes[boxLinearIndex];

//        std::cout << "Box at " << Strings::join(boxIndex, boxIndex+nd, ", ")
//              << " (" << box->getExtentsStr() << ") ";


        // Find the linear index of the upper left vertex of the box.
        // (note that we're using the VERTEX index maker to find the linear index in that LARGER array)
        size_t vertLinearIndex = Utils::NestedForLoop::GetLinearIndex(nd, boxIndex, vertexIndexMaker);

        // OK, now its time to see if the box is touching or contained or out of it.
        // Recall that:
        //  - if a plane has NO vertices, then the box DOES NOT TOUCH
        //  - if EVERY plane has EVERY vertex, then the box is CONTAINED
        //  - if EVERY plane has at least one vertex, then the box is TOUCHING

        size_t numPlanesWithAllVertexes = 0;

        bool boxIsNotTouching = false;

        // Go plane by plane
        for (size_t p=0; p<numPlanes; p++)
        {
          size_t numVertexesInThisPlane = 0;
          // Evaluate the 2^nd vertexes for this box.
          for (size_t i=0; i<verticesPerBox; i++)
          {
            // (the index of the vertex is) = vertLinearIndex + vertexNeighborsOffsets[i]
            if (vertexContained[p*numVertices + vertLinearIndex + vertexNeighborsOffsets[i]])
              numVertexesInThisPlane++;
          }

          // Plane with no vertexes = NOT TOUCHING. You can exit now
          if (numVertexesInThisPlane == 0)
          {
            boxIsNotTouching = true;
            break;
          }

          // Plane has all the vertexes
          if (numVertexesInThisPlane == verticesPerBox)
            numPlanesWithAllVertexes++;
        } // (for each plane)

        // Is there a chance that the box is contained?
        if (!boxIsNotTouching)
        {

          if (numPlanesWithAllVertexes == numPlanes)
          {
            // All planes have all vertexes
            // The box is FULLY CONTAINED
            // So we can get ALL children and don't need to check the implicit function
            box->getBoxes(outBoxes, maxDepth, leafOnly);
          }
          else
          {
            // There is a chance the box is touching. Keep checking with implicit functions
            box->getBoxes(outBoxes, maxDepth, leafOnly, function);
          }
        }
        else
        {
//          std::cout << " is not touching at all." << std::endl;
        }

        // Move on to the next box in the list
        allDone = Utils::NestedForLoop::Increment(nd, boxIndex, split);
      }

      // Clean up.
      delete [] vertexContained;
      delete [] vertexNeighborsOffsets;

    } // Not at max depth
    else
    {
      // Oh, we reached the max depth and want only leaves.
      // ... so we consider this box to be a leaf too.
      if (leafOnly)
        outBoxes.push_back(this);
    }

  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the lowest-level box at the given coordinates
   * @param coords :: nd-sized array of the coordinate of the point to look at
   * @return MDBoxBase pointer.
   */
  template <typename MDE, size_t nd>
  const MDBoxBase<MDE,nd> * MDGridBox<MDE,nd>::getBoxAtCoord(const coord_t * coords) const
  {
    size_t index = 0;
    for (size_t d=0; d<nd; d++)
    {
      coord_t x = coords[d];
      int i = int((x - this->extents[d].min) / boxSize[d]);
      // NOTE: No bounds checking is done (for performance).
      // Accumulate the index
      index += (i * splitCumul[d]);
    }

    // Add it to the contained box
    if (index < numBoxes) // avoid segfaults for floating point round-off errors.
      return boxes[index]->getBoxAtCoord(coords);
    else
      return NULL;
  }



  //-----------------------------------------------------------------------------------------------
  /** Split a box that is contained in the GridBox, at the given index,
   * into a MDGridBox.
   *
   * Thread-safe as long as 'index' is different for all threads.
   *
   * @param index :: index into the boxes vector.
   *        Warning: No bounds check is made, don't give stupid values!
   * @param ts :: optional ThreadScheduler * that will be used to parallelize
   *        recursive splitting. Set to NULL for no recursive splitting.
   */
  TMDE(
  void MDGridBox)::splitContents(size_t index, ThreadScheduler * ts)
  {
    // You can only split it if it is a MDBox (not MDGridBox).
    MDBox<MDE, nd> * box = dynamic_cast<MDBox<MDE, nd> *>(boxes[index]);
    if (!box) return;
    // Track how many MDBoxes there are in the overall workspace
    this->m_BoxController->trackNumBoxes(box->getDepth());
    // Construct the grid box. This should take the object out of the disk MRU
    MDGridBox<MDE, nd> * gridbox = new MDGridBox<MDE, nd>(box);

    // Delete the old ungridded box
    delete boxes[index];
    // And now we have a gridded box instead of a boring old regular box.
    boxes[index] = gridbox;

    if (ts)
    {
      // Create a task to split the newly created MDGridBox.
      ts->push(new FunctionTask(boost::bind(&MDGridBox<MDE,nd>::splitAllIfNeeded, &*gridbox, ts) ) );
    }
  }

  //-----------------------------------------------------------------------------------------------
  /** Get the child index from its ID
   *
   * @param childId :: ID of the child you want
   * @return the index into the children of this grid box; size_t(-1) if NOT found.
   */
  TMDE(
  size_t MDGridBox)::getChildIndexFromID(size_t childId) const
  {
    for (size_t index=0; index<numBoxes; index++)
    {
      if (boxes[index]->getId() == childId)
        return index;
    }
    return size_t(-1);
  }



  //-----------------------------------------------------------------------------------------------
  /** Goes through all the sub-boxes and splits them if they contain
   * enough events to be worth it.
   *
   * @param ts :: optional ThreadScheduler * that will be used to parallelize
   *        recursive splitting. Set to NULL to do it serially.
   */
  TMDE(
  void MDGridBox)::splitAllIfNeeded(ThreadScheduler * ts)
  {
    for (size_t i=0; i < numBoxes; ++i)
    {
      MDBox<MDE, nd> * box = dynamic_cast<MDBox<MDE, nd> *>(boxes[i]);
      if (box)
      {
        // Plain MD-Box. Does it need to split?
        if (this->m_BoxController->willSplit(box->getNPoints(), box->getDepth() ))
        {
          // The MDBox needs to split into a grid box.
          if (!ts)
          {
            // ------ Perform split serially (no ThreadPool) ------
            MDGridBox<MDE, nd> * gridBox = new MDGridBox<MDE, nd>(box);
            // Track how many MDBoxes there are in the overall workspace
            this->m_BoxController->trackNumBoxes(box->getDepth());
            // Replace in the array
            boxes[i] = gridBox;
            // Delete the old box
            delete box;
            // Now recursively check if this NEW grid box's contents should be split too
            gridBox->splitAllIfNeeded(NULL);
          }
          else
          {
            // ------ Perform split in parallel (using ThreadPool) ------
            // So we create a task to split this MDBox,
            // Task is : this->splitContents(i, ts);
            ts->push(new FunctionTask(boost::bind(&MDGridBox<MDE,nd>::splitContents, &*this, i, ts) ) );
          }
        }
        else
        {
          // This box does NOT have enough events to be worth splitting
          if (box->dataAdded() && this->m_BoxController->isFileBacked() &&
              !box->getOnDisk())
          {
            // The box is NOT on disk but the workspace is file-backed.
            // Therefore, it is likely a NEW MDBox that was just created by splitting.
            // Mark that it is to be on disk from now on
            box->setOnDisk(true);
            // Set it "modified" so that it gets written out upon saving
            box->setDataModified(true);

            //Mark the box as "to-write" in DiskBuffer.
            this->m_BoxController->getDiskBuffer().toWrite(box);

//            // Make the MRU track it in the buffer. It is using up memory!
//            this->m_BoxController->getDiskBuffer().loading(box);
//            // So the MRU will cache it to the WriteBuffer when it falls out of the cache.
          }
        }
      }
      else
      {
        // It should be a MDGridBox
        MDGridBox<MDE, nd> * gridBox = dynamic_cast<MDGridBox<MDE, nd>*>(boxes[i]);
        if (gridBox)
        {
          // Now recursively check if this old grid box's contents should be split too
          if (!ts || (this->nPoints < this->m_BoxController->getAddingEvents_eventsPerTask()))
            // Go serially if there are only a few points contained (less overhead).
            gridBox->splitAllIfNeeded(ts);
          else
            // Go parallel if this is a big enough gridbox.
            // Task is : gridBox->splitAllIfNeeded(ts);
            ts->push(new FunctionTask(boost::bind(&MDGridBox<MDE,nd>::splitAllIfNeeded, &*gridBox, ts) ) );
        }
      }
    }
  }


  //-----------------------------------------------------------------------------------------------
  /** Add a single MDLeanEvent to the grid box. If the boxes
   * contained within are also gridded, this will recursively push the event
   * down to the deepest level.
   * Warning! No bounds checking is done (for performance). It must
   * be known that the event is within the bounds of the grid box before adding.
   *
   * Note! nPoints, signal and error must be re-calculated using refreshCache()
   * after all events have been added.
   *
   * @param event :: reference to a MDLeanEvent to add.
   * */
  TMDE(
  inline void MDGridBox)::addEvent( const MDE & event)
  {
    size_t index = 0;
    for (size_t d=0; d<nd; d++)
    {
      coord_t x = event.getCenter(d);
      int i = int((x - this->extents[d].min) / boxSize[d]);
      // NOTE: No bounds checking is done (for performance).
      //if (i < 0 || i >= int(split[d])) return;

      // Accumulate the index
      index += (i * splitCumul[d]);
    }

    // Add it to the contained box
    if (index < numBoxes) // avoid segfaults for floating point round-off errors.
      boxes[index]->addEvent(event);
  }

  //-----------------------------------------------------------------------------------------------
  /** Add a single MDLeanEvent to the grid box. If the boxes
   * contained within are also gridded, this will recursively push the event
   * down to the deepest level.
   *
   * Warning! No bounds checking is done (for performance). It must
   * be known that the event is within the bounds of the grid box before adding.
   *
   * Warning! Call is NOT thread-safe. Only 1 thread should be writing to this
   * box (or any child boxes) at a time
   *
   * Note! nPoints, signal and error must be re-calculated using refreshCache()
   * after all events have been added.
   *
   * @param event :: reference to a MDEvent to add.
   * */
  TMDE(
  inline void MDGridBox)::addEventUnsafe(const MDE & event)
  {
    size_t index = 0;
    for (size_t d=0; d<nd; d++)
    {
      coord_t x = event.getCenter(d);
      int i = int((x - this->extents[d].min) / boxSize[d]);
      // Accumulate the index
      index += (i * splitCumul[d]);
    }

    // Add it to the contained box
    if (index < numBoxes) // avoid segfaults for floating point round-off errors.
      boxes[index]->addEventUnsafe(event);
  }



  //-----------------------------------------------------------------------------------------------
  /** Perform centerpoint binning of events, with bins defined
   * in axes perpendicular to the axes of the workspace.
   *
   * @param bin :: MDBin object giving the limits of events to accept.
   * @param fullyContained :: optional bool array sized [nd] of which dimensions are known to be fully contained (for MDSplitBox)
   */
  TMDE(
  void MDGridBox)::centerpointBin(MDBin<MDE,nd> & bin, bool * fullyContained) const
  {

    // The MDBin ranges from index_min to index_max (inclusively) if each dimension. So
    // we'll need to make nested loops from index_min[0] to index_max[0]; from index_min[1] to index_max[1]; etc.
    int index_min[nd];
    int index_max[nd];
    // For running the nested loop, counters of each dimension. These are bounded by 0..split[d]
    size_t counters_min[nd];
    size_t counters_max[nd];

    for (size_t d=0; d<nd; d++)
    {
      int min,max;

      // The min index in this dimension (we round down - we'll include this edge)
      if (bin.m_min[d] >= this->extents[d].min)
      {
        min = int((bin.m_min[d] - this->extents[d].min) / boxSize[d]);
        counters_min[d] = min;
      }
      else
      {
        min = -1; // Goes past the edge
        counters_min[d] = 0;
      }

      // If the minimum is bigger than the number of blocks in that dimension, then the bin is off completely in
      //  that dimension. There is nothing to integrate.
      if (min >= static_cast<int>(split[d]))
        return;
      index_min[d] = min;

      // The max index in this dimension (we round UP, but when we iterate we'll NOT include this edge)
      if (bin.m_max[d] < this->extents[d].max)
      {
        max = int(ceil((bin.m_max[d] - this->extents[d].min) / boxSize[d])) - 1;
        counters_max[d] = max+1; // (the counter looping will NOT include counters_max[d])
      }
      else
      {
        max = int(split[d]); // Goes past THAT edge
        counters_max[d] = max; // (the counter looping will NOT include max)
      }

      // If the max value is before the min, that means NOTHING is in the bin, and we can return
      if ((max < min) || (max < 0))
        return;
      index_max[d] = max;

      //std::cout << d << " from " << std::setw(5) << index_min[d] << " to " << std::setw(5)  << index_max[d] << "inc" << std::endl;
    }

    // If you reach here, than at least some of bin is overlapping this box
    size_t counters[nd];
    for (size_t d=0; d<nd; d++)
      counters[d] = counters_min[d];

    bool allDone = false;
    while (!allDone)
    {
      size_t index = getLinearIndex(counters);
      //std::cout << index << ": " << counters[0] << ", " << counters[1] << std::endl;

      // Find if the box is COMPLETELY held in the bin.
      bool completelyWithin = true;
      for(size_t dim=0; dim<nd; dim++)
        if ((static_cast<int>(counters[dim]) <= index_min[dim]) ||
            (static_cast<int>(counters[dim]) >= index_max[dim]))
        {
          // The index we are at is at the edge of the integrated area (index_min or index_max-1)
          // That means that the bin only PARTIALLY covers this MDBox
          completelyWithin = false;
          break;
        }

      if (completelyWithin)
      {
        // Box is completely in the bin.
        //std::cout << "Box at index " << counters[0] << ", " << counters[1] << " is entirely contained.\n";
        // Use the aggregated signal and error
        bin.m_signal += boxes[index]->getSignal();
        bin.m_errorSquared += boxes[index]->getErrorSquared();
      }
      else
      {
        // Perform the binning
        boxes[index]->centerpointBin(bin,fullyContained);
      }

      // Increment the counter(s) in the nested for loops.
      allDone = Utils::NestedForLoop::Increment(nd, counters, counters_max, counters_min);
    }

  }




//  TMDE(
//  void MDGridBox)::generalBin(MDBin<MDE,nd> & bin, Mantid::API::ImplicitFunction & function) const
//  {
//    // The MDBin ranges from index_min to index_max (inclusively) if each dimension. So
//    // we'll need to make nested loops from index_min[0] to index_max[0]; from index_min[1] to index_max[1]; etc.
//    int index_min[nd];
//    int index_max[nd];
//    // For running the nested loop, counters of each dimension. These are bounded by 0..split[d]
//    size_t counters_min[nd];
//    size_t counters_max[nd];
//
//    for (size_t d=0; d<nd; d++)
//    {
//      int min,max;
//
//      // The min index in this dimension (we round down - we'll include this edge)
//      if (bin.m_min[d] >= this->extents[d].min)
//      {
//        min = int((bin.m_min[d] - this->extents[d].min) / boxSize[d]);
//        counters_min[d] = min;
//      }
//      else
//      {
//        min = -1; // Goes past the edge
//        counters_min[d] = 0;
//      }
//
//      // If the minimum is bigger than the number of blocks in that dimension, then the bin is off completely in
//      //  that dimension. There is nothing to integrate.
//      if (min >= static_cast<int>(split[d]))
//        return;
//      index_min[d] = min;
//
//      // The max index in this dimension (we round UP, but when we iterate we'll NOT include this edge)
//      if (bin.m_max[d] < this->extents[d].max)
//      {
//        max = int(ceil((bin.m_max[d] - this->extents[d].min) / boxSize[d])) - 1;
//        counters_max[d] = max+1; // (the counter looping will NOT include counters_max[d])
//      }
//      else
//      {
//        max = int(split[d]); // Goes past THAT edge
//        counters_max[d] = max; // (the counter looping will NOT include max)
//      }
//
//      // If the max value is before the min, that means NOTHING is in the bin, and we can return
//      if ((max < min) || (max < 0))
//        return;
//      index_max[d] = max;
//
//      //std::cout << d << " from " << std::setw(5) << index_min[d] << " to " << std::setw(5)  << index_max[d] << "inc" << std::endl;
//    }
//
//    // If you reach here, than at least some of bin is overlapping this box
//
//
//    // We start by looking at the vertices at every corner of every box contained,
//    // to see which boxes are partially contained/fully contained.
//
//    // One entry with the # of vertices in this box contained; start at 0.
//    size_t * verticesContained = new size_t[numBoxes];
//    memset( verticesContained, 0, numBoxes * sizeof(size_t) );
//
//    // Set to true if there is a possibility of the box at least partly touching the integration volume.
//    bool * boxMightTouch = new bool[numBoxes];
//    memset( boxMightTouch, 0, numBoxes * sizeof(bool) );
//
//    // How many vertices does one box have? 2^nd, or bitwise shift left 1 by nd bits
//    size_t maxVertices = 1 << nd;
//
//    // The index to the vertex in each dimension
//    size_t * vertexIndex = Utils::NestedForLoop::SetUp(nd, 0);
//
//    // This is the index in each dimension at which we start looking at vertices
//    size_t * vertices_min = Utils::NestedForLoop::SetUp(nd, 0);
//    for (size_t d=0; d<nd; ++d)
//    {
//      vertices_min[d] = counters_min[d];
//      vertexIndex[d] = vertices_min[d]; // This is where we start
//    }
//
//    // There is one more vertex in each dimension than there are boxes we are considering
//    size_t * vertices_max = Utils::NestedForLoop::SetUp(nd, 0);
//    for (size_t d=0; d<nd; ++d)
//      vertices_max[d] = counters_max[d]+1;
//
//    size_t * boxIndex = Utils::NestedForLoop::SetUp(nd, 0);
//    size_t * indexMaker = Utils::NestedForLoop::SetUpIndexMaker(nd, split);
//
//    bool allDone = false;
//    while (!allDone)
//    {
//      // Coordinates of this vertex
//      coord_t vertexCoord[nd];
//      bool masks[nd];
//      for (size_t d=0; d<nd; ++d)
//      {
//        vertexCoord[d] = double(vertexIndex[d]) * boxSize[d] + this->extents[d].min;
//        masks[d] = false; //HACK ... assumes that all vertexes are used.
//      }
//      // Is this vertex contained?
//      if (function.evaluate(vertexCoord, masks, nd))
//      {
//        // Yes, this vertex is contained within the integration volume!
////        std::cout << "vertex at " << vertexCoord[0] << ", " << vertexCoord[1] << ", " << vertexCoord[2] << " is contained\n";
//
//        // This vertex is shared by up to 2^nd adjacent boxes (left-right along each dimension).
//        for (size_t neighb=0; neighb<maxVertices; ++neighb)
//        {
//          // The index of the box is the same as the vertex, but maybe - 1 in each possible combination of dimensions
//          bool badIndex = false;
//          // Build the index of the neighbor
//          for (size_t d=0; d<nd;d++)
//          {
//            boxIndex[d] = vertexIndex[d] - ((neighb & (1 << d)) >> d); //(this does a bitwise and mask, shifted back to 1 to subtract 1 to the dimension)
//            // Taking advantage of the fact that unsigned(0)-1 = some large POSITIVE number.
//            if (boxIndex[d] >= split[d])
//            {
//              badIndex = true;
//              break;
//            }
//          }
//          if (!badIndex)
//          {
//            // Convert to linear index
//            size_t linearIndex = Utils::NestedForLoop::GetLinearIndex(nd, boxIndex, indexMaker);
//            // So we have one more vertex touching this box that is contained in the integration volume. Whew!
//            verticesContained[linearIndex]++;
////            std::cout << "... added 1 vertex to box " << boxes[linearIndex]->getExtentsStr() << "\n";
//          }
//        }
//      }
//
//      // Increment the counter(s) in the nested for loops.
//      allDone = Utils::NestedForLoop::Increment(nd, vertexIndex, vertices_max, vertices_min);
//    }
//
//    // OK, we've done all the vertices. Now we go through and check each box.
//    size_t numFullyContained = 0;
//    //size_t numPartiallyContained = 0;
//
//    // We'll iterate only through the boxes with (bin)
//    size_t counters[nd];
//    for (size_t d=0; d<nd; d++)
//      counters[d] = counters_min[d];
//
//    allDone = false;
//    while (!allDone)
//    {
//      size_t index = getLinearIndex(counters);
//      MDBoxBase<MDE,nd> * box = boxes[index];
//
//      // Is this box fully contained?
//      if (verticesContained[index] >= maxVertices)
//      {
//        // Use the integrated sum of signal in the box
//        bin.m_signal += box->getSignal();
//        bin.m_errorSquared += box->getErrorSquared();
//        numFullyContained++;
//      }
//      else
//      {
//        // The box MAY be contained. Need to evaluate every event
//
//        // box->generalBin(bin,function);
//      }
//
//      // Increment the counter(s) in the nested for loops.
//      allDone = Utils::NestedForLoop::Increment(nd, counters, counters_max, counters_min);
//    }
//
////    std::cout << "Depth " << this->getDepth() << " with " << numFullyContained << " fully contained; " << numPartiallyContained << " partial. Signal = " << signal <<"\n";
//
//    delete [] verticesContained;
//    delete [] boxMightTouch;
//    delete [] vertexIndex;
//    delete [] vertices_max;
//    delete [] boxIndex;
//    delete [] indexMaker;
//
//  }
//
//



  //-----------------------------------------------------------------------------------------------
  /** Integrate the signal within a sphere; for example, to perform single-crystal
   * peak integration.
   * The CoordTransform object could be used for more complex shapes, e.g. "lentil" integration, as long
   * as it reduces the dimensions to a single value.
   *
   * @param radiusTransform :: nd-to-1 coordinate transformation that converts from these
   *        dimensions to the distance (squared) from the center of the sphere.
   * @param radiusSquared :: radius^2 below which to integrate
   * @param signal [out] :: set to the integrated signal
   * @param errorSquared [out] :: set to the integrated squared error.
   */
  TMDE(
  void MDGridBox)::integrateSphere(CoordTransform & radiusTransform, const coord_t radiusSquared, signal_t & signal, signal_t & errorSquared) const
  {
    // We start by looking at the vertices at every corner of every box contained,
    // to see which boxes are partially contained/fully contained.

    // One entry with the # of vertices in this box contained; start at 0.
    size_t * verticesContained = new size_t[numBoxes];
    memset( verticesContained, 0, numBoxes * sizeof(size_t) );

    // Set to true if there is a possibility of the box at least partly touching the integration volume.
    bool * boxMightTouch = new bool[numBoxes];
    memset( boxMightTouch, 0, numBoxes * sizeof(bool) );

    // How many vertices does one box have? 2^nd, or bitwise shift left 1 by nd bits
    size_t maxVertices = 1 << nd;

    // The number of vertices in each dimension is the # split[d] + 1
    size_t vertices_max[nd]; Utils::NestedForLoop::SetUp(nd, vertices_max, 0);
    for (size_t d=0; d<nd; ++d)
      vertices_max[d] = split[d]+1;

    // The index to the vertex in each dimension
    size_t vertexIndex[nd]; Utils::NestedForLoop::SetUp(nd, vertexIndex, 0);
    size_t boxIndex[nd]; Utils::NestedForLoop::SetUp(nd, boxIndex, 0);
    size_t indexMaker[nd]; Utils::NestedForLoop::SetUpIndexMaker(nd, indexMaker, split);

    bool allDone = false;
    while (!allDone)
    {
      // Coordinates of this vertex
      coord_t vertexCoord[nd];
      for (size_t d=0; d<nd; ++d)
        vertexCoord[d] = static_cast<coord_t>(vertexIndex[d]) * boxSize[d] + this->extents[d].min;

      // Is this vertex contained?
      coord_t out[nd];
      radiusTransform.apply(vertexCoord, out);
      if (out[0] < radiusSquared)
      {
        // Yes, this vertex is contained within the integration volume!
//        std::cout << "vertex at " << vertexCoord[0] << ", " << vertexCoord[1] << ", " << vertexCoord[2] << " is contained\n";

        // This vertex is shared by up to 2^nd adjacent boxes (left-right along each dimension).
        for (size_t neighb=0; neighb<maxVertices; ++neighb)
        {
          // The index of the box is the same as the vertex, but maybe - 1 in each possible combination of dimensions
          bool badIndex = false;
          // Build the index of the neighbor
          for (size_t d=0; d<nd;d++)
          {
            boxIndex[d] = vertexIndex[d] - ((neighb & ((size_t)1 << d)) >> d); //(this does a bitwise and mask, shifted back to 1 to subtract 1 to the dimension)
            // Taking advantage of the fact that unsigned(0)-1 = some large POSITIVE number.
            if (boxIndex[d] >= split[d])
            {
              badIndex = true;
              break;
            }
          }
          if (!badIndex)
          {
            // Convert to linear index
            size_t linearIndex = Utils::NestedForLoop::GetLinearIndex(nd, boxIndex, indexMaker);
            // So we have one more vertex touching this box that is contained in the integration volume. Whew!
            verticesContained[linearIndex]++;
//            std::cout << "... added 1 vertex to box " << boxes[linearIndex]->getExtentsStr() << "\n";
          }
        }
      }

      // Increment the counter(s) in the nested for loops.
      allDone = Utils::NestedForLoop::Increment(nd, vertexIndex, vertices_max);
    }

    // OK, we've done all the vertices. Now we go through and check each box.
    size_t numFullyContained = 0;
    size_t numPartiallyContained = 0;

    for (size_t i=0; i < numBoxes; ++i)
    {
      MDBoxBase<MDE, nd> * box = boxes[i];
      // Box partially contained?
      bool partialBox = false;

      // Is this box fully contained?
      if (verticesContained[i] >= maxVertices)
      {
        // Use the integrated sum of signal in the box
        signal += box->getSignal();
        errorSquared += box->getErrorSquared();

//        std::cout << "box at " << i << " (" << box->getExtentsStr() << ") is fully contained. Vertices = " << verticesContained[i] << "\n";

        numFullyContained++;
        // Go on to the next box
        continue;
      }

      if (verticesContained[i] == 0)
      {
        // There is a chance that this part of the box is within integration volume,
        // even if no vertex of it is.
        coord_t boxCenter[nd];
        box->getCenter(boxCenter);

        // Distance from center to the peak integration center
        coord_t out[nd];
        radiusTransform.apply(boxCenter, out);

        if (out[0] < diagonalSquared*0.72 + radiusSquared)
        {
          // If the center is closer than the size of the box, then it MIGHT be touching.
          // (We multiply by 0.72 (about sqrt(2)) to look for half the diagonal).
          // NOTE! Watch out for non-spherical transforms!
//          std::cout << "box at " << i << " is maybe touching\n";
          partialBox = true;
        }
      }
      else
      {
        partialBox = true;
//        std::cout << "box at " << i << " has a vertex touching\n";
      }

      // We couldn't rule out that the box might be partially contained.
      if (partialBox)
      {
        // Use the detailed integration method.
        box->integrateSphere(radiusTransform, radiusSquared, signal, errorSquared);
//        std::cout << ".signal=" << signal << "\n";
        numPartiallyContained++;
      }
    } // (for each box)

//    std::cout << "Depth " << this->getDepth() << " with " << numFullyContained << " fully contained; " << numPartiallyContained << " partial. Signal = " << signal <<"\n";

    delete [] verticesContained;
    delete [] boxMightTouch;
  }


  //-----------------------------------------------------------------------------------------------
  /** Find the centroid of all events contained within by doing a weighted average
   * of their coordinates.
   *
   * @param radiusTransform :: nd-to-1 coordinate transformation that converts from these
   *        dimensions to the distance (squared) from the center of the sphere.
   * @param radiusSquared :: radius^2 below which to integrate
   * @param[out] centroid :: array of size [nd]; its centroid will be added
   * @param[out] signal :: set to the integrated signal
   */
  TMDE(
  void MDGridBox)::centroidSphere(CoordTransform & radiusTransform, const coord_t radiusSquared, coord_t * centroid, signal_t & signal) const
  {
    for (size_t i=0; i < numBoxes; ++i)
    {
      // Go through each contained box
      MDBoxBase<MDE, nd> * box = boxes[i];
      coord_t boxCenter[nd];
      box->getCenter(boxCenter);

      // Distance from center to the peak integration center
      coord_t out[nd];
      radiusTransform.apply(boxCenter, out);

      if (out[0] < diagonalSquared*0.72 + radiusSquared)
      {
        // If the center is closer than the size of the box, then it MIGHT be touching.
        // (We multiply by 0.72 (about sqrt(2)) to look for half the diagonal).
        // NOTE! Watch out for non-spherical transforms!

        // Go down one level to keep centroiding
        box->centroidSphere(radiusTransform, radiusSquared, centroid, signal);
      }
    } // (for each box)
  }

  /**
  Getter for the masking status of the gridded box.
  @return TRUE if ANY ONE of its referenced boxes is masked.
  */
  TMDE(
  bool MDGridBox)::getIsMasked() const
  {
    bool isMasked = false;
    for (size_t i=0; i < numBoxes; ++i)
    {
      // Go through each contained box
      MDBoxBase<MDE, nd> * box = boxes[i];
      if(box->getIsMasked())
      {
        isMasked = true;
        break;
      }
    }
    return isMasked;
  }

  ///Setter for masking the box
  TMDE(
  void MDGridBox)::mask()
  {
    for (size_t i=0; i < numBoxes; ++i)
    {
      // Go through each contained box
      MDBoxBase<MDE, nd> * box = boxes[i];
      box->mask();
    }
  }

  ///Setter for unmasking the box
  TMDE(
  void MDGridBox)::unmask()
  {
    for (size_t i=0; i < numBoxes; ++i)
    {
      // Go through each contained box
      MDBoxBase<MDE, nd> * box = boxes[i];
      box->unmask();
    }
  }

}//namespace MDEvents

}//namespace Mantid

