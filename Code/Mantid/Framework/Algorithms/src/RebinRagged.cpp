/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include <sstream>
#include "MantidAlgorithms/RebinRagged.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/VectorHelper.h"

namespace Mantid
{
namespace Algorithms
{
  using namespace API;
  using namespace DataObjects;
  using namespace Kernel;
  using std::map;
  using std::pair;
  using std::string;
  using std::stringstream;
  using std::vector;

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(RebinRagged)
  


  //----------------------------------------------------------------------------------------------
  /// Constructor
  RebinRagged::RebinRagged(): m_useLogBinning(true), m_preserveEvents(true), m_numBins(0),
    m_isDistribution(false), m_isHistogram(true)
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /// Destructor
  RebinRagged::~RebinRagged()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string RebinRagged::name() const
  {
    return "RebinRagged";
  }
  
  /// Algorithm's version for identification. @see Algorithm::version
  int RebinRagged::version() const
  {
    return 1;
  }
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string RebinRagged::category() const
  {
    return "Transforms\\Rebin";
  }

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void RebinRagged::initDocs()
  {
    this->setWikiSummary("TODO: Enter a quick description of your algorithm.");
    this->setOptionalMessage("TODO: Enter a quick description of your algorithm.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void RebinRagged::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input), "An input workspace.");
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace.");

    declareProperty(new ArrayProperty<double>("XMin"),
                    "A comma separated list of the XMin for every spectrum. (Optional)");
    declareProperty(new ArrayProperty<double>("XMax"),
                    "A comma separated list of the XMax for every spectrum. (Optional)");

    auto min = boost::make_shared<BoundedValidator<int> >();
    min->setLower(1);
    declareProperty("NumberBins", 0, min, "Number of bins to split up each spectrum into.");
    declareProperty("LogBinning", false, "Use logorithmic binning. If false use constant step sizes.");

    declareProperty("PreserveEvents", true, "Keep the output workspace as an EventWorkspace, if the input has events (default).\n"
        "If the input and output EventWorkspace names are the same, only the X bins are set, which is very quick.\n"
        "If false, then the workspace gets converted to a Workspace2D histogram.");
  }

  map<string, string> RebinRagged::validateInputs()
  {
    map<string, string> errors;
    vector<double> xmins = getProperty("XMin");
    vector<double> xmaxs = getProperty("XMax");
    if ((!xmins.empty()) && (!xmaxs.empty()))
    {
      if (xmins.size() != xmaxs.size())
      {
        stringstream msg;
        msg << "XMin and XMax do not define same number of spectra ("
            << xmins.size() << " != " << xmaxs.size() << ")";
        errors.insert(pair<string,string>("XMax", msg.str()));
      }
      else
      {
        size_t size = xmins.size();
        for (size_t i = 0; i < size; ++i)
        {
          if (xmins[i] >= xmaxs[i])
          {
            stringstream msg;
            msg << "XMin (" << xmins[i] << ") cannot be greater than XMax ("
                << xmaxs[i] << ")";
            errors.insert(pair<string,string>("XMax", msg.str()));
          }
        }
      }
    }

    return errors;
  }

  /**
   * Determine the min and max x-values for each spectrum and error check the pairs.
   *
   * @param inputWS The workspace to check the numbers for.
   * @param xmins The input/output that will hold the x-mins.
   * @param xmaxs The input/output that will hold the x-maxs.
   *
   * @return Any error messages generated during the execution. If empty everything
   * went according to plan.
   */
  string determineXMinMax(MatrixWorkspace_sptr inputWS,
                          vector<double>& xmins, vector<double>& xmaxs)
  {
    bool updateXMins = xmins.empty(); // they weren't set
    bool updateXMaxs = xmaxs.empty(); // they weren't set

    stringstream msg;

    size_t numSpectra = inputWS->getNumberHistograms();
    for (size_t i=0; i < numSpectra; ++i)
    {
      // determine ranges if necessary
      if (updateXMins || updateXMaxs) {
        const MantidVec& xvalues = inputWS->getSpectrum(i)->dataX();
        if (updateXMins)
          xmins.push_back(xvalues.front());
        if (updateXMaxs)
          xmaxs.push_back(xvalues.back());
      }

      // error check the ranges
      if (xmins[i] >= xmaxs[i])
      {
        if (!msg.str().empty())
          msg << ", ";
        msg << "at wksp_index=" << i << " XMin >= XMax ("
            << xmins[i] << " >= " << xmaxs[i] << ")";
      }
    }

    return msg.str(); // empty string means nothing went wrong
  }

  /**
   * Use the binning information to generate a x-axis.
   *
   * @param xValues The new x-axis.
   * @param xmin The x-min to be used.
   * @param xmax The x-max to be used.
   *
   * @return The final delta value (absolute value).
   */
  double RebinRagged::determineBinning(MantidVec& xValues, const double xmin, const double xmax)
  {
    xValues.clear(); // clear out the x-values

    int numBoundaries(0);
    int expNumBoundaries(m_numBins);
    if (!m_isDistribution)
      expNumBoundaries += 1; // should be one more bin boundary for histograms

    vector<double> params; // xmin, delta, xmax
    params.push_back(xmin);
    params.push_back(0.); // dummy delta value
    params.push_back(xmax);

    // constant binning is easy
    if (m_useLogBinning)
    {
      const int MAX_ITER(100); // things went wrong if we get this far

      // starting delta value assuming everything happens exactly
      double delta = (log(xmax) - log(xmin))/static_cast<double>(m_numBins);
      double shift = .1;
      int sign = 0;
      for (int numIter = 0; numIter < MAX_ITER; ++numIter)
      {
        params[1] = -1. * delta;
        numBoundaries = VectorHelper::createAxisFromRebinParams(params, xValues, true);

        if (numBoundaries == expNumBoundaries)
          break; // happy ending

        if (numBoundaries > expNumBoundaries) // too few points
        {
          delta *= (1. + shift);
          if (sign < 0)
            shift *= .9;
          sign = 1;
        }
        else // too many points
        {
          delta *= (1. - shift);
          if (sign > 0)
            shift *= .9;
          sign = -1;
        }
      }
    }
    else
    {
      params[1] = (xmax - xmin) / static_cast<double>(m_numBins);
      numBoundaries = VectorHelper::createAxisFromRebinParams(params, xValues, true);
    }

    if (numBoundaries != expNumBoundaries)
    {
      g_log.warning() << "Did not generate the requested number of bins: generated "
                      << (numBoundaries-1) << " requested " << m_numBins << "\n";
    }

    // return the delta value so the caller can do debug printing
    return params[1];
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void RebinRagged::exec()
  {
    // generically having access to the input workspace is a good idea
    MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
    MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
    bool inPlace = (inputWS == outputWS); // Rebinning in-place
    m_isDistribution = inputWS->isDistribution();
    m_isHistogram = inputWS->isHistogramData();
    int numSpectra = static_cast<int>(inputWS->getNumberHistograms());

    // the easy parameters
    m_useLogBinning  = getProperty("LogBinning");
    m_numBins        = getProperty("NumberBins");
    m_preserveEvents = getProperty("PreserveEvents");

    // determine the xmin/xmax for the workspace
    vector<double> xmins = getProperty("XMin");
    vector<double> xmaxs = getProperty("XMax");
    string error = determineXMinMax(inputWS, xmins, xmaxs);
    if (!error.empty())
      throw std::runtime_error(error);

    // start doing actual work
    EventWorkspace_const_sptr inputEventWS = boost::dynamic_pointer_cast<const EventWorkspace>(inputWS);
    if (inputEventWS != NULL)
    {
      if (m_preserveEvents)
      {
        EventWorkspace_sptr outputEventWS = boost::dynamic_pointer_cast<EventWorkspace>(outputWS);
        if (inPlace)
        {
          g_log.debug() << "Rebinning event workspace in place\n";
        }
        else
        {
          g_log.debug() << "Rebinning event workspace in place\n";

          // copy the event workspace to a new EventWorkspace
          outputEventWS = boost::dynamic_pointer_cast<EventWorkspace>(
              API::WorkspaceFactory::Instance().create("EventWorkspace", inputWS->getNumberHistograms(), 2, 1));
          // copy geometry over.
          API::WorkspaceFactory::Instance().initializeFromParent(inputEventWS, outputEventWS, false);
          // copy over the data as well.
          outputEventWS->copyDataFrom( (*inputEventWS) );
        }

        // initialize progress reporting.
        Progress prog(this,0.0,1.0, numSpectra);

        // do the rebinning
        PARALLEL_FOR2(inputEventWS, outputWS)
        for (int wkspIndex = 0; wkspIndex < numSpectra; ++wkspIndex)
        {
          PARALLEL_START_INTERUPT_REGION
          MantidVec xValues;
          double delta = this->determineBinning(xValues, xmins[wkspIndex], xmaxs[wkspIndex]);
          g_log.debug() << "delta[wkspindex=" << wkspIndex << "] = " << delta << "\n";
          outputEventWS->getSpectrum(wkspIndex)->setX(xValues);
          prog.report(name()); //Report progress
          PARALLEL_END_INTERUPT_REGION
        }
        PARALLEL_CHECK_INTERUPT_REGION

        this->setProperty("OutputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(outputEventWS));
      } // end if (m_preserveEvents)
      else // event workspace -> matrix workspace
      {
        //--------- Different output, OR you're inplace but not preserving Events --- create a Workspace2D -------
        g_log.information() << "Creating a Workspace2D from the EventWorkspace " << inputEventWS->getName() << ".\n";

        //Create a Workspace2D
        // This creates a new Workspace2D through a torturous route using the WorkspaceFactory.
        // The Workspace2D is created with an EMPTY CONSTRUCTOR
        outputWS = WorkspaceFactory::Instance().create("Workspace2D",numSpectra,m_numBins,m_numBins-1);
        WorkspaceFactory::Instance().initializeFromParent(inputWS, outputWS, true);

        //Initialize progress reporting.
        Progress prog(this,0.0,1.0, numSpectra);

        //Go through all the histograms and set the data
        PARALLEL_FOR2(inputEventWS, outputWS)
        for (int wkspIndex=0; wkspIndex < numSpectra; ++wkspIndex)
        {
          PARALLEL_START_INTERUPT_REGION

          //Set the X axis for each output histogram
          MantidVec xValues;
          double delta = this->determineBinning(xValues, xmins[wkspIndex], xmaxs[wkspIndex]);
          g_log.information() << "delta[wkspindex=" << wkspIndex << "] = " << delta << "\n";
          outputWS->setX(wkspIndex, xValues);

          //Get a const event list reference. inputEventWS->dataY() doesn't work.
          const EventList& el = inputEventWS->getEventList(wkspIndex);
          MantidVec y_data, e_data;
          // The EventList takes care of histogramming.
          el.generateHistogram(xValues, y_data, e_data);

          //Copy the data over.
          outputWS->dataY(wkspIndex).assign(y_data.begin(), y_data.end());
          outputWS->dataE(wkspIndex).assign(e_data.begin(), e_data.end());

          //Report progress
          prog.report(name());
          PARALLEL_END_INTERUPT_REGION
        }
        PARALLEL_CHECK_INTERUPT_REGION

        //Copy all the axes
        for (int i=1; i<inputWS->axes(); i++)
        {
          outputWS->replaceAxis( i, inputWS->getAxis(i)->clone(outputWS.get()) );
          outputWS->getAxis(i)->unit() = inputWS->getAxis(i)->unit();
        }

        //Copy the units over too.
        for (int i=0; i < outputWS->axes(); ++i)
          outputWS->getAxis(i)->unit() = inputWS->getAxis(i)->unit();
        outputWS->setYUnit(inputEventWS->YUnit());
        outputWS->setYUnitLabel(inputEventWS->YUnitLabel());

        // Assign it to the output workspace property
        setProperty("OutputWorkspace", outputWS);
      }
      return;
    } // end if (inputeventWS != NULL)

    throw std::runtime_error("Only event workspace mode is implemented");
  }

} // namespace Algorithms
} // namespace Mantid
