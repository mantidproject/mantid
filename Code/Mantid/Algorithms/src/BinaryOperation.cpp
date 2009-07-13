//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/BinaryOperation.h"
#include "MantidAPI/WorkspaceProperty.h"


using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid
{
  namespace Algorithms
  {
    BinaryOperation::BinaryOperation() : API::Algorithm(), m_progress(NULL) {}
    
    BinaryOperation::~BinaryOperation()
    {
      if (m_progress) delete m_progress;
    }
    
    /** Initialisation method.
    * Defines input and output workspaces
    *
    */
    void BinaryOperation::init()
    {
      declareProperty(new WorkspaceProperty<MatrixWorkspace>(inputPropName1(),"",Direction::Input));
      declareProperty(new WorkspaceProperty<MatrixWorkspace>(inputPropName2(),"",Direction::Input));
      declareProperty(new WorkspaceProperty<MatrixWorkspace>(outputPropName(),"",Direction::Output));
    }

    /** Executes the algorithm
    *
    *  @throw runtime_error Thrown if algorithm cannot execute
    */
    void BinaryOperation::exec()
    {
      // get input workspace, dynamic cast not needed
      MatrixWorkspace_const_sptr lhs = getProperty(inputPropName1());
      MatrixWorkspace_const_sptr rhs = getProperty(inputPropName2());

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
        doSingleColumn(lhs,rhs,out_work);
      }
      else // The two are both 2D and should be the same size
      {
        do2D(lhs,rhs,out_work);
      }
      
      // Assign the result to the output workspace property
      setProperty(outputPropName(),out_work);

      return;
    }

    const bool BinaryOperation::checkCompatibility(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs) const
    {
      Unit_sptr lhs_unit = Unit_sptr();
      Unit_sptr rhs_unit = Unit_sptr();
      if ( lhs->axes() && rhs->axes() ) // If one of these is a WorkspaceSingleValue then we don't want to check units match
      {
        lhs_unit = lhs->getAxis(0)->unit();
        rhs_unit = rhs->getAxis(0)->unit();
      }

      // Check the workspaces have the same units and distribution flag
      if ( lhs_unit != rhs_unit && lhs->blocksize() > 1 && rhs->blocksize() > 1 )
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

    /** Performs a simple check to see if the sizes of two workspaces are compatible for a binary operation
    * In order to be size compatible then the larger workspace
    * must divide be the size of the smaller workspace leaving no remainder
    * @param lhs the first workspace to compare
    * @param rhs the second workspace to compare
    * @retval true The two workspaces are size compatible
    * @retval false The two workspaces are NOT size compatible
    */
    const bool BinaryOperation::checkSizeCompatibility(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs) const
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

    /** Performs a simple check to see if the X arrays of two workspaces are compatible for a binary operation
    * The X arrays of two workspaces must be identical to allow a binary operation to be performed
    * @param lhs the first workspace to compare
    * @param rhs the second workspace to compare
    * @retval true The two workspaces are size compatible
    * @retval false The two workspaces are NOT size compatible
    */
    const bool BinaryOperation::checkXarrayCompatibility(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs) const
    {
      // Not using the WorkspaceHelpers::matching bins method because that requires the workspaces to be
      // the same size, which isn't a requirement of BinaryOperation

      // single values, or workspaces with just a single bin/value in each spectrum, are compatible with anything
      if ((rhs->blocksize() ==1) || (lhs->blocksize() ==1)) return true;

      const std::vector<double>& w1x = lhs->readX(0);
      const std::vector<double>& w2x = rhs->readX(0);

      double sum;
      sum=0.0;
      for (unsigned int i=0; i < w1x.size(); i++) sum += fabs(w1x[i]-w2x[i]);
      if( sum < 0.0000001)
        return true;
      else
        return false;
    }

    /** Called when the rhs operand is a single value. 
     *  Loops over the lhs workspace calling the abstract binary operation function. 
     *  @param lhs The workspace which is the left hand operand
     *  @param rhs The workspace which is the right hand operand
     *  @param out The result workspace
     */
    void BinaryOperation::doSingleValue(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs,API::MatrixWorkspace_sptr out)
    {
      // Pull out the single value and its error
      const double rhsY = rhs->readY(0)[0];
      const double rhsE = rhs->readE(0)[0];
      
      // Now loop over the spectra of the left hand side calling the virtual function
      const int numHists = lhs->getNumberHistograms();
				PARALLEL_FOR3(lhs,rhs,out)
      for (int i = 0; i < numHists; ++i)
      {
        out->dataX(i) = lhs->readX(i);
        performBinaryOperation(lhs->readX(i),lhs->readY(i),lhs->readE(i),rhsY,rhsE,out->dataY(i),out->dataE(i));
        m_progress->report();
      }
    }
   
    /** Called when the rhs operand is a single spectrum. 
     *  Loops over the lhs workspace calling the abstract binary operation function. 
     *  @param lhs The workspace which is the left hand operand
     *  @param rhs The workspace which is the right hand operand
     *  @param out The result workspace
     */
    void BinaryOperation::doSingleSpectrum(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs,API::MatrixWorkspace_sptr out)
    {
      // Pull out the rhs spectrum
      MantidVec rhsY = rhs->readY(0);
      MantidVec rhsE = rhs->readE(0);

      // Now loop over the spectra of the left hand side calling the virtual function
      const int numHists = lhs->getNumberHistograms();
				PARALLEL_FOR3(lhs,rhs,out)
      for (int i = 0; i < numHists; ++i)
      {
        out->dataX(i) = lhs->readX(i);
        performBinaryOperation(lhs->readX(i),lhs->readY(i),lhs->readE(i),rhsY,rhsE,out->dataY(i),out->dataE(i));
        m_progress->report();
      }
    }
    
    /** Called when the rhs operand is a 2D workspace of single values. 
     *  Loops over the workspaces calling the abstract binary operation function. 
     *  @param lhs The workspace which is the left hand operand
     *  @param rhs The workspace which is the right hand operand
     *  @param out The result workspace
     */
    void BinaryOperation::doSingleColumn(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs,API::MatrixWorkspace_sptr out)
    {
      // Now loop over the spectra of the left hand side pulling out the single value from each rhs 'spectrum'
      // and then calling the virtual function
      const int numHists = lhs->getNumberHistograms();
			PARALLEL_FOR3(lhs,rhs,out)
      for (int i = 0; i < numHists; ++i)
      {
        const double rhsY = rhs->readY(i)[0];
        const double rhsE = rhs->readE(i)[0];        
        
        out->dataX(i) = lhs->readX(i);
        performBinaryOperation(lhs->readX(i),lhs->readY(i),lhs->readE(i),rhsY,rhsE,out->dataY(i),out->dataE(i));
        m_progress->report();
      }
    }
    
    /** Called when the two workspaces are the same size. 
     *  Loops over the workspaces extracting the appropriate spectra and calling the abstract binary operation function. 
     *  @param lhs The workspace which is the left hand operand
     *  @param rhs The workspace which is the right hand operand
     *  @param out The result workspace
     */
    void BinaryOperation::do2D(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs,API::MatrixWorkspace_sptr out)
    {
      // Loop over the spectra calling the virtual function for each one
      const int numHists = lhs->getNumberHistograms();
	 			PARALLEL_FOR3(lhs,rhs,out)
      for (int i = 0; i < numHists; ++i)
      {
        out->dataX(i) = lhs->readX(i);
        performBinaryOperation(lhs->readX(i),lhs->readY(i),lhs->readE(i),rhs->readY(i),rhs->readE(i),out->dataY(i),out->dataE(i));
        m_progress->report();
		//progress(double(i/numHists));
      }      
    }
    
  }
}
