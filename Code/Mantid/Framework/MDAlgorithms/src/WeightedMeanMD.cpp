/*WIKI* 

Takes two MDHistoWorkspaces and calculates the weighted mean for each bin. See [[WeightedMean]] for more details on the algorithm workings. Both inputs must be MDHistoWorkspaces, the algorithm will not run with MDEventWorkspaces.

== Usage ==

The following utilises [[WeightedMean]] and [[WeightedMeanMD]] to inspect the same data.

 import math
 
 # Create some input arrays data
 pi = 3.14
 s1 = []
 e1 = []
 s2 = []
 e2 = []
 extents = [0,40]
 x = range(extents[0], extents[1])
 theta_shift=0.4
 for i in x :
 	theta = 0.02 * i * pi
 	s1.append(math.sin(theta))
 	e1.append(math.sin(theta))
 	s2.append(math.sin(theta+theta_shift))
 	e2.append(math.sin(theta+theta_shift))
 
 # Create Matrix workspaces from input arrrays
 matrix_1 =CreateWorkspace(DataX=x, DataE=e1, NSpec=1,DataY=s1)
 matrix_2 =CreateWorkspace(DataX=x, DataE=e2, NSpec=1,DataY=s2)
 # Create MD workspaces from input arrays
 md_1 =CreateMDHistoWorkspace(Dimensionality=1,SignalInput=s1,ErrorInput=e1,NumberOfBins=[len(x)],Extents=extents,Names="v",Units="t")
 md_2 =CreateMDHistoWorkspace(Dimensionality=1,SignalInput=s2,ErrorInput=e2,NumberOfBins=[len(x)],Extents=extents,Names="v",Units="t")
 
 # Produce the weighted mean as a matrix workspace.
 mean = WeightedMean(InputWorkspace1=matrix_1, InputWorkspace2=matrix_2)
 
 # Produce the weithed mean as a 1D MD workspace. Contents sould be identical to the output created above.
 mean_md = WeightedMeanMD(LHSWorkspace=md_1,RHSWorkspace=md_2)


*WIKI*/

#include "MantidMDAlgorithms/WeightedMeanMD.h"
#include "MantidMDEvents/MDHistoWorkspaceIterator.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace MDAlgorithms
{

  DECLARE_ALGORITHM(WeightedMeanMD)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  WeightedMeanMD::WeightedMeanMD()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  WeightedMeanMD::~WeightedMeanMD()
  {
  }
  
  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void WeightedMeanMD::initDocs()
  {
    this->setWikiSummary("Find weighted mean of two [[MDHistoWorkspace]]s.");
    this->setOptionalMessage("Find weighted mean of two MDHistoWorkspaces.");
  }

  //----------------------------------------------------------------------------------------------
  /// Is the operation commutative?
  bool WeightedMeanMD::commutative() const
  { return true; }

    //----------------------------------------------------------------------------------------------
  /// Check the inputs and throw if the algorithm cannot be run
  void WeightedMeanMD::checkInputs()
  {
    if (!m_lhs_histo || !m_rhs_histo)
      throw std::invalid_argument(this->name() + " can only be run on a MDHistoWorkspace.");
  }

  //----------------------------------------------------------------------------------------------
  /// Run the algorithm with a MDHisotWorkspace as output and operand
  void WeightedMeanMD::execHistoHisto(Mantid::MDEvents::MDHistoWorkspace_sptr out, Mantid::MDEvents::MDHistoWorkspace_const_sptr operand)
  {
    using MDEvents::MDHistoWorkspaceIterator;
    MDHistoWorkspaceIterator* lhs_it  = dynamic_cast<MDHistoWorkspaceIterator*>(out->createIterator());
    MDHistoWorkspaceIterator* rhs_it  = dynamic_cast<MDHistoWorkspaceIterator*>(operand->createIterator());

    do
    {
      double lhs_s = lhs_it->getSignal();
      double lhs_err = lhs_it->getError();
      double rhs_s = rhs_it->getSignal();
      double rhs_err = rhs_it->getError();
      double signal = 0;
      double error_sq = 0;
      if ((lhs_err > 0.0) && (rhs_err > 0.0))
      {
        double rhs_err_sq = rhs_err*rhs_err;
        double lhs_err_sq = lhs_err*lhs_err;
        double s = (rhs_s/rhs_err_sq) + (lhs_s/lhs_err_sq);
        double e = rhs_err_sq * lhs_err_sq / ( rhs_err_sq + lhs_err_sq);
        signal = s * e;
        error_sq = e;
      }
      else if((rhs_err > 0) && (lhs_err <= 0))
      {
        signal = rhs_s;
        error_sq = rhs_err*rhs_err;
      }
      else if((lhs_err <= 0) && (rhs_err > 0))
      {
        signal = lhs_s;
        error_sq = lhs_err*lhs_err;
      }

      size_t pos = lhs_it->getLinearIndex();
      out->setSignalAt(pos, signal);
      out->setErrorSquaredAt(pos, error_sq);
    }
    while(lhs_it->next() && rhs_it->next());
      
    delete lhs_it;
    delete rhs_it;
}

  //----------------------------------------------------------------------------------------------
  /// Run the algorithm with a MDHisotWorkspace as output, scalar and operand
  void WeightedMeanMD::execHistoScalar(Mantid::MDEvents::MDHistoWorkspace_sptr, Mantid::DataObjects::WorkspaceSingleValue_const_sptr)
  {
    throw std::runtime_error(this->name() + " can only be run with two MDHistoWorkspaces as inputs");
  }

  //----------------------------------------------------------------------------------------------
  /// Run the algorithm on a MDEventWorkspace
  void WeightedMeanMD::execEvent()
  {
    throw std::runtime_error(this->name() + " can only be run on a MDHistoWorkspace.");
  }



} // namespace Mantid
} // namespace MDAlgorithms