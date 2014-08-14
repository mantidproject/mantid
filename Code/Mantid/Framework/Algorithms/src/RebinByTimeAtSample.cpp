#include "MantidAlgorithms/RebinByTimeAtSample.h"

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/Unit.h"
#include <boost/make_shared.hpp>
#include <algorithm>

namespace Mantid
{
  namespace Algorithms
  {
    using namespace Mantid::Kernel;
    using namespace Mantid::API;

    /**
     Helper method to transform a MantidVector containing absolute times in nanoseconds to relative times in seconds given an offset.
     */
    class ConvertToRelativeTime: public std::unary_function<const MantidVec::value_type&,
        MantidVec::value_type>
    {
    private:
      double m_offSet;
    public:
      ConvertToRelativeTime(const DateAndTime& offSet) :
          m_offSet(static_cast<double>(offSet.totalNanoseconds()) * 1e-9)
      {
      }
      MantidVec::value_type operator()(const MantidVec::value_type& absTNanoSec)
      {
        return (absTNanoSec * 1e-9) - m_offSet;
      }
    };



    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(RebinByTimeAtSample)

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    RebinByTimeAtSample::RebinByTimeAtSample()
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    RebinByTimeAtSample::~RebinByTimeAtSample()
    {
    }

    //----------------------------------------------------------------------------------------------

    /// Algorithm's version for identification. @see Algorithm::version
    int RebinByTimeAtSample::version() const
    {
      return 1;
    }
    ;

    /// Algorithm's category for identification. @see Algorithm::category
    const std::string RebinByTimeAtSample::category() const
    {
      return "Transforms\\Rebin;Events\\EventFiltering";
    }

    /// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
    const std::string RebinByTimeAtSample::summary() const
    {
      return "Rebins with an x-axis of absolute time at sample for comparing event arrival time at the sample environment.";
    }

    const std::string RebinByTimeAtSample::name() const
    {
      return "RebinByTimeAtSample";
    }

    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
     */
    void RebinByTimeAtSample::exec()
    {
      using Mantid::DataObjects::EventWorkspace;
      IEventWorkspace_sptr inWS = getProperty("InputWorkspace");
      if (!boost::dynamic_pointer_cast<EventWorkspace>(inWS))
      {
        throw std::invalid_argument("RebinByPulseTimes requires an EventWorkspace as an input.");
      }

      MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");

      // retrieve the properties
      const std::vector<double> inParams = getProperty("Params");
      std::vector<double> rebinningParams;

      // workspace independent determination of length
      const int histnumber = static_cast<int>(inWS->getNumberHistograms());

      const uint64_t nanoSecondsInASecond = static_cast<uint64_t>(1e9);
      const DateAndTime runStartTime = inWS->run().startTime();
      // The validator only passes parameters with size 1, or 3xn.

      double tStep = 0;
      if (inParams.size() >= 3)
      {
        // Use the start of the run to offset the times provided by the user. pulse time of the events are absolute.
        const DateAndTime startTime = runStartTime + inParams[0];
        const DateAndTime endTime = runStartTime + inParams[2];
        // Rebinning params in nanoseconds.
        rebinningParams.push_back(static_cast<double>(startTime.totalNanoseconds()));
        tStep = inParams[1] * nanoSecondsInASecond;
        rebinningParams.push_back(tStep);
        rebinningParams.push_back(static_cast<double>(endTime.totalNanoseconds()));
      }
      else if (inParams.size() == 1)
      {
        const uint64_t xmin = inWS->getTimeAtSampleMin().totalNanoseconds();
        const uint64_t xmax = inWS->getTimeAtSampleMax().totalNanoseconds();

        rebinningParams.push_back(static_cast<double>(xmin));
        tStep = inParams[0] * nanoSecondsInASecond;
        rebinningParams.push_back(tStep);
        rebinningParams.push_back(static_cast<double>(xmax));
      }

      // Validate the timestep.
      if (tStep <= 0)
      {
        throw std::invalid_argument("Cannot have a timestep less than or equal to zero.");
      }

      //Initialize progress reporting.
      Progress prog(this, 0.0, 1.0, histnumber);

      MantidVecPtr XValues_new;
      // create new X axis, with absolute times in seconds.
      const int ntcnew = VectorHelper::createAxisFromRebinParams(rebinningParams, XValues_new.access());

      ConvertToRelativeTime transformToRelativeT(runStartTime);

      // Transform the output into relative times in seconds.
      MantidVec OutXValues_scaled(XValues_new->size());
      std::transform(XValues_new->begin(), XValues_new->end(), OutXValues_scaled.begin(),
          transformToRelativeT);

      outputWS = WorkspaceFactory::Instance().create("Workspace2D", histnumber, ntcnew, ntcnew - 1);
      WorkspaceFactory::Instance().initializeFromParent(inWS, outputWS, true);


      const double tofOffset = 0;
      auto instrument = inWS->getInstrument();
      auto source = instrument->getSource();
      auto sample = instrument->getSample();
      const double L1 = source->getDistance(*sample);

      //Go through all the histograms and set the data
      PARALLEL_FOR2(inWS, outputWS)
      for (int i = 0; i < histnumber; ++i)
      {
        PARALLEL_START_INTERUPT_REGION

        const double L2 = inWS->getDetector(i)->getDistance(*sample);
        const double tofFactor = L1 / (L1 + L2);

        const IEventList* el = inWS->getEventListPtr(i);
        MantidVec y_data, e_data;
        // The EventList takes care of histogramming.
        el->generateHistogramTimeAtSample(*XValues_new, y_data, e_data, tofFactor, tofOffset);

        //Set the X axis for each output histogram
        outputWS->setX(i, OutXValues_scaled);

        //Copy the data over.
        outputWS->dataY(i).assign(y_data.begin(), y_data.end());
        outputWS->dataE(i).assign(e_data.begin(), e_data.end());

        //Report progress
        prog.report(name());
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION

    //Copy all the axes
    for (int i = 1; i < inWS->axes(); i++)
    {
      outputWS->replaceAxis(i, inWS->getAxis(i)->clone(outputWS.get()));
      outputWS->getAxis(i)->unit() = inWS->getAxis(i)->unit();
    }

    // X-unit is relative time since the start of the run.
    outputWS->getAxis(0)->unit() = boost::make_shared<Units::Time>();

    //Copy the units over too.
    for (int i = 1; i < outputWS->axes(); ++i)
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
