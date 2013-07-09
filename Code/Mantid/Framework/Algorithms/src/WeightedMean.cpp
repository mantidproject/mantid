/*WIKI* 

The algorithm calculates the weighted mean of two workspaces. This is useful when working with distributions rather than histograms, particularly when counting statistics are poor and it is possible that the value of one data set is statistically insignificant but differs greatly from the other. In such a case simply calculating the average of the two data sets would produce a spurious result. This algorithm will eventually be modified to take a list of workspaces as an input.

<math>\displaystyle y=\frac{\sum\frac{x_i}{\sigma^{2}_i}}{\sum\frac{1}{\sigma^{2}_i}} </math>



*WIKI*/
#include "MantidAlgorithms/WeightedMean.h"

namespace Mantid
{
  namespace Algorithms
  {

    // Algorithm must be declared
    DECLARE_ALGORITHM(WeightedMean)

    /// Sets documentation strings for this algorithm
    void WeightedMean::initDocs()
    {
      this->setWikiSummary("An algorithm to calculate the weighted mean of two workspaces. ");
      this->setOptionalMessage("An algorithm to calculate the weighted mean of two workspaces.");
    }


    bool WeightedMean::checkCompatibility(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs) const
    {
      if ( lhs->YUnit() != rhs->YUnit() )
      {
        g_log.error("The two workspaces are not compatible because they have different units for the data (Y).");
        return false;
      }
      if ( lhs->isDistribution() != rhs->isDistribution() )
      {
        g_log.error("The two workspaces are not compatible because one is flagged as a distribution.");
        return false;
      }

      return BinaryOperation::checkCompatibility(lhs,rhs);
    }

    /** Performs a simple check to see if the sizes of two workspaces are identically sized
     *  @param lhs :: the first workspace to compare
     *  @param rhs :: the second workspace to compare
     *  @retval "" The two workspaces are size compatible
     *  @retval "<reason why not compatible>" The two workspaces are NOT size compatible
     */
    std::string WeightedMean::checkSizeCompatibility(const API::MatrixWorkspace_const_sptr lhs,const API::MatrixWorkspace_const_sptr rhs) const
    {
      //in order to be size compatible then the workspaces must be identically sized
      if (lhs->size() == rhs->size())
      {
        return "";
      }
      else
      {
        return "Workspaces not identically sized";
      }
    }

    void WeightedMean::performBinaryOperation(const MantidVec& lhsX, const MantidVec& lhsY, const MantidVec& lhsE,
                                              const MantidVec& rhsY, const MantidVec& rhsE, MantidVec& YOut, MantidVec& EOut)
    {
      (void) lhsX; //Avoid compiler warning
      const size_t bins = lhsY.size();
      for (size_t j=0; j<bins; ++j)
      {
        if (lhsE[j] > 0.0 && rhsE[j] > 0.0)
        {
          const double err1 = lhsE[j]*lhsE[j];
          const double err2 = rhsE[j]*rhsE[j];
          YOut[j] = (lhsY[j]/err1)+(rhsY[j]/err2);
          EOut[j] = (err1*err2)/(err1+err2);
          YOut[j] *= EOut[j];
          EOut[j] = sqrt(EOut[j]);
        }
        else if (lhsE[j] > 0.0 && rhsE[j] <= 0.0)
        {
          YOut[j] = lhsY[j];
          EOut[j] = lhsE[j];
        }
        else if (lhsE[j] <= 0.0 && rhsE[j] > 0.0)
        {
          YOut[j] = rhsY[j];
          EOut[j] = rhsE[j];
        }
        else
        {
          YOut[j] = 0.0;
          EOut[j] = 0.0;
        }
      }
    }

    void WeightedMean::performBinaryOperation(const MantidVec& lhsX, const MantidVec& lhsY, const MantidVec& lhsE,
                                              const double rhsY, const double rhsE, MantidVec& YOut, MantidVec& EOut)
    {
	    UNUSED_ARG(lhsX);
      assert( lhsX.size() == 1 );
      // If we get here we've got two single column workspaces so it's easy.
      if (lhsE[0] > 0.0 && rhsE > 0.0)
      {
        const double err1 = lhsE[0]*lhsE[0];
        const double err2 = rhsE*rhsE;
        YOut[0] = (lhsY[0]/err1)+(rhsY/err2);
        EOut[0] = (err1*err2)/(err1+err2);
        YOut[0] *= EOut[0];
        EOut[0] = sqrt(EOut[0]); 
      }
      else if (lhsE[0] > 0.0 && rhsE <= 0.0)
      {
        YOut[0] = lhsY[0];
        EOut[0] = lhsE[0];
      }
      else if (lhsE[0] <= 0.0 && rhsE > 0.0)
      {
        YOut[0] = rhsY;
        EOut[0] = rhsE;
      }
      else
      {
        YOut[0] = 0.0;
        EOut[0] = 0.0;
      }
    }

  } // namespace Algorithms
} // namespace Mantid
