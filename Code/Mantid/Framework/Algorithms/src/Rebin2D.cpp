//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------    
#include "MantidAlgorithms/Rebin2D.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/NumericAxis.h"

namespace Mantid
{
  namespace Algorithms
  {

    // Register the algorithm into the AlgorithmFactory
    //DECLARE_ALGORITHM(Rebin2D)

    using API::MatrixWorkspace_const_sptr;
  
    //--------------------------------------------------------------------------
    // Public methods
    //--------------------------------------------------------------------------
    /**
     * Sets documentation strings for this algorithm
     */
    void Rebin2D::initDocs()
    {
      this->setWikiSummary("Rebins both axes of a 2D workspace.");
      this->setOptionalMessage("Rebins both axes of a 2D workspace using the given parameters");
      this->setWikiDescription("The input bin parameters are used to form an output grid. The overlap "
                               "of the polygons is tested to compute the required signal weight for the "
                               "new bin on the workspace");
    }

    /** 
     * Initialize the algorithm's properties.
     */
    void Rebin2D::init()
    {      
      using Kernel::ArrayProperty;
      using Kernel::Direction;
      using Kernel::RebinParamsValidator;
      using API::WorkspaceProperty;
      declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input), "An input workspace.");
      declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace.");
      const std::string docString = 
        "A comma separated list of first bin boundary, width, last bin boundary. Optionally\n"
        "this can be followed by a comma and more widths and last boundary pairs.\n"
        "Negative width values indicate logarithmic binning.";
      declareProperty(new ArrayProperty<double>("Axis1Binning", new RebinParamsValidator), docString);
      declareProperty(new ArrayProperty<double>("Axis2Binning", new RebinParamsValidator), docString);
    }

    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
     */
    void Rebin2D::exec()
    {
      MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

      // We require two grids. The input workspace has axis 1 as non-histogram data so we
      // need to make bins out of axis1.
      const MantidVec oldXBins = inputWS->readX(0);
      const std::vector<double> oldYBins = 
        dynamic_cast<API::NumericAxis*>(inputWS->getAxis(1))->createBinBoundaries();
      // Output bins
      MantidVecPtr newXBins;
      const int nXValues = 
        Kernel::VectorHelper::createAxisFromRebinParams(getProperty("Axis1Binning"), newXBins.access());
      MantidVecPtr newYBins;
      const int nYValues = 
        Kernel::VectorHelper::createAxisFromRebinParams(getProperty("Axis2Binning"), newYBins.access());
      assert(nXValues > 0);
      assert(nYValues > 0);
      

    }
    
  } // namespace Algorithms
} // namespace Mantid

