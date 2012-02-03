/*WIKI* 

This algorithm performs dense binning of the events in multiple dimensions of an input
[[MDEventWorkspace]] and places them into a dense MDHistoWorkspace with 1-4 dimensions.

The input MDEventWorkspace may have more dimensions than the number of output dimensions.
The names of the dimensions in the DimX, etc. parameters are used to find the corresponding
dimensions that will be created in the output.

An ImplicitFunction can be defined using the ImplicitFunctionXML parameter;
any points NOT belonging inside of the ImplicitFunction will be set as NaN (not-a-number).

=== Axis-Aligned Binning ===

This is binning where the output axes are aligned with the original workspace.
Specify each of the AlignedDimX, etc. parameters with these values, separated by commas:
* First, the name of the dimension in the original workspace
* Next, the start and end position along that dimension
* Finally, the number of bins to use in that dimensions.

If you specify fewer output dimensions, then events in the remaining dimensions will be integrated.

=== Non-Axis Aligned Binning ===

This allows rebinning to a new arbitrary space. You must specify up to 4 basis vectors
with these parameters, separated by commas: 'name, units, x,y,z,.., length, number_of_bins':
* Name: of the new dimension
* Units: string giving the units
* x, y, z, etc.: a vector, matching the number of dimensions, giving the direction of the basis vector
* Length: the length, along the basis vector direction, to take along the input dimension
* number_of_bins: separate Length into this many bins.

You must also specify a point for the origin.
This point (in the input space) corresponds to 0,0,0 in the output space.

Finally, the ForceOrthogonal parameter will modify your basis vectors
if needed to make them orthogonal to each other. Only works in 3 dimensions!

=== Binning a MDHistoWorkspace ===

It is possible to rebin a [[MDHistoWorkspace]].
Each MDHistoWorkspace holds a reference to the [[MDEventWorkspace]] that created it,
as well as the coordinate transformation that was used. In this case, the rebinning is actually performed on the original MDEventWorkspace,
after suitably transforming the basis vectors.

Only the non-axis aligned binning method can be performed on a MDHistoWorkspace!
Of course, your basis vectors can be aligned with the dimensions, which is equivalent.

For more details on the coordinate transformations applied in this case, please see [[BinMD Coordinate Transformations]].

[[File:BinMD_Coordinate_Transforms_withLine.png]]

*WIKI*/

#include "MantidAPI/ImplicitFunctionFactory.h"
#include "MantidGeometry/MDGeometry/MDBoxImplicitFunction.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Utils.h"
#include "MantidMDEvents/BinMD.h"
#include "MantidMDEvents/CoordTransformAffineParser.h"
#include "MantidMDEvents/CoordTransformAligned.h"
#include "MantidMDEvents/IMDBox.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include <boost/algorithm/string.hpp>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Element.h>
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidMDEvents/CoordTransformAffine.h"

using Mantid::Kernel::CPUTimer;
using Mantid::Kernel::EnabledWhenProperty;

namespace Mantid
{
namespace MDEvents
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(BinMD)

