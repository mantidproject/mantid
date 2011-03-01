//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ChangeBinOffset.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid
{
  namespace Algorithms
  {

   using namespace Kernel;
   using namespace API;
   using namespace DataObjects;

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(ChangeBinOffset)

    /**
     * Default constructor
     */
    ChangeBinOffset::ChangeBinOffset() : API::Algorithm(), m_progress(NULL) {}

    /**
     * Destructor
     */
    ChangeBinOffset::~ChangeBinOffset()
    {
      if( m_progress )
      {
	delete m_progress;
      }
    }

    /** Initialisation method. Declares properties to be used in algorithm.
    *
    */
    void ChangeBinOffset::init()
    {
      //this->setWikiSummary("Adjusts all the time bin values in a workspace by a specified amount.");
      //this->setOptionalMessage("Adjusts all the time bin values in a workspace by a specified amount.");

      declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input),
        "Name of the input workspace");
      declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output),
        "Name of the output workspace");
      BoundedValidator<double> *isDouble = new BoundedValidator<double>();
      declareProperty("Offset", 0.0, isDouble,
        "The amount to change each time bin by");

      BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
      mustBePositive->setLower(0);
      declareProperty("IndexMin", 0, mustBePositive,
        "The workspace index of the first spectrum to shift. Only used if\n"
        "IndexMax is set.");
      declareProperty("IndexMax", Mantid::EMPTY_INT(), mustBePositive->clone(),
        "The workspace index of the last spectrum to shift. Only used if\n"
        "explicitly set.");

    }

    /** Executes the algorithm
     *
     */
    void ChangeBinOffset::exec()
    {
	    //Get input workspace and offset
	    const MatrixWorkspace_sptr inputW = getProperty("InputWorkspace");
	    //Check if its an event workspace
	    EventWorkspace_const_sptr eventWS = boost::dynamic_pointer_cast<const EventWorkspace>(inputW);
	    if (eventWS != NULL)
	    {
	      this->execEvent();
	      return;
	    }

	    double offset = getProperty("Offset");
	    
	    API::MatrixWorkspace_sptr outputW = createOutputWS(inputW);	    
	    
	    //Get number of histograms
	    int histnumber = inputW->getNumberHistograms();
	    
	    m_progress = new API::Progress(this, 0.0, 1.0, histnumber);
	    
      int wi_min = 0;
      int wi_max=histnumber-1;

      //check if workspace indexes have been set
      int tempwi_min = getProperty("IndexMin");
      int tempwi_max = getProperty("IndexMax");
      if ( tempwi_max != Mantid::EMPTY_INT() ) 
      {
        //check wimin<=tempwi_min<=tempwi_max<=wi_max
        if ((wi_min <= tempwi_min) && (tempwi_min <= tempwi_max) && (tempwi_max <= wi_max))
        {
          wi_min = tempwi_min;
          wi_max = tempwi_max;
        }
        else
        {
          g_log.error("Invalid Workspace Index min/max properties");
          throw std::invalid_argument("Inconsistent properties defined");
        }
      }


      // do the shift in X
	    PARALLEL_FOR2(inputW, outputW)
	    for (int i=0; i < histnumber; ++i)
	    {		    
				PARALLEL_START_INTERUPT_REGION
		    //Do the offsetting
		    for (size_t j=0; j <  inputW->readX(i).size(); ++j)
		    {
			    //Change bin value by offset
			    if ((i >= wi_min) && (i <= wi_max)) outputW->dataX(i)[j] = inputW->readX(i)[j] + offset;
          else outputW->dataX(i)[j] = inputW->readX(i)[j];
		    }
		    //Copy y and e data
		    outputW->dataY(i) = inputW->dataY(i);
		    outputW->dataE(i) = inputW->dataE(i);
		    m_progress->report();
				PARALLEL_END_INTERUPT_REGION
	    }
			PARALLEL_CHECK_INTERUPT_REGION
	    
	    // Copy units
      if (outputW->getAxis(0)->unit().get())
          outputW->getAxis(0)->unit() = inputW->getAxis(0)->unit();
      try
      {
          if (inputW->getAxis(1)->unit().get())
              outputW->getAxis(1)->unit() = inputW->getAxis(1)->unit();
      }
      catch(Exception::IndexError &) {
          // OK, so this isn't a Workspace2D
      }

	    // Assign it to the output workspace property
	    setProperty("OutputWorkspace",outputW);
    }
    
    API::MatrixWorkspace_sptr ChangeBinOffset::createOutputWS(API::MatrixWorkspace_sptr input)
   {
	   //Check whether input = output to see whether a new workspace is required.
	    if (getPropertyValue("InputWorkspace") == getPropertyValue("OutputWorkspace"))
	    {
		    //Overwrite the original
		    return input;
	    }
	    else
	    {	    
		//Create new workspace for output from old
		API::MatrixWorkspace_sptr output = API::WorkspaceFactory::Instance().create(input);
		output->isDistribution(input->isDistribution());
		return output;
	    }
    }	
    
    void ChangeBinOffset::execEvent()
    {
      g_log.information("Processing event workspace");

      const MatrixWorkspace_const_sptr matrixInputWS = this->getProperty("InputWorkspace");
      EventWorkspace_const_sptr inputWS
                     = boost::dynamic_pointer_cast<const EventWorkspace>(matrixInputWS);

      // generate the output workspace pointer
      API::MatrixWorkspace_sptr matrixOutputWS = this->getProperty("OutputWorkspace");
      EventWorkspace_sptr outputWS;
      if (matrixOutputWS == matrixInputWS)
        outputWS = boost::dynamic_pointer_cast<EventWorkspace>(matrixOutputWS);
      else
      {
        //Make a brand new EventWorkspace
        outputWS = boost::dynamic_pointer_cast<EventWorkspace>(
            API::WorkspaceFactory::Instance().create("EventWorkspace", inputWS->getNumberHistograms(), 2, 1));
        //Copy geometry over.
        API::WorkspaceFactory::Instance().initializeFromParent(inputWS, outputWS, false);
        //You need to copy over the data as well.
        outputWS->copyDataFrom( (*inputWS) );

        //Cast to the matrixOutputWS and save it
        matrixOutputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(outputWS);
        this->setProperty("OutputWorkspace", matrixOutputWS);
      }

      double offset = getProperty("Offset");
      const int numberOfSpectra = inputWS->getNumberHistograms();

      m_progress = new API::Progress(this, 0.0, 1.0, numberOfSpectra);
      int wi_min = 0;
      int wi_max = numberOfSpectra-1;
      //check if workspace indexes have been set
      int tempwi_min = getProperty("IndexMin");
      int tempwi_max = getProperty("IndexMax");
      if ( tempwi_max != Mantid::EMPTY_INT() ) 
      {
        //check wimin<=tempwi_min<=tempwi_max<=wi_max
        if ((wi_min <= tempwi_min) && (tempwi_min <= tempwi_max) && (tempwi_max <= wi_max))
        {
          wi_min = tempwi_min;
          wi_max = tempwi_max;
        }
        else
        {
          g_log.error("Invalid Workspace Index min/max properties");
          throw std::invalid_argument("Inconsistent properties defined");
        }
      }

      PARALLEL_FOR1(outputWS)
      for (int i=0; i < numberOfSpectra; ++i)
      {
        PARALLEL_START_INTERUPT_REGION
        //Do the offsetting
		    if ((i >= wi_min) && (i <= wi_max)) outputWS->getEventList(i).addTof(offset);
        m_progress->report();
        PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION

      outputWS->clearMRU();
    }

  } // namespace Algorithm
} // namespace Mantid




