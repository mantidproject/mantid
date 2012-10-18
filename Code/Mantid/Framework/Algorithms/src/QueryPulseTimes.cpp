/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidAlgorithms/QueryPulseTimes.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/VectorHelper.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace Algorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(QueryPulseTimes)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  QueryPulseTimes::QueryPulseTimes()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  QueryPulseTimes::~QueryPulseTimes()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string QueryPulseTimes::name() const { return "QueryPulseTimes";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int QueryPulseTimes::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string QueryPulseTimes::category() const { return "General";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void QueryPulseTimes::initDocs()
  {
    this->setWikiSummary("TODO: Enter a quick description of your algorithm.");
    this->setOptionalMessage("TODO: Enter a quick description of your algorithm.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
  */
  void QueryPulseTimes::init()
  {
    declareProperty(new API::WorkspaceProperty<API::IEventWorkspace>("InputWorkspace","",Direction::Input), "An input workspace.");
    declareProperty(
      new ArrayProperty<double>("Params", boost::make_shared<RebinParamsValidator>()),
      "A comma separated list of first bin boundary, width, last bin boundary. Optionally\n"
      "this can be followed by a comma and more widths and last boundary pairs.\n"
      "Negative width values indicate logarithmic binning.");
    declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace","",Direction::Output), "An output workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
  */
  void QueryPulseTimes::exec()
  {
    using Mantid::DataObjects::EventWorkspace;
    IEventWorkspace_sptr temp = getProperty("InputWorkspace");
    boost::shared_ptr<EventWorkspace> inWS = boost::dynamic_pointer_cast<EventWorkspace>(temp);

    MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace"); // TODO: MUST BE A HISTOGRAM WORKSPACE!

    // retrieve the properties
    const std::vector<double> in_params=getProperty("Params");
    std::vector<double> rb_params;

    // workspace independent determination of length
    const int histnumber = static_cast<int>(inWS->getNumberHistograms());

    // The validator only passes parameters with size 1, or 3xn.  No need to check again here
    if (in_params.size() >= 3)
    {
      // Input are min, delta, max
      rb_params = in_params;

    } 
    else if (in_params.size() == 1)
    {
      double xmin = 0.;
      double xmax = 0.;
      
      Progress sortProg(this,0.0,1.0, histnumber);
      inWS->sortAll(DataObjects::PULSETIME_SORT, &sortProg); 
      bool firstRun = true;
      for(int i = 0; i < histnumber; ++i)
      {
        const IEventList* el = inWS->getEventListPtr(i);
        const int nEvents = el->getNumberEvents();
        if(nEvents > 0)
        {
          double tempMin = el->getPulseTimeMin().totalNanoseconds();
          double tempMax = el->getPulseTimeMax().totalNanoseconds();
          if(firstRun)
          {
            xmin = tempMin;
            xmax = tempMax;
            firstRun = false;
          }
          
          xmin = tempMin < xmin ? tempMin : xmin;
          xmax = tempMax > xmax ? tempMax : xmax;
        }
      }
      g_log.information() << "Using the current min and max as default " << xmin << ", " << xmax << std::endl;

      rb_params.push_back(xmin);
      rb_params.push_back(in_params[0]);
      rb_params.push_back(xmax);

    }

    const bool dist = inWS->isDistribution();

    const bool isHist = inWS->isHistogramData();

    //Initialize progress reporting.
    Progress prog(this,0.0,1.0, histnumber);
    
    MantidVecPtr XValues_new;
    // create new output X axis
    const int ntcnew = VectorHelper::createAxisFromRebinParams(rb_params, XValues_new.access());

    outputWS = WorkspaceFactory::Instance().create("Workspace2D",histnumber,ntcnew,ntcnew-1);
    WorkspaceFactory::Instance().initializeFromParent(inWS, outputWS, true);

    //Go through all the histograms and set the data
    //PARALLEL_FOR2(inWS, outputWS)
    for (int i=0; i < histnumber; ++i)
    {
      //PARALLEL_START_INTERUPT_REGION

      //Set the X axis for each output histogram
      outputWS->setX(i, XValues_new);

      const IEventList* el = inWS->getEventListPtr(i);
      MantidVec y_data, e_data;
      // The EventList takes care of histogramming.
      el->generateHistogramPulseTime(*XValues_new, y_data, e_data);

      //Copy the data over.
      outputWS->dataY(i).assign(y_data.begin(), y_data.end());
      outputWS->dataE(i).assign(e_data.begin(), e_data.end());

      //Report progress
      prog.report(name());
      //PARALLEL_END_INTERUPT_REGION
    }
    //PARALLEL_CHECK_INTERUPT_REGION

    //Copy all the axes
    for (int i=1; i<inWS->axes(); i++)
    {
      outputWS->replaceAxis( i, inWS->getAxis(i)->clone(outputWS.get()) );
      outputWS->getAxis(i)->unit() = inWS->getAxis(i)->unit();
    }

    //Copy the units over too.
    for (int i=0; i < outputWS->axes(); ++i)
    {
      outputWS->getAxis(i)->unit() = inWS->getAxis(i)->unit();
    }
    outputWS->setYUnit(inWS->YUnit());
    outputWS->setYUnitLabel(inWS->YUnitLabel());

    // Assign it to the output workspace property
    setProperty("OutputWorkspace", outputWS);

    return;
  }



} // namespace Algorithms
} // namespace Mantid