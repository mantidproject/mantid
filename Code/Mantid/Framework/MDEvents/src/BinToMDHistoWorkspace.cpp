#include "MantidAPI/ImplicitFunction.h"
#include "MantidAPI/ImplicitFunctionFactory.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Utils.h"
#include "MantidMDEvents/BinToMDHistoWorkspace.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include <boost/algorithm/string.hpp>
#include "MantidMDEvents/IMDBox.h"

using Mantid::Kernel::CPUTimer;

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
    std::string dimHelp = "Enter it as a comma-separated list of values with the format: 'name,minimum,maximum,number_of_bins'. Leave blank for NONE.";
    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("InputWorkspace","",Direction::InOut), "An input MDEventWorkspace.");
    declareProperty(new PropertyWithValue<std::string>("DimX","",Direction::Input), "Binning parameters for the X dimension.\n" + dimHelp);
    declareProperty(new PropertyWithValue<std::string>("DimY","",Direction::Input), "Binning parameters for the Y dimension.\n" + dimHelp);
    declareProperty(new PropertyWithValue<std::string>("DimZ","",Direction::Input), "Binning parameters for the Z dimension.\n" + dimHelp);
    declareProperty(new PropertyWithValue<std::string>("DimT","",Direction::Input), "Binning parameters for the T dimension.\n" + dimHelp);
    declareProperty(new PropertyWithValue<std::string>("ImplicitFunctionXML","",Direction::Input), "XML string describing the implicit function determining which bins to use.");
    declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output), "A name for the output MDHistoWorkspace.");
  }


  //----------------------------------------------------------------------------------------------
  /** Generate a MDHistoDimension_sptr from a comma-sep string.
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


  //-----------------------------------------------------------------------------------------------
  /** Performs template argument deduction and uses local call to fabricate an adapter instance of Point3D.
   * Templated function may have a more suitable location.
   */
  template <typename AdapteeType>
  Mantid::API::Point3D* makePoint3D(const AdapteeType& adaptee)
  {

    //Local adapter.
    class PointAdapter : public Mantid::API::Point3D
    {
    private:
      AdapteeType m_adaptee;
    public:
      PointAdapter(const AdapteeType& adaptee) : m_adaptee(adaptee)
      {
      }
      virtual double getX() const
      {
        return m_adaptee.m_min[0] + ((m_adaptee.m_max[0] - m_adaptee.m_min[0]) /2);
      }
      virtual double getY() const
      {
        return m_adaptee.m_min[1] + ((m_adaptee.m_max[1] - m_adaptee.m_min[1]) /2);
      }
      virtual double getZ() const
      {
        return m_adaptee.m_min[2] + ((m_adaptee.m_max[2] - m_adaptee.m_min[2]) /2);
      }
    };
    return new PointAdapter(adaptee);
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
    bool DODEBUG = false;
    CPUTimer tim;

    // Thin down the input dimensions for any invalid ones
    std::vector<MDHistoDimension_sptr> binDimensions;
    std::vector<size_t> dimensionToBinFrom;
    for (size_t i = 0; i < binDimensionsIn.size(); ++i)
    {
      if (binDimensionsIn[i]) // (valid pointer?)
      {
        if (binDimensionsIn[i]->getNBins() == 0)
          throw std::runtime_error("Dimension " + binDimensionsIn[i]->getName() + " was set to have 0 bins. Cannot continue.");

        try {
          size_t dim_index = ws->getDimensionIndexByName(binDimensionsIn[i]->getName());
          dimensionToBinFrom.push_back(dim_index);
          binDimensions.push_back(binDimensionsIn[i]);
        }
        catch (std::runtime_error &)
        {
          // The dimension was not found, so we are not binning across it. TODO: Log message?
          if (binDimensionsIn[i]->getNBins() > 1)
            throw std::runtime_error("Dimension " + binDimensionsIn[i]->getName() + " was not found in the MDEventWorkspace and has more than one bin! Cannot continue.");
        }
      }
    }
    // Number of output binning dimensions found
    size_t numBD = binDimensions.size();

    // Create the dense histogram. This allocates the memory
    outWS = MDHistoWorkspace_sptr(new MDHistoWorkspace(binDimensions));
    if (DODEBUG) std::cout << tim << " to create the MDHistoWorkspace.\n";

    if (numBD == 0)
      throw std::runtime_error("No output dimensions were found in the MDEventWorkspace. Cannot bin!");
    if (numBD > nd)
      throw std::runtime_error("More output dimensions were specified than input dimensions exist in the MDEventWorkspace. Cannot bin!");

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
    size_t * index_max = Utils::nestedForLoopSetUp(numBD);
    for (size_t bd=0; bd<numBD; bd++) index_max[bd] = binDimensions[bd]->getNBins();
    // Cache a calculation to convert indices x,y,z,t into a linear index.
    size_t * index_maker = Utils::nestedForLoopSetUpIndexMaker(numBD, index_max);

    int numPoints = int(outWS->getNPoints());
    // Run in OpenMP with dynamic scheduling and a smallish chunk size (binsPerTask)
    PRAGMA_OMP(parallel for schedule(dynamic, binsPerTask))
    for (int i=0; i < numPoints; i++)
    {
      size_t linear_index = size_t(i);
      // nd >= numBD in all cases so this is safe.
      size_t index[nd];

      // Get the index at each dimension for this bin.
      Utils::nestedForLoopGetIndicesFromLinearIndex(numBD, linear_index, index_maker, index_max, index);

      // Construct the bin and its coordinates
      MDBin<MDE,nd> bin;
      for (size_t bd=0; bd<numBD; bd++)
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

      // Check if the bin is in the ImplicitFunction (if any)
      bool binContained = true;
      if (implicitFunction)
      {
        binContained = implicitFunction->evaluate(makePoint3D(bin));
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
    } // (for each linear index)

    if (DODEBUG) std::cout << tim << " to run the openmp loop.\n";

    delete index_max;
    delete index_maker;
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void BinToMDHistoWorkspace::exec()
  {
    // Create the dimensions based on the strings from the user
    binDimensionsIn.push_back( makeMDHistoDimensionFromString( getPropertyValue("DimX")) );
    binDimensionsIn.push_back( makeMDHistoDimensionFromString( getPropertyValue("DimY")) );
    binDimensionsIn.push_back( makeMDHistoDimensionFromString( getPropertyValue("DimZ")) );
    binDimensionsIn.push_back( makeMDHistoDimensionFromString( getPropertyValue("DimT")) );

    IMDEventWorkspace_sptr in_ws = getProperty("InputWorkspace");

    // De serialize the implicit function
    std::string ImplicitFunctionXML = getPropertyValue("ImplicitFunctionXML");
    implicitFunction = NULL;
    if (!ImplicitFunctionXML.empty())
    {
      implicitFunction = Mantid::API::ImplicitFunctionFactory::Instance().createUnwrapped(ImplicitFunctionXML);
    }

    prog = new Progress(this, 0, 1.0, 1); // This gets deleted by the thread pool; don't delete it in here.

    // Wrapper to cast to MDEventWorkspace then call the function
    CALL_MDEVENT_FUNCTION(this->do_centerpointBin, in_ws);

    // Save the output
    setProperty("OutputWorkspace", boost::dynamic_pointer_cast<Workspace>(outWS));

  }



} // namespace Mantid
} // namespace MDEvents