  using namespace Mantid::Kernel;
  using namespace Mantid::API;
  using namespace Mantid::Geometry;


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  BinMD::BinMD()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  BinMD::~BinMD()
  {
  }


  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void BinMD::initDocs()
  {
    this->setWikiSummary("Take a [[MDEventWorkspace]] and bin into into a dense, multi-dimensional histogram workspace ([[MDHistoWorkspace]]).");
    this->setOptionalMessage("Take a MDEventWorkspace and bin into into a dense, multi-dimensional histogram workspace (MDHistoWorkspace).");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void BinMD::init()
  {
    declareProperty(new WorkspaceProperty<IMDWorkspace>("InputWorkspace","",Direction::Input), "An input MDWorkspace.");

    // Properties for specifying the slice to perform.
    this->initSlicingProps();

    // --------------- Processing methods and options ---------------------------------------
    std::string grp = "Methods";
    declareProperty(new PropertyWithValue<std::string>("ImplicitFunctionXML","",Direction::Input),
        "XML string describing the implicit function determining which bins to use.");
    setPropertyGroup("ImplicitFunctionXML", grp);

    declareProperty(new PropertyWithValue<bool>("IterateEvents",true,Direction::Input),
        "Alternative binning method where you iterate through every event, placing them in the proper bin.\n"
        "This may be faster for workspaces with few events and lots of output bins.");
    setPropertyGroup("IterateEvents", grp);

    declareProperty(new PropertyWithValue<bool>("Parallel",false,Direction::Input),
        "Temporary parameter: true to run in parallel. This is ignored for file-backed workspaces, where running in parallel makes things slower due to disk thrashing.");
    setPropertyGroup("Parallel", grp);

    declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output), "A name for the output MDHistoWorkspace.");

  }



  //----------------------------------------------------------------------------------------------
  /** Bin the contents of a MDBox
   *
   * @param box :: pointer to the MDBox to bin
   * @param chunkMin :: the minimum index in each dimension to consider "valid" (inclusive)
   * @param chunkMax :: the maximum index in each dimension to consider "valid" (exclusive)
   */
  template<typename MDE, size_t nd>
  inline void BinMD::binMDBox(MDBox<MDE, nd> * box, size_t * chunkMin, size_t * chunkMax)
  {
    // An array to hold the rotated/transformed coordinates
    coord_t * outCenter = new coord_t[m_outD];

    // Evaluate whether the entire box is in the same bin
    if (box->getNPoints() > (1 << nd) * 2)
    {
      // There is a check that the number of events is enough for it to make sense to do all this processing.
      size_t numVertexes = 0;
      coord_t * vertexes = box->getVertexesArray(numVertexes);

      // All vertexes have to be within THE SAME BIN = have the same linear index.
      size_t lastLinearIndex = 0;
      bool badOne = false;

      for (size_t i=0; i<numVertexes; i++)
      {
        // Cache the center of the event (again for speed)
        const coord_t * inCenter = vertexes + i * nd;

        // Now transform to the output dimensions
        m_transform->apply(inCenter, outCenter);
        //std::cout << "Input coord " << VMD(nd,inCenter) << " transformed to " <<  VMD(nd,outCenter) << std::endl;

        // To build up the linear index
        size_t linearIndex = 0;
        // To mark VERTEXES outside range
        badOne = false;

        /// Loop through the dimensions on which we bin
        for (size_t bd=0; bd<m_outD; bd++)
        {
          // What is the bin index in that dimension
          coord_t x = outCenter[bd];
          size_t ix = size_t(x);
          // Within range (for this chunk)?
          if ((x >= 0) && (ix >= chunkMin[bd]) && (ix < chunkMax[bd]))
          {
            // Build up the linear index
            linearIndex += indexMultiplier[bd] * ix;
          }
          else
          {
            // Outside the range
            badOne = true;
            break;
          }
        } // (for each dim in MDHisto)

        // Is the vertex at the same place as the last one?
        if (!badOne)
        {
          if ((i > 0) && (linearIndex != lastLinearIndex))
          {
            // Change of index
            badOne = true;
            break;
          }
          lastLinearIndex = linearIndex;
        }

        // Was the vertex completely outside the range?
        if (badOne)
          break;
      } // (for each vertex)

      delete [] vertexes;

      if (!badOne)
      {
        // Yes, the entire box is within a single bin
//        std::cout << "Box at " << box->getExtentsStr() << " is within a single bin.\n";
        // Add the CACHED signal from the entire box
        signals[lastLinearIndex] += box->getSignal();
        errors[lastLinearIndex] += box->getErrorSquared();
        // And don't bother looking at each event. This may save lots of time loading from disk.
        delete [] outCenter;
        return;
      }
    }

    // If you get here, you could not determine that the entire box was in the same bin.
    // So you need to iterate through events.

    const std::vector<MDE> & events = box->getConstEvents();
    typename std::vector<MDE>::const_iterator it = events.begin();
    typename std::vector<MDE>::const_iterator it_end = events.end();
    for (; it != it_end; it++)
    {
      // Cache the center of the event (again for speed)
      const coord_t * inCenter = it->getCenter();

      // Now transform to the output dimensions
      m_transform->apply(inCenter, outCenter);

      // To build up the linear index
      size_t linearIndex = 0;
      // To mark events outside range
      bool badOne = false;

      /// Loop through the dimensions on which we bin
      for (size_t bd=0; bd<m_outD; bd++)
      {
        // What is the bin index in that dimension
        coord_t x = outCenter[bd];
        size_t ix = size_t(x);
        // Within range (for this chunk)?
        if ((x >= 0) && (ix >= chunkMin[bd]) && (ix < chunkMax[bd]))
        {
          // Build up the linear index
          linearIndex += indexMultiplier[bd] * ix;
        }
        else
        {
          // Outside the range
          badOne = true;
          break;
        }
      } // (for each dim in MDHisto)

      if (!badOne)
      {
        signals[linearIndex] += it->getSignal();
        errors[linearIndex] += it->getErrorSquared();
      }
    }
    // Done with the events list
    box->releaseEvents();

    delete [] outCenter;
  }


  //----------------------------------------------------------------------------------------------
  /** Perform binning by iterating through every event and placing them in the output workspace
   *
   * @param ws :: MDEventWorkspace of the given type.
   */
  template<typename MDE, size_t nd>
  void BinMD::binByIterating(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    BoxController_sptr bc = ws->getBoxController();

    // Start with signal at 0.0
    outWS->setTo(0.0, 0.0);

    // Cache some data to speed up accessing them a bit
    indexMultiplier = new size_t[m_outD];
    for (size_t d=0; d<m_outD; d++)
    {
      if (d > 0)
        indexMultiplier[d] = outWS->getIndexMultiplier()[d-1];
      else
        indexMultiplier[d] = 1;
    }
    signals = outWS->getSignalArray();
    errors = outWS->getErrorSquaredArray();

    // The dimension (in the output workspace) along which we chunk for parallel processing
    // TODO: Find the smartest dimension to chunk against
    size_t chunkDimension = 0;

    // How many bins (in that dimension) per chunk.
    // Try to split it so each core will get 2 tasks:
    int chunkNumBins =  int(m_binDimensions[chunkDimension]->getNBins() / (Mantid::Kernel::ThreadPool::getNumPhysicalCores() * 2));
    if (chunkNumBins < 1) chunkNumBins = 1;

    // Do we actually do it in parallel?
    bool doParallel = getProperty("Parallel");
    // Not if file-backed!
    if (bc->isFileBacked()) doParallel = false;
    if (!doParallel)
      chunkNumBins = int(m_binDimensions[chunkDimension]->getNBins());

    // Total number of steps
    size_t progNumSteps = 0;
    if (prog) prog->setNotifyStep(0.1);
    if (prog) prog->resetNumSteps(100, 0.00, 1.0);

    // Run the chunks in parallel. There is no overlap in the output workspace so it is
    // thread safe to write to it..
    // cppcheck-suppress syntaxError
    PRAGMA_OMP( parallel for schedule(dynamic,1) if (doParallel) )
    for(int chunk=0; chunk < int(m_binDimensions[chunkDimension]->getNBins()); chunk += chunkNumBins)
    {
      PARALLEL_START_INTERUPT_REGION
      // Region of interest for this chunk.
      size_t * chunkMin = new size_t[m_outD];
      size_t * chunkMax = new size_t[m_outD];
      for (size_t bd=0; bd<m_outD; bd++)
      {
        // Same limits in the other dimensions
        chunkMin[bd] = 0;
        chunkMax[bd] = m_binDimensions[bd]->getNBins();
      }
      // Parcel out a chunk in that single dimension dimension
      chunkMin[chunkDimension] = size_t(chunk);
      if (size_t(chunk+chunkNumBins) > m_binDimensions[chunkDimension]->getNBins())
        chunkMax[chunkDimension] = m_binDimensions[chunkDimension]->getNBins();
      else
        chunkMax[chunkDimension] = size_t(chunk+chunkNumBins);

      // Build an implicit function (it needs to be in the space of the MDEventWorkspace)
      MDImplicitFunction * function = this->getImplicitFunctionForChunk(chunkMin, chunkMax);

      // Use getBoxes() to get an array with a pointer to each box
      std::vector<IMDBox<MDE,nd>*> boxes;
      // Leaf-only; no depth limit; with the implicit function passed to it.
      ws->getBox()->getBoxes(boxes, 1000, true, function);

      // Sort boxes by file position IF file backed. This reduces seeking time, hopefully.
      if (bc->isFileBacked())
        IMDBox<MDE, nd>::sortBoxesByFilePos(boxes);

      // For progress reporting, the # of boxes
      if (prog)
      {
        PARALLEL_CRITICAL(BinMD_progress)
        {
          std::cout << "Chunk " << chunk << ": found " << boxes.size() << " boxes within the implicit function." << std::endl;
          progNumSteps += boxes.size();
          prog->setNumSteps( progNumSteps );
        }
      }

      // Go through every box for this chunk.
      for (size_t i=0; i<boxes.size(); i++)
      {
        MDBox<MDE,nd> * box = dynamic_cast<MDBox<MDE,nd> *>(boxes[i]);
        // Perform the binning in this separate method.
        if (box)
          this->binMDBox(box, chunkMin, chunkMax);

        // Progress reporting
        if (prog) prog->report();

      }// for each box in the vector
      PARALLEL_END_INTERUPT_REGION
    } // for each chunk in parallel
    PARALLEL_CHECK_INTERUPT_REGION



    // Now the implicit function
    if (implicitFunction)
    {
      prog->report("Applying implicit function.");
      signal_t nan = std::numeric_limits<signal_t>::quiet_NaN();
      outWS->applyImplicitFunction(implicitFunction, nan, nan);
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Templated method to apply the binning operation to the particular
   * MDEventWorkspace passed in.
   *
   * @param ws :: MDEventWorkspace of the given type
   */
  template<typename MDE, size_t nd>
  void BinMD::do_centerpointBin(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    bool DODEBUG = true;

    CPUTimer tim;

    // Number of output binning dimensions found
    size_t m_outD = m_binDimensions.size();

    //Since the costs are not known ahead of time, use a simple FIFO buffer.
    ThreadScheduler * ts = new ThreadSchedulerFIFO();

    // Create the threadpool with: all CPUs, a progress reporter
    ThreadPool tp(ts, 0, prog);

    // For progress reporting, the approx  # of tasks
    if (prog)
      prog->setNumSteps( int(outWS->getNPoints()/100) );

    // The root-level box.
    IMDBox<MDE,nd> * rootBox = ws->getBox();

    // This is the limit to loop over in each dimension
    size_t * index_max = new size_t[m_outD];
    for (size_t bd=0; bd<m_outD; bd++) index_max[bd] = m_binDimensions[bd]->getNBins();

    // Cache a calculation to convert indices x,y,z,t into a linear index.
    size_t * index_maker = new size_t[m_outD];
    Utils::NestedForLoop::SetUpIndexMaker(m_outD, index_maker, index_max);

    int numPoints = int(outWS->getNPoints());

    // Big efficiency gain is obtained by grouping a few bins per task.
    size_t binsPerTask = 100; (void)binsPerTask; // Avoid a compiler warning on gcc 4.6

    // Run in OpenMP with dynamic scheduling and a smallish chunk size (binsPerTask)
    // Right now, not parallel for file-backed systems.
    bool fileBacked = (ws->getBoxController()->getFile() != NULL);
    PRAGMA_OMP(parallel for schedule(dynamic, binsPerTask) if (!fileBacked)  )
    for (int i=0; i < numPoints; i++)
    {
      PARALLEL_START_INTERUPT_REGION

      size_t linear_index = size_t(i);
      // nd >= m_outD in all cases so this is safe.
      size_t index[nd];

      // Get the index at each dimension for this bin.
      Utils::NestedForLoop::GetIndicesFromLinearIndex(m_outD, linear_index, index_maker, index_max, index);

      // Construct the bin and its coordinates
      MDBin<MDE,nd> bin;
      for (size_t bd=0; bd<m_outD; bd++)
      {
        // Index in this binning dimension (i_x, i_y, etc.)
        size_t idx = index[bd];
        // Dimension in the MDEventWorkspace
        size_t d = m_dimensionToBinFrom[bd];
        // Corresponding extents
        bin.m_min[d] = m_binDimensions[bd]->getX(idx);
        bin.m_max[d] = m_binDimensions[bd]->getX(idx+1);
      }
      bin.m_index = linear_index;

      // Check if the bin is in the ImplicitFunction (if any)
      bool binContained = true;
      if (implicitFunction)
      {
        binContained = implicitFunction->isPointContained(bin.m_min); //TODO. Correct argument passed to this method?
      }

      if (binContained)
      {
        // Array of bools set to true when a dimension is fully contained (binary splitting only)
        bool fullyContained[nd];
        for (size_t d=0; d<nd; d++)
          fullyContained[d] = false;

        // This will recursively bin into the sub grids
        rootBox->centerpointBin(bin, fullyContained);

        // Save the data into the dense histogram
        outWS->setSignalAt(linear_index, bin.m_signal);
        outWS->setErrorSquaredAt(linear_index, bin.m_errorSquared);
      }

      // Report progress but not too often.
      if (((linear_index % 100) == 0) && prog ) prog->report();

      PARALLEL_END_INTERUPT_REGION
    } // (for each linear index)
    PARALLEL_CHECK_INTERUPT_REGION

    if (DODEBUG) std::cout << tim << " to run the openmp loop.\n";

    delete [] index_max;
    delete [] index_maker;
  }














  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void BinMD::exec()
  {
    // Input MDEventWorkspace
    m_inWS = getProperty("InputWorkspace");
    // Look at properties, create either axis-aligned or general transform.
    this->createTransform();

    // De serialize the implicit function
    std::string ImplicitFunctionXML = getPropertyValue("ImplicitFunctionXML");
    implicitFunction = NULL;
    if (!ImplicitFunctionXML.empty())
      implicitFunction = Mantid::API::ImplicitFunctionFactory::Instance().createUnwrapped(ImplicitFunctionXML);

    prog = new Progress(this, 0, 1.0, 1); // This gets deleted by the thread pool; don't delete it in here.

    // Create the dense histogram. This allocates the memory
    outWS = MDHistoWorkspace_sptr(new MDHistoWorkspace(m_binDimensions));

    // Saves the geometry transformation from original to binned in the workspace
    outWS->setTransformFromOriginal( this->m_transformFromOriginal, 0 );
    outWS->setTransformToOriginal( this->m_transformToOriginal, 0 );
    for (size_t i=0; i<m_bases.size(); i++)
      outWS->setBasisVector(i, m_bases[i]);
    outWS->setOrigin( this->m_origin );
    outWS->setOriginalWorkspace(m_inWS, 0);

    // And the intermediate WS one too, if any
    if (m_intermediateWS)
    {
      outWS->setOriginalWorkspace(m_intermediateWS, 1);
      outWS->setTransformFromOriginal(m_transformFromIntermediate, 1);
      outWS->setTransformToOriginal(m_transformToIntermediate, 1);
    }

    // Wrapper to cast to MDEventWorkspace then call the function
    bool IterateEvents = getProperty("IterateEvents");
    if (!m_axisAligned && !IterateEvents)
    {
      g_log.notice() << "Algorithm does not currently support IterateEvents=False if AxisAligned=False. Setting IterateEvents=True." << std::endl;
      IterateEvents = true;
    }

    if (IterateEvents)
    {
      CALL_MDEVENT_FUNCTION(this->binByIterating, m_inWS);
    }
    else
    {
      CALL_MDEVENT_FUNCTION(this->do_centerpointBin, m_inWS);
    }

    // Copy the experiment infos to the output
    IMDEventWorkspace_sptr inEWS = boost::dynamic_pointer_cast<IMDEventWorkspace>(m_inWS);
    outWS->copyExperimentInfos( *inEWS );

    // Save the output
    setProperty("OutputWorkspace", boost::dynamic_pointer_cast<Workspace>(outWS));
  }



} // namespace Mantid
} // namespace MDEvents
