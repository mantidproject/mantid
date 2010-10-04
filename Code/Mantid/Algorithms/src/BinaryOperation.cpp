//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/BinaryOperation.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidGeometry/IDetector.h"

using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

namespace Mantid
{
  namespace Algorithms
  {
    BinaryOperation::BinaryOperation() : API::PairedGroupAlgorithm(), m_indicesToMask(), m_progress(NULL) {}
    
    BinaryOperation::~BinaryOperation()
    {
      if (m_progress) delete m_progress;
    }
    
    /** Initialisation method.
     *  Defines input and output workspaces
     *
     */
    void BinaryOperation::init()
    {
      declareProperty(new WorkspaceProperty<MatrixWorkspace>(inputPropName1(),"",Direction::Input));
      declareProperty(new WorkspaceProperty<MatrixWorkspace>(inputPropName2(),"",Direction::Input));
      declareProperty(new WorkspaceProperty<MatrixWorkspace>(outputPropName(),"",Direction::Output));
    }

    //--------------------------------------------------------------------------------------------
    /** Executes the algorithm
     *
     *  @throw runtime_error Thrown if algorithm cannot execute
     */
    void BinaryOperation::exec()
    {
      // get input workspace, dynamic cast not needed
      MatrixWorkspace_const_sptr lhs = getProperty(inputPropName1());
      MatrixWorkspace_const_sptr rhs = getProperty(inputPropName2());

      //Should we use event-type processing?
      if (checkEventCompatibility(lhs, rhs))
      {
        //Have the subclass handle the event stuff
        this->execEvent( boost::dynamic_pointer_cast<const EventWorkspace>(lhs),
            boost::dynamic_pointer_cast<const EventWorkspace>(rhs));

        //And we are done
        return;
      }

      // Check that the input workspace are compatible
      if (!checkCompatibility(lhs,rhs))
      {
        std::ostringstream ostr;
        ostr << "The two workspaces are not compatible for algorithm " << this->name();
        g_log.error() << ostr.str() << std::endl;
        throw std::invalid_argument( ostr.str() );
      }

      // Make sure the left hand workspace is the larger (this is fine if we got past the compatibility checks)
      if ( rhs->size() > lhs->size() )
      {
        MatrixWorkspace_const_sptr smaller = lhs;
        lhs = rhs;
        rhs = smaller;
      }

      MatrixWorkspace_sptr out_work = getProperty(outputPropName());
      // We need to create a new workspace for the output if:
      //   (a) the output workspace hasn't been set to one of the input ones, or
      //   (b) it has been, but it's not the correct dimensions
      if ( (out_work != lhs && out_work != rhs) || ( out_work == rhs && ( lhs->size() > rhs->size() ) ) )
      {
        out_work = WorkspaceFactory::Instance().create(lhs);
      }

      // only overridden for some operations (plus and minus at the time of writing)
      operateOnRun(lhs->run(), rhs->run(), out_work->mutableRun());

      // Initialise the progress reporting object
      m_progress = new Progress(this,0.0,1.0,lhs->getNumberHistograms());

      // There are now 4 possible scenarios, shown schematically here:
      // xxx x   xxx xxx   xxx xxx   xxx x
      // xxx   , xxx xxx , xxx     , xxx x
      // xxx   , xxx xxx   xxx       xxx x
      // So work out which one we have and call the appropriate function
      if ( rhs->size() == 1 ) // Single value workspace
      {
        doSingleValue(lhs,rhs,out_work);
      }
      else if ( rhs->getNumberHistograms() == 1 ) // Single spectrum on rhs
      {
        doSingleSpectrum(lhs,rhs,out_work);
      }
      else if ( rhs->blocksize() == 1 ) // Single column on rhs
      {
	m_indicesToMask.reserve(out_work->getNumberHistograms());
        doSingleColumn(lhs,rhs,out_work);
      }
      else // The two are both 2D and should be the same size
      {
	m_indicesToMask.reserve(out_work->getNumberHistograms());
        do2D(lhs,rhs,out_work);
      }

      applyMaskingToOutput(out_work);
      setOutputUnits(lhs,rhs,out_work);

      // Assign the result to the output workspace property
      setProperty(outputPropName(),out_work);

      return;
    }


    //--------------------------------------------------------------------------------------------
    /** Execute a binary operation on events. Should be overridden. */
    void BinaryOperation::execEvent( DataObjects::EventWorkspace_const_sptr lhs, DataObjects::EventWorkspace_const_sptr rhs )
    {
      (void) lhs;(void) rhs; //Avoid compiler warnings
      //This should never happen
      throw Exception::NotImplementedError("BinaryOperation::execEvent() is not implemented for this operation.");
    }

    //--------------------------------------------------------------------------------------------
    /** Return true if the two workspaces are compatible for this operation
     * Virtual: will be overridden as needed. */
    bool BinaryOperation::checkCompatibility(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs) const
    {
      Unit_const_sptr lhs_unit = Unit_sptr();
      Unit_const_sptr rhs_unit = Unit_sptr();
      if ( lhs->axes() && rhs->axes() ) // If one of these is a WorkspaceSingleValue then we don't want to check units match
      {
        lhs_unit = lhs->getAxis(0)->unit();
        rhs_unit = rhs->getAxis(0)->unit();
      }

      const std::string lhs_unitID = ( lhs_unit ? lhs_unit->unitID() : "" );
      const std::string rhs_unitID = ( rhs_unit ? rhs_unit->unitID() : "" );

      // Check the workspaces have the same units and distribution flag
      if ( lhs_unitID != rhs_unitID && lhs->blocksize() > 1 && rhs->blocksize() > 1 )
      {
        g_log.error("The two workspace are not compatible because they have different units on the X axis.");
        return false;
      }

      // Check the size compatibility
      if (!checkSizeCompatibility(lhs,rhs))
      {
        std::ostringstream ostr;
        ostr<<"The sizes of the two workspaces are not compatible for algorithm "<<this->name();
        g_log.error() << ostr.str() << std::endl;
        throw std::invalid_argument( ostr.str() );
      }

      return true;
    }

    //--------------------------------------------------------------------------------------------
    /** Return true if the two workspaces can be treated as event workspaces
     * for the binary operation (e.g. Plus algorithm will concatenate event lists)
     *
     * @return false by default; will be overridden by specific algorithms
     */
    bool BinaryOperation::checkEventCompatibility(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs)
    {
      (void) lhs; (void) rhs; //Avoid compiler warning
      return false;
    }


    //--------------------------------------------------------------------------------------------
    /** Performs a simple check to see if the sizes of two workspaces are compatible for a binary operation
     *  In order to be size compatible then the larger workspace
     *  must divide be the size of the smaller workspace leaving no remainder
     *  @param lhs the first workspace to compare
     *  @param rhs the second workspace to compare
     *  @retval true The two workspaces are size compatible
     *  @retval false The two workspaces are NOT size compatible
     */
    bool BinaryOperation::checkSizeCompatibility(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs) const
    {
      const int lhsSize = lhs->size();
      const int rhsSize = rhs->size();
      // A SingleValueWorkspace matches anything
      if ( rhsSize == 1 ) return true;
      // The rhs must not be smaller than the lhs
      if ( lhsSize < rhsSize ) return false;
      // 
      // Otherwise they must match both ways, or horizontally or vertically with the other rhs dimension=1
      if ( rhs->blocksize() == 1 && lhs->getNumberHistograms() == rhs->getNumberHistograms() ) return true;
      // Past this point, we require the X arrays to match. Note this only checks the first spectrum
      if ( !WorkspaceHelpers::matchingBins(lhs,rhs,true) ) return false;
      
      const int rhsSpec = rhs->getNumberHistograms();
      return ( lhs->blocksize() == rhs->blocksize() && ( rhsSpec==1 || lhs->getNumberHistograms() == rhsSpec ) );
    }

    //--------------------------------------------------------------------------------------------
    /**
     * Checks if the spectra at the given index of either input workspace is masked. If so then the output spectra has zeroed data
     * and is also masked. 
     * @param lhs A pointer to the left-hand operand
     * @param rhs A pointer to the right-hand operand
     * @param index The workspace index to check
     * @param out A pointer to the output workspace
     * @returns True if further processing is not required on the spectra, false if the binary operation should be performed.
     */
    bool BinaryOperation::propagateSpectraMask(const API::MatrixWorkspace_const_sptr lhs, const API::MatrixWorkspace_const_sptr rhs, 
					       const int index, API::MatrixWorkspace_sptr out)
    {
      bool continueOp(true);
      IDetector_sptr det_lhs, det_rhs;
      try
      {
	det_lhs = lhs->getDetector(index);
	det_rhs = rhs->getDetector(index);
      }
      catch(std::runtime_error &)
      {
      }
      if( (det_lhs && det_lhs->isMasked()) || ( det_rhs && det_rhs->isMasked()) )
      {
 	continueOp = false;
	//Zero the output data and ensure that the output spectra is masked. The masking is done outside of this
	//loop modiying the parameter map in a multithreaded loop requires too much locking
	m_indicesToMask.push_back(index);
   	MantidVec & yValues = out->dataY(index);
   	MantidVec & eValues = out->dataE(index);
   	MantidVec::const_iterator yend = yValues.end();
	for( MantidVec::iterator yit(yValues.begin()), eit(eValues.begin()); yit != yend; ++yit, ++eit)
 	{
   	  (*yit) = 0.0;
   	  (*eit) = 0.0;
 	}
      }
      return continueOp;
    }

    //--------------------------------------------------------------------------------------------
    /** Called when the rhs operand is a single value. 
     *  Loops over the lhs workspace calling the abstract binary operation function. 
     *  @param lhs The workspace which is the left hand operand
     *  @param rhs The workspace which is the right hand operand
     *  @param out The result workspace
     */
    void BinaryOperation::doSingleValue(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs,API::MatrixWorkspace_sptr out)
    {
      // Don't propate masking from the rhs here - it would be decidedly odd if the single value was masked

      // Pull out the single value and its error
      const double rhsY = rhs->readY(0)[0];
      const double rhsE = rhs->readE(0)[0];
      
      // Now loop over the spectra of the left hand side calling the virtual function
      const int numHists = lhs->getNumberHistograms();
      PARALLEL_FOR3(lhs,rhs,out)
      for (int i = 0; i < numHists; ++i)
      {
        PARALLEL_START_INTERUPT_REGION
        out->setX(i,lhs->refX(i));
        performBinaryOperation(lhs->readX(i),lhs->readY(i),lhs->readE(i),rhsY,rhsE,out->dataY(i),out->dataE(i));
        m_progress->report();
        PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION
   }
   

    //--------------------------------------------------------------------------------------------
    /** Called when the rhs operand is a single spectrum. 
     *  Loops over the lhs workspace calling the abstract binary operation function. 
     *  @param lhs The workspace which is the left hand operand
     *  @param rhs The workspace which is the right hand operand
     *  @param out The result workspace
     */
    void BinaryOperation::doSingleSpectrum(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs,API::MatrixWorkspace_sptr out)
    {
      // Propagate any masking first or it could mess up the numbers
      propagateBinMasks(rhs,out);

      // Pull out the rhs spectrum
      const MantidVec& rhsY = rhs->readY(0);
      const MantidVec& rhsE = rhs->readE(0);

      // Now loop over the spectra of the left hand side calling the virtual function
      const int numHists = lhs->getNumberHistograms();
      PARALLEL_FOR3(lhs,rhs,out)
      for (int i = 0; i < numHists; ++i)
      {
        PARALLEL_START_INTERUPT_REGION
        out->setX(i,lhs->refX(i));
        performBinaryOperation(lhs->readX(i),lhs->readY(i),lhs->readE(i),rhsY,rhsE,out->dataY(i),out->dataE(i));
        m_progress->report();
        PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION
    }
    
    /** Called when the rhs operand is a 2D workspace of single values. 
     *  Loops over the workspaces calling the abstract binary operation function. 
     *  @param lhs The workspace which is the left hand operand
     *  @param rhs The workspace which is the right hand operand
     *  @param out The result workspace
     */
    void BinaryOperation::doSingleColumn(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs,API::MatrixWorkspace_sptr out)
    {
      // Don't propate masking from the rhs here - it would be decidedly odd if the single bin was masked

      // Now loop over the spectra of the left hand side pulling out the single value from each rhs 'spectrum'
      // and then calling the virtual function
      const int numHists = lhs->getNumberHistograms();
      PARALLEL_FOR3(lhs,rhs,out)
      for (int i = 0; i < numHists; ++i)
      {
        PARALLEL_START_INTERUPT_REGION
        const double rhsY = rhs->readY(i)[0];
        const double rhsE = rhs->readE(i)[0];        
        
        out->setX(i,lhs->refX(i));
	if( propagateSpectraMask(lhs, rhs, i, out) )
	{
	  performBinaryOperation(lhs->readX(i),lhs->readY(i),lhs->readE(i),rhsY,rhsE,out->dataY(i),out->dataE(i));
	}
        m_progress->report();
        PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION
    }
    
    /** Called when the two workspaces are the same size. 
     *  Loops over the workspaces extracting the appropriate spectra and calling the abstract binary operation function. 
     *  @param lhs The workspace which is the left hand operand
     *  @param rhs The workspace which is the right hand operand
     *  @param out The result workspace
     */
    void BinaryOperation::do2D(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs,API::MatrixWorkspace_sptr out)
    {
      // Propagate any masking first or it could mess up the numbers
      propagateBinMasks(rhs,out);
      // Loop over the spectra calling the virtual function for each one
      const int numHists = lhs->getNumberHistograms();
      PARALLEL_FOR3(lhs,rhs,out)
      for (int i = 0; i < numHists; ++i)
      {
        PARALLEL_START_INTERUPT_REGION
        out->setX(i,lhs->refX(i));
	if( propagateSpectraMask(lhs, rhs, i, out) )
	{
	  performBinaryOperation(lhs->readX(i),lhs->readY(i),lhs->readE(i),rhs->readY(i),rhs->readE(i),out->dataY(i),out->dataE(i));
	}
        m_progress->report();
        PARALLEL_END_INTERUPT_REGION
      }      
      PARALLEL_CHECK_INTERUPT_REGION
    }

    /** Copies any bin masking from the smaller/rhs input workspace to the output.
     *  Masks on the other input workspace are copied automatically by the workspace factory.
     *  @param rhs The workspace which is the right hand operand
     *  @param out The result workspace
     */
    void BinaryOperation::propagateBinMasks(const API::MatrixWorkspace_const_sptr rhs, API::MatrixWorkspace_sptr out)
    {
      const int outHists = out->getNumberHistograms();
      const int rhsHists = rhs->getNumberHistograms();
      for (int i = 0; i < outHists; ++i)
      {
        // Copy over masks from the rhs, if any exist.
        // If rhs is single spectrum, copy masks from that to all spectra in the output.
        if ( rhs->hasMaskedBins((rhsHists==1) ? 0 : i) )
        {
          const MatrixWorkspace::MaskList & masks = rhs->maskedBins( (rhsHists==1) ? 0 : i );
          MatrixWorkspace::MaskList::const_iterator it;
          for (it = masks.begin(); it != masks.end(); ++it)
	  {
            out->maskBin(i,it->first,it->second);
	  }
        }
      }
    }

  /**
   * Apply the requested masking to the output workspace
   * @param out The workspace to mask
   */
    void BinaryOperation::applyMaskingToOutput(API::MatrixWorkspace_sptr out)
  {
    int nindices = static_cast<int>(m_indicesToMask.size());
    ParameterMap &pmap = out->instrumentParameters();
    PARALLEL_FOR1(out)
    for(int i = 0; i < nindices; ++i)
    {
      PARALLEL_START_INTERUPT_REGION

      try
      {
	IDetector_sptr det_out = out->getDetector(m_indicesToMask[i]);
	PARALLEL_CRITICAL(BinaryOperation_masking)
	{
	  pmap.addBool(det_out.get(), "masked", true);
	}
      }
      catch(std::runtime_error &)
      {
      }

      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION
  }

  } // namespace Algorithms
} // namespace Mantid
