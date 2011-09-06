#include "MantidAPI/ImplicitFunctionFactory.h"
#include "MantidGeometry/MDGeometry/MDBoxImplicitFunction.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Utils.h"
#include "MantidMDEvents/BinToMDHistoWorkspace.h"
#include "MantidMDEvents/CoordTransformAffineParser.h"
#include "MantidMDEvents/CoordTransformAligned.h"
#include "MantidMDEvents/IMDBox.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDBoxIterator.h"
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
  DECLARE_ALGORITHM(BinToMDHistoWorkspace)

  using namespace Mantid::Kernel;
  using namespace Mantid::API;
  using namespace Mantid::Geometry;


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  BinToMDHistoWorkspace::BinToMDHistoWorkspace()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  BinToMDHistoWorkspace::~BinToMDHistoWorkspace()
  {
  }


  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void BinToMDHistoWorkspace::initDocs()
  {
    this->setWikiSummary("Take a [[MDEventWorkspace]] and bin into into a dense, multi-dimensional histogram workspace ([[MDHistoWorkspace]]).");
    this->setOptionalMessage("Take a MDEventWorkspace and bin into into a dense, multi-dimensional histogram workspace (MDHistoWorkspace).");
    this->setWikiDescription(
        "This algorithm performs dense binning of the events in multiple dimensions of an input [[MDEventWorkspace]] "
        "and places them into a dense MDHistoWorkspace with 1-4 dimensions."
        "\n\n"
        "The input MDEventWorkspace may have more dimensions than the number of output dimensions. "
        "The names of the dimensions in the DimX, etc. parameters are used to find the corresponding dimensions that will "
        "be created in the output."
        "\n\n"
        "An ImplicitFunction can be defined using the ImplicitFunctionXML parameter; any points NOT belonging inside of the "
        "ImplicitFunction will be set as NaN (not-a-number). "
        "\n\n"
        "As of now, binning is only performed along axes perpendicular to the dimensions defined "
        "in the MDEventWorkspace."
        );
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void BinToMDHistoWorkspace::init()
  {
    std::string dimChars = "XYZT";

    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("InputWorkspace","",Direction::Input), "An input MDEventWorkspace.");

    // --------------- Axis-aligned properties ---------------------------------------
    declareProperty("AxisAligned", true, "Perform binning aligned with the axes of the input MDEventWorkspace?");
    setPropertyGroup("AxisAligned", "Axis-Aligned Binning");
    for (size_t i=0; i<4; i++)
    {
      std::string dim(" "); dim[0] = dimChars[i];
      std::string propName = "AlignedDim" + dim;
      declareProperty(new PropertyWithValue<std::string>(propName,"",Direction::Input),
          "Binning parameters for the " + dim + " dimension.\n"
          "Enter it as a comma-separated list of values with the format: 'name,minimum,maximum,number_of_bins'. Leave blank for NONE.");
      setPropertySettings(propName, new EnabledWhenProperty(this, "AxisAligned", IS_EQUAL_TO, "1") );
      setPropertyGroup(propName, "Axis-Aligned Binning");
    }

    // --------------- NON-Axis-aligned properties ---------------------------------------
//    declareProperty(new PropertyWithValue<std::string>("TransformationXML","",Direction::Input),
//            "XML string describing the coordinate transformation that converts from the MDEventWorkspace dimensions to the output dimensions.\n");
//    setPropertyGroup("TransformationXML", "Non-Aligned Binning");

    for (size_t i=0; i<4; i++)
    {
      std::string dim(" "); dim[0] = dimChars[i];
      std::string propName = "BasisVector" + dim;
      declareProperty(new PropertyWithValue<std::string>(propName,"",Direction::Input),
          "Description of the basis vector of the output dimension " + dim + "."
          "Format: 'name, units, x,y,z,.., length, number_of_bins'.\n"
          "  x,y,z,...: vector definining the basis in the input dimensions space.\n"
          "  length: length of this dimension in the input space.\n"
          "  number_of_bins: separate 'length' into this many bins\n"
          "Leave blank for NONE." );
      setPropertySettings(propName, new EnabledWhenProperty(this, "AxisAligned", IS_EQUAL_TO, "0") );
      setPropertyGroup(propName, "Non-Aligned Binning");
    }
    declareProperty(new PropertyWithValue<std::string>("Origin","",Direction::Input),
        "Origin (in the input workspace) that corresponds to (0,0,0) in the output MDHistoWorkspace.\n"
        "Enter as a comma-separated string." );
    setPropertyGroup("Origin", "Non-Aligned Binning");


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
        "Temporary parameter: true to run in parallel.");
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
  inline void BinToMDHistoWorkspace::binMDBox(MDBox<MDE, nd> * box, size_t * chunkMin, size_t * chunkMax)
  {
    // An array to hold the rotated/transformed coordinates
    coord_t * outCenter = new coord_t[outD];

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

        // To build up the linear index
        size_t linearIndex = 0;
        // To mark VERTEXES outside range
        badOne = false;

        /// Loop through the dimensions on which we bin
        for (size_t bd=0; bd<outD; bd++)
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
      for (size_t bd=0; bd<outD; bd++)
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
  /** Create an implicit function for picking boxes, based on the indexes in the
   * output MDHistoWorkspace.
   * This needs to be in the space of the INPUT MDEventWorkspace
   *
   * @param chunkMin :: the minimum index in each dimension to consider "valid" (inclusive)
   * @param chunkMax :: the maximum index in each dimension to consider "valid" (exclusive)
   * @return MDImplicitFunction
   */
  template<typename MDE, size_t nd>
  MDImplicitFunction * BinToMDHistoWorkspace::getImplicitFunctionForChunk(typename MDEventWorkspace<MDE, nd>::sptr ws,
      size_t * chunkMin, size_t * chunkMax)
  {
    UNUSED_ARG(ws);

    if (m_axisAligned)
    {
      std::vector<coord_t> function_min(nd, -1e50); // default to all space if the dimension is not specified
      std::vector<coord_t> function_max(nd, +1e50); // default to all space if the dimension is not specified
      for (size_t bd=0; bd<outD; bd++)
      {
        // Dimension in the MDEventWorkspace
        size_t d = dimensionToBinFrom[bd];
        function_min[d] = binDimensions[bd]->getX(chunkMin[bd]);
        function_max[d] = binDimensions[bd]->getX(chunkMax[bd]);
      }
      MDBoxImplicitFunction * function = new MDBoxImplicitFunction(function_min, function_max);
      return function;
    }
    else
    {
      // General implicit function
      // TODO: Apply the transform!
      return new MDImplicitFunction;
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Perform binning by iterating through every event and placing them in the output workspace
   *
   * @param ws :: MDEventWorkspace of the given type.
   */
  template<typename MDE, size_t nd>
  void BinToMDHistoWorkspace::binByIterating(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    // Start with signal at 0.0
    outWS->setTo(0.0, 0.0);

    // Cache some data to speed up accessing them a bit
    indexMultiplier = new size_t[outD];
    for (size_t d=0; d<outD; d++)
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
    int chunkNumBins =  int(binDimensions[chunkDimension]->getNBins() / (Mantid::Kernel::ThreadPool::getNumPhysicalCores() * 2));
    if (chunkNumBins < 1) chunkNumBins = 1;

    // Do we actually do it in parallel?
    bool doParallel = getProperty("Parallel");
    if (!doParallel)
      chunkNumBins = int(binDimensions[chunkDimension]->getNBins());

    // Total number of steps
    size_t progNumSteps = 0;

    // Run the chunks in parallel. There is no overlap in the output workspace so it is
    // thread safe to write to it..
    PRAGMA_OMP( parallel for schedule(dynamic,1) if (doParallel) )
    for(int chunk=0; chunk < int(binDimensions[chunkDimension]->getNBins()); chunk += chunkNumBins)
    {
      // Region of interest for this chunk.
      size_t * chunkMin = new size_t[outD];
      size_t * chunkMax = new size_t[outD];
      for (size_t bd=0; bd<outD; bd++)
      {
        // Same limits in the other dimensions
        chunkMin[bd] = 0;
        chunkMax[bd] = binDimensions[bd]->getNBins();
      }
      // Parcel out a chunk in that single dimension dimension
      chunkMin[chunkDimension] = size_t(chunk);
      if (size_t(chunk+chunkNumBins) > binDimensions[chunkDimension]->getNBins())
        chunkMax[chunkDimension] = binDimensions[chunkDimension]->getNBins();
      else
        chunkMax[chunkDimension] = size_t(chunk+chunkNumBins);

      // Build an implicit function (it needs to be in the space of the MDEventWorkspace)
      MDImplicitFunction * function = this->getImplicitFunctionForChunk<MDE,nd>(ws, chunkMin, chunkMax);

      // Use getBoxes() to get an array with a pointer to each box
      std::vector<IMDBox<MDE,nd>*> boxes;
      // Leaf-only; no depth limit; with the implicit function passed to it.
      ws->getBox()->getBoxes(boxes, 1000, true, function);

      // For progress reporting, the # of boxes
      if (prog)
      {
        PARALLEL_CRITICAL(BinToMDHistoWorkspace_progress)
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
    } // for each chunk in parallel



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
  void BinToMDHistoWorkspace::do_centerpointBin(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    bool DODEBUG = true;

    CPUTimer tim;

    // Number of output binning dimensions found
    size_t outD = binDimensions.size();

    //Since the costs are not known ahead of time, use a simple FIFO buffer.
    ThreadScheduler * ts = new ThreadSchedulerFIFO();

    // Create the threadpool with: all CPUs, a progress reporter
    ThreadPool tp(ts, 0, prog);

    // Big efficiency gain is obtained by grouping a few bins per task.
    size_t binsPerTask = 100;

    // For progress reporting, the approx  # of tasks
    if (prog)
      prog->setNumSteps( int(outWS->getNPoints()/100) );

    // The root-level box.
    IMDBox<MDE,nd> * rootBox = ws->getBox();

    // This is the limit to loop over in each dimension
    size_t * index_max = new size_t[outD];
    for (size_t bd=0; bd<outD; bd++) index_max[bd] = binDimensions[bd]->getNBins();

    // Cache a calculation to convert indices x,y,z,t into a linear index.
    size_t * index_maker = new size_t[outD];
    Utils::NestedForLoop::SetUpIndexMaker(outD, index_maker, index_max);

    int numPoints = int(outWS->getNPoints());

    // Run in OpenMP with dynamic scheduling and a smallish chunk size (binsPerTask)
    // Right now, not parallel for file-backed systems.
    bool fileBacked = (ws->getBoxController()->getFile() != NULL);
    PRAGMA_OMP(parallel for schedule(dynamic, binsPerTask) if (!fileBacked)  )
    for (int i=0; i < numPoints; i++)
    {
      PARALLEL_START_INTERUPT_REGION

      size_t linear_index = size_t(i);
      // nd >= outD in all cases so this is safe.
      size_t index[nd];

      // Get the index at each dimension for this bin.
      Utils::NestedForLoop::GetIndicesFromLinearIndex(outD, linear_index, index_maker, index_max, index);

      // Construct the bin and its coordinates
      MDBin<MDE,nd> bin;
      for (size_t bd=0; bd<outD; bd++)
      {
        // Index in this binning dimension (i_x, i_y, etc.)
        size_t idx = index[bd];
        // Dimension in the MDEventWorkspace
        size_t d = dimensionToBinFrom[bd];
        // Corresponding extents
        bin.m_min[d] = binDimensions[bd]->getX(idx);
        bin.m_max[d] = binDimensions[bd]->getX(idx+1);
      }
      bin.m_index = linear_index;

      bool dimensionsUsed[nd];
      for (size_t d=0; d<nd; d++)
        dimensionsUsed[d] = (d<3);

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
        outWS->setErrorAt(linear_index, bin.m_errorSquared);
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
  /** Generate the MDHistoDimension and basis vector for a given string from BasisVectorX etc.
   *
   *  "Format: 'name, units, x,y,z,.., length, number_of_bins'.\n"
   *
   * @param str :: name,number_of_bins
   * @return a vector describing the basis vector in the input dimensions.
   */
  void BinToMDHistoWorkspace::makeBasisVectorFromString(const std::string & str)
  {
    if (str.empty())
      return;

    std::string name, id, units;
    double min, max;
    int numBins = 0;
    std::vector<std::string> strs;
    boost::split(strs, str, boost::is_any_of(","));
    if (strs.size() != this->in_ws->getNumDims() + 4)
      throw std::invalid_argument("Wrong number of values (expected 4 + # of input dimensions) in the dimensions string: " + str);
    // Extract the arguments
    name = Strings::strip(strs[0]);
    id = name;
    units = Strings::strip(strs[1]);
    Strings::convert(strs[ strs.size()-1 ], numBins);
    max = double(numBins);
    if (name.size() == 0)
      throw std::invalid_argument("Name should not be blank.");
    if (numBins < 1)
      throw std::invalid_argument("Number of bins should be >= 1.");

    double length = 0.0;
    Strings::convert(strs[ strs.size()-2 ], length);
    min = 0.0;
    max = length;
    // Scaling factor, to convert from units in the inDim to the output BIN number
    double scaling = double(numBins) / length;

    // Create the basis vector with the right # of dimensions
    VMD basis(this->in_ws->getNumDims());
    for (size_t d=0; d<this->in_ws->getNumDims(); d++)
      Strings::convert(strs[d+2], basis[d]);

    // Create the output dimension
    MDHistoDimension_sptr out(new MDHistoDimension(name, id, units, min, max, numBins));

    // Put both in the algo for future use
    m_bases.push_back(basis);
    binDimensions.push_back(out);
    m_scaling.push_back(scaling);
  }


  //----------------------------------------------------------------------------------------------
  /** Loads the dimensions and create the coordinate transform, using the inputs.
   * This is for the general (i.e. non-aligned) case
   */
  void BinToMDHistoWorkspace::createTransform()
  {
    // Number of input dimensions
    size_t inD = in_ws->getNumDims();

    // Create the dimensions based on the strings from the user
    std::string dimChars = "XYZT";
    for (size_t i=0; i<4; i++)
    {
      std::string propName = "BasisVectorX"; propName[11] = dimChars[i];
      try
      { makeBasisVectorFromString( getPropertyValue(propName) ); }
      catch (std::exception & e)
      { throw std::invalid_argument("Error parsing the " + propName + " parameter: " + std::string(e.what()) ); }
    }
    // Number of output binning dimensions found
    outD = binDimensions.size();

    // Get the origin
    VMD origin;
    try
    { origin = VMD( getPropertyValue("Origin") ); }
    catch (std::exception & e)
    { throw std::invalid_argument("Error parsing the Origin parameter: " + std::string(e.what()) ); }
    if (origin.getNumDims() != inD)
      throw std::invalid_argument("The number of dimensions in the Origin parameter is not consistent with the number of dimensions in the input workspace.");

    // Validate
    if (outD == 0)
      throw std::runtime_error("No output dimensions were found in the MDEventWorkspace. Cannot bin!");
    if (outD > inD)
      throw std::runtime_error("More output dimensions were specified than input dimensions exist in the MDEventWorkspace. Cannot bin!");
    if (m_scaling.size() != outD)
      throw std::runtime_error("Inconsistent number of entries in scaling vector.");

    // Create the CoordTransformAffine with these basis vectors
    CoordTransformAffine * ct = new CoordTransformAffine(inD, outD);
    ct->buildOrthogonal(origin, this->m_bases, VMD(this->m_scaling) ); // note the scaling makes the coordinate correspond to a bin index
    this->m_transform = ct;

    // Transformation original->binned
    std::vector<double> unitScaling(outD, 1.0);
    CoordTransformAffine * ctFrom = new CoordTransformAffine(inD, outD);
    ctFrom->buildOrthogonal(origin, this->m_bases, VMD(unitScaling) );
    m_transformFromOriginal = ctFrom;

    // Validate
    if (m_transform->getInD() != inD)
      throw std::invalid_argument("The number of input dimensions in the CoordinateTransform object is not consistent with the number of dimensions in the input workspace.");
    if (m_transform->getOutD() != outD)
      throw std::invalid_argument("The number of output dimensions in the CoordinateTransform object is not consistent with the number of dimensions specified in the OutDimX, etc. properties.");
  }






  //----------------------------------------------------------------------------------------------
  /** Generate a MDHistoDimension_sptr from a comma-sep string (for AlignedDimX, etc.)
   *
   * @param str :: name,minimum,maximum,number_of_bins
   * @return
   */
  MDHistoDimension_sptr makeMDHistoDimensionFromString(const std::string & str)
  {
    if (str.empty())
    {
      // Make a blank dimension
      MDHistoDimension_sptr out;
      return out;
    }
    else
    {
      std::string name, id;
      double min, max;
      int numBins = 0;
      std::vector<std::string> strs;
      boost::split(strs, str, boost::is_any_of(","));
      if (strs.size() != 4)
        throw std::invalid_argument("Wrong number of values (4 are expected) in the dimensions string: " + str);
      // Extract the arguments
      name = Strings::strip(strs[0]);
      id = name;
      Strings::convert(strs[1], min);
      Strings::convert(strs[2], max);
      Strings::convert(strs[3], numBins);
      if (name.size() == 0)
        throw std::invalid_argument("Name should not be blank.");
      if (min >= max)
        throw std::invalid_argument("Min should be > max.");
      if (numBins < 1)
        throw std::invalid_argument("Number of bins should be >= 1.");

      MDHistoDimension_sptr out(new MDHistoDimension(name, id, "", min, max, numBins));
      return out;
    }
  }
  //----------------------------------------------------------------------------------------------
  /** Using the parameters, create a coordinate transformation
   * for aligned cuts
   */
  void BinToMDHistoWorkspace::createAlignedTransform()
  {
    // Create the dimensions based on the strings from the user
    std::string dimChars = "XYZT";
    for (size_t i=0; i<4; i++)
    {
      std::string propName = "AlignedDimX"; propName[10] = dimChars[i];
      MDHistoDimension_sptr binDim = makeMDHistoDimensionFromString( getPropertyValue(propName));
      if (binDim)
      {
        // (valid pointer?)
        if (binDim->getNBins() == 0)
          throw std::runtime_error("Dimension " + binDim->getName() + " was set to have 0 bins. Cannot continue.");

        // We have to find the dimension in the INPUT workspace to set to the OUTPUT workspace
        try {
          size_t dim_index = in_ws->getDimensionIndexByName(binDim->getName());
          dimensionToBinFrom.push_back(dim_index);
        }
        catch (std::runtime_error &)
        {
          // The dimension was not found, so we are not binning across it.
          if (binDim->getNBins() > 1)
            throw std::runtime_error("Dimension " + binDim->getName() + " was not found in the MDEventWorkspace and has more than one bin! Cannot continue.");
        }
        // This is a dimension we'll use in the output
        binDimensions.push_back(binDim);
      }
    }
    // Number of output binning dimensions found
    outD = binDimensions.size();
    // Number of input dimension
    size_t inD = in_ws->getNumDims();

    // Validate
    if (outD == 0)
      throw std::runtime_error("No output dimensions were found in the MDEventWorkspace. Cannot bin!");
    if (outD > inD)
      throw std::runtime_error("More output dimensions were specified than input dimensions exist in the MDEventWorkspace. Cannot bin!");

    // Now we build the coordinate transformation object
    m_origin = VMD(inD);
    m_bases.clear();
    std::vector<coord_t> origin(outD), scaling(outD);
    for (size_t d=0; d<outD; d++)
    {
      origin[d] = binDimensions[d]->getMinimum();
      scaling[d] = coord_t(1.0) / binDimensions[d]->getBinWidth();
      // Origin in the input
      m_origin[ dimensionToBinFrom[d] ] = origin[d];
      // Create a unit basis vector that corresponds to this
      VMD basis(inD);
      basis[ dimensionToBinFrom[d] ] = 1.0;
      m_bases.push_back(basis);
    }

    m_transform = new CoordTransformAligned(in_ws->getNumDims(), outD,
        dimensionToBinFrom, origin, scaling);

    // Transformation original->binned
    std::vector<double> unitScaling(outD, 1.0);
    m_transformFromOriginal = new CoordTransformAligned(in_ws->getNumDims(), outD,
        dimensionToBinFrom, origin, unitScaling);
  }









  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void BinToMDHistoWorkspace::exec()
  {
    // Input MDEventWorkspace
    in_ws = getProperty("InputWorkspace");

    // Is the transformation aligned with axes?
    m_axisAligned = getProperty("AxisAligned");

    // Create the coordinate transformation
    m_transform = NULL;
    if (m_axisAligned)
      this->createAlignedTransform();
    else
      this->createTransform();

    // De serialize the implicit function
    std::string ImplicitFunctionXML = getPropertyValue("ImplicitFunctionXML");
    implicitFunction = NULL;
    if (!ImplicitFunctionXML.empty())
      implicitFunction = Mantid::API::ImplicitFunctionFactory::Instance().createUnwrapped(ImplicitFunctionXML);

    prog = new Progress(this, 0, 1.0, 1); // This gets deleted by the thread pool; don't delete it in here.

    // Create the dense histogram. This allocates the memory
    outWS = MDHistoWorkspace_sptr(new MDHistoWorkspace(binDimensions));

    // Saves the geometry transformation from original to binned in the workspace
    outWS->setTransformFromOriginal( this->m_transformFromOriginal );
    //outWS->setTransformToOriginal( this->m_transformToOriginal );
    for (size_t i=0; i<m_bases.size(); i++)
      outWS->setBasisVector(i, m_bases[i]);
    outWS->setOrigin( this->m_origin );

    // Wrapper to cast to MDEventWorkspace then call the function
    bool IterateEvents = getProperty("IterateEvents");
    if (IterateEvents)
    {
      CALL_MDEVENT_FUNCTION(this->binByIterating, in_ws);
    }
    else
    {
      CALL_MDEVENT_FUNCTION(this->do_centerpointBin, in_ws);
    }

    // Save the output
    setProperty("OutputWorkspace", boost::dynamic_pointer_cast<Workspace>(outWS));
  }



} // namespace Mantid
} // namespace MDEvents
