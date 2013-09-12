/*WIKI*
Performs 1D stitching of Reflectometry 2D MDHistoWorkspaces. Based on the Quick script developed at ISIS. This only works on 1D Histogrammed MD Workspaces.

Scales either the LHS or RHS workspace by some scale factor which, can be manually specified, or calculated from the overlap.

Calculates the weighted mean values in the overlap region and then combines the overlap region with the difference of the LHS and RHS workspaces
*WIKI*/

#include "MantidMDAlgorithms/Stitch1DMD.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include <boost/assign.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::MDEvents;

namespace Mantid
{
namespace MDAlgorithms
{

  /**
  Non-member helper function. Extracts the first instance of a non-integrated dimension. Throws if one cannot be found.
  @param ws: Input workspace to query.
  @return shared pointer to the non-integrated dimension.
  @throw runtime_error if there is not a single non-integrated dimension available.
  */
  Geometry::IMDDimension_const_sptr getFirstNonIntegratedDimension(IMDHistoWorkspace_const_sptr ws)
  {
    auto nonIntegratedDimensions =  ws->getNonIntegratedDimensions();
    if(nonIntegratedDimensions.size() == 0)
    {
      throw std::runtime_error("Workspace has no non-integrated dimensions.");
    }
    return nonIntegratedDimensions.front();
  }

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(Stitch1DMD)
  
  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  Stitch1DMD::Stitch1DMD()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  Stitch1DMD::~Stitch1DMD()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string Stitch1DMD::name() const { return "Stitch1DMD";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int Stitch1DMD::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string Stitch1DMD::category() const { return "Reflectometry\\ISIS";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void Stitch1DMD::initDocs()
  {
    this->setWikiSummary("Stitch two MD ReflectometryQ group workspaces together");
    this->setOptionalMessage("Sticch two MD ReflectometryQ group workspaces together.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void Stitch1DMD::init()
  {
    declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("RHSWorkspace", "", Direction::Input), "Input MD Histo Workspace");
    declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("LHSWorkspace", "", Direction::Input), "Input MD Histo Workspace");
    declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("OutputWorkspace", "", Direction::Output), "Input MD Histo Workspace");
    auto overlap_validator = boost::make_shared<CompositeValidator>();
    overlap_validator->add(boost::make_shared<BoundedValidator<double> >(0.0, 1.0));
    overlap_validator->add(boost::make_shared<MandatoryValidator<double> >());    

    declareProperty("StartOverlap", 0.0, overlap_validator->clone(), "Fraction along axis to start overlap. 0 to 1.");
    declareProperty("EndOverlap", 0.1, overlap_validator->clone(), "Fraction along axis to end overlap. 0 to 1.");
    declareProperty("ScaleRHSWorkspace", true, "Scaling either with respect to RHS or LHS Workspace.");
    declareProperty("UseManualScaleFactor", false, "True to use a provided value for the scale factor.");
    declareProperty("ManualScaleFactor", 1.0, "Provided value for the scale factor.");
    setPropertySettings("ManualScaleFactor", new EnabledWhenProperty("UseManualScaleFactor", IS_NOT_DEFAULT) );
    declareProperty("OutScaleFactor", -2.0, "The actual used value for the scaling factor.", Direction::Output); 
  }

  std::string Stitch1DMD::fetchInputPropertyName() const
  {
    return "RHSWorkspace";
  }

  /**
  Check/Validate that a given workspace is suitable as an algorithm input.
  @param ws : The input workspace to check.
  @throws if the workspace is not suitable.
  */
  void Stitch1DMD::checkIndividualWorkspace(IMDHistoWorkspace_const_sptr ws) const
  {
    size_t ndims = ws->getNumDims();
    if ((ndims < 1) || (ndims > 2))
    {
      throw  std::runtime_error( ws->name() + " must have 1 or 2 dimensions" );
    }
    if(ndims == 1)
    {
      auto dim1 = ws->getDimension(0);
      if (dim1->getNBins() == 1)
      {
        throw std::runtime_error(ws->name() + " is one-dimensional, so must have an un-integrated dimension.");
      }
    }
    if(ndims == 2)
    {
      auto dim1 = ws->getDimension(0);
      auto dim2 = ws->getDimension(1);
      if(dim1->getIsIntegrated() ^ dim2->getIsIntegrated())
      {
        std::runtime_error(ws->name() + " is two-dimensional, so must have one integrated and one un-integrated dimension.");
      }
    }
  }

  /**
  Validate that two input workspaces are suitable. 
  @param lhsWorkspace : workspace 1 input.
  @param rhsWorkspace : workspace 2 input.
  @throws if there are any inconsistencies between the two input workspaces.
  */
  void Stitch1DMD::checkBothWorkspaces(IMDHistoWorkspace_const_sptr lhsWorkspace, IMDHistoWorkspace_const_sptr rhsWorkspace) const
  {
    size_t ndims = std::min(lhsWorkspace->getNumDims(), rhsWorkspace->getNumDims());
    for(size_t i = 0; i < ndims; ++i)
    {
      auto ws1Dim = lhsWorkspace->getDimension(i);
      auto ws2Dim = rhsWorkspace->getDimension(i);

      if(ws1Dim->getNBins() != ws2Dim->getNBins())
      {
        throw std::runtime_error(lhsWorkspace->name() + " and " + rhsWorkspace->name() + " do not have the same number of bins.");
      }

      if(ws1Dim->getName() != ws2Dim->getName())
      {
        throw std::runtime_error("Dimension names do not match up.");
      }
      auto ws1DimIntegrated = getFirstNonIntegratedDimension(lhsWorkspace);
      auto ws2DimIntegrated = getFirstNonIntegratedDimension(rhsWorkspace);

      if (ws1DimIntegrated->getMaximum() != ws2DimIntegrated->getMaximum())
      {
        throw std::runtime_error("Max values in the two non-integrated dimensions of the combining workspaces are not equal.");
      }
      if (ws1DimIntegrated->getMinimum() != ws2DimIntegrated->getMinimum())
      {
        throw std::runtime_error("Min values in the two non-integrated dimensions of the combining workspaces are not equal.");
      }
    }
  }

  /**
  Reconstruct a workspace as a truely 1D workspace. This is required if one of the dimensions has been integrated-out.
  A new workspace is fabricated to be identical to the original, but missing the input integrated dimension.
  @param ws : input workspace to flatten.
  @return flattened 1D Histo workspace.
  */
  MDHistoWorkspace_sptr Stitch1DMD::trimOutIntegratedDimension(IMDHistoWorkspace_sptr ws)
  {
    auto dim = getFirstNonIntegratedDimension(ws);
    auto nbins = dim->getNBins();
        
    Mantid::MantidVec signals(nbins);
    Mantid::MantidVec errors(nbins);
    Mantid::MantidVec extents = boost::assign::list_of(dim->getMinimum())(dim->getMaximum());
    std::vector<int> vecNBins = boost::assign::list_of(static_cast<int>(nbins));
    std::vector<std::string> names = boost::assign::list_of(dim->getName());
    std::vector<std::string> units = boost::assign::list_of(dim->getUnits());

    for(size_t index = 0; index < nbins; ++index)

    {
      signals[index] = ws->signalAt(index);
      double errSq = ws->errorSquaredAt(index);
      double err = std::sqrt(errSq);
      errors[index] = err;
    }

    return create1DHistoWorkspace(signals, errors, extents, vecNBins, names, units);
  };

  /**
  Creates a 1D MDHistoWorkspace from the inputs arrays. Runs the CreateMDHistoWorkspace algorithm as a ChildAlgorithm.
  @param signals: signal collection
  @param errors: error collection
  @param extents: extents collection
  @param vecNBins: number of bins collection
  @param names: names collection
  @param units: units collection
  */
  MDHistoWorkspace_sptr Stitch1DMD::create1DHistoWorkspace(const MantidVec& signals,const MantidVec& errors, const MantidVec& extents, const std::vector<int>& vecNBins, const std::vector<std::string> names, const std::vector<std::string>& units)
  {
    IAlgorithm_sptr createMDHistoWorkspace = this->createChildAlgorithm("CreateMDHistoWorkspace");
    createMDHistoWorkspace->initialize();
    createMDHistoWorkspace->setProperty("SignalInput", signals);
    createMDHistoWorkspace->setProperty("ErrorInput", errors);
    createMDHistoWorkspace->setProperty("Dimensionality", 1);
    createMDHistoWorkspace->setProperty("Extents", extents);
    createMDHistoWorkspace->setProperty("NumberOfBins", vecNBins);
    createMDHistoWorkspace->setProperty("Names", names);
    createMDHistoWorkspace->setProperty("Units", units);
    createMDHistoWorkspace->executeAsChildAlg();
    IMDHistoWorkspace_sptr outWS = createMDHistoWorkspace->getProperty("OutputWorkspace");
    return boost::dynamic_pointer_cast<MDHistoWorkspace>(outWS);
  }

  /**
  Sum over the signal value in the specified input workspace between a start and end position.
  @param ws : workspace to integrate over
  @param fractionLow : Low fraction along the 1D axis to start integration from.
  @param fractionHigh : High fraction along the 1D axis to stop integration at.
  @return the integrated/summed value.
  */
  double Stitch1DMD::integrateOver(IMDHistoWorkspace_sptr ws, const double& fractionLow, const double& fractionHigh)
  {
    auto dim = getFirstNonIntegratedDimension(ws);
    size_t nbins = dim->getNBins();
    int binLow = int(double(nbins) * fractionLow);
    int binHigh = int(double(nbins) * fractionHigh);
    double sumSignal = 0.0;
    for(int index = binLow; index < binHigh; ++index)
    {
      sumSignal += ws->signalAt(index);
    }
    return sumSignal;
  }

  /**
  Overlay the overlap 1D workspace over the original 1D workspace. overlap shouuld overwrite values on the sum workspace.
  @param original : Original 1D workspace to be overwritten only in the region of the overlap.
  @param overlap : Overlap 1D workspace to overwrite with
  */
  void Stitch1DMD::overlayOverlap(MDHistoWorkspace_sptr original, IMDHistoWorkspace_sptr overlap)
  {
    const auto targetDim = original->getDimension(0);
    const double targetQMax = targetDim->getMaximum();
    const double targetQMin = targetDim->getMinimum();
    const size_t targetNbins = targetDim->getNBins();
    const double targetStep = double(targetNbins) / (targetQMax - targetQMin); 
    const double targetC = -1 * targetStep * targetQMin;

    const auto overlapDim = overlap->getDimension(0);
    const double overlapQMax = overlapDim->getMaximum();
    const double overlapQMin = overlapDim->getMinimum();
    const size_t overlapNBins = overlapDim->getNBins();
    const double overlapStep = (overlapQMax - overlapQMin) / double(overlapNBins);
    const double overlapC = overlapQMin;

    for(size_t i = 0; i < overlapNBins; ++i)
    {
      // Calculate the q value for each index in the overlap region.
      const double q = (overlapStep * double(i)) + overlapC;
      // Find the target index by recentering (adding 0.5) and then truncating to an integer.
      size_t targetIndex = size_t((targetStep * q) + targetC + 0.5) ;
      // Overwrite signal
      original->setSignalAt(targetIndex, overlap->signalAt(i));
      // Overwrite error
      original->setErrorSquaredAt(targetIndex, overlap->errorSquaredAt(i));
    }
  }

  /**
  Extract the overlap region as a distinct workspace.
  @param ws: Workspace to extract region
  @param fractionLow : Low fraction to start slicing from
  @param fractionHigh : High fraction to stop slicing from
  @return MDHistoWorkspace encompasing the overlap region only.
  */
  MDHistoWorkspace_sptr Stitch1DMD::extractOverlapAsWorkspace(IMDHistoWorkspace_sptr ws, const double& fractionLow, const double& fractionHigh)
  {
    auto dim = getFirstNonIntegratedDimension(ws);
    auto nbins = dim->getNBins();
    int binLow = int(double(nbins) * fractionLow);
    int binHigh = int(double(nbins) * fractionHigh);
    if(binLow == binHigh)
    {
      throw std::invalid_argument("There are no complete bins in the overlap region specified by fraction low, fraction high");
    }
    double step = ( dim->getMaximum() - dim->getMinimum() )/ double(nbins);
    double qLow = (double(binLow) * step) + dim->getMinimum();
    double qHigh = (double(binHigh) * step) + dim->getMinimum();

    const int binRange = binHigh - binLow;
    Mantid::MantidVec signals(binRange);
    Mantid::MantidVec errors(binRange);
    Mantid::MantidVec extents = boost::assign::list_of(qLow)(qHigh);
    std::vector<int> vecNBins(1, binRange);
    std::vector<std::string> names = boost::assign::list_of(dim->getName());
    std::vector<std::string> units = boost::assign::list_of(dim->getUnits());

    int counter = 0;
    for(int index = binLow; index < binHigh; ++index)
    {
      signals[counter] = ws->signalAt(index);
      errors[counter] =  std::sqrt(ws->errorSquaredAt(index));
      ++counter;
    }

    return create1DHistoWorkspace(signals, errors, extents, vecNBins, names, units);
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
  */
  void Stitch1DMD::exec()
  {
    MDHistoWorkspace_sptr workspace1;
    MDHistoWorkspace_sptr workspace2;
    {
      IMDHistoWorkspace_sptr temp1 = this->getProperty("LHSWorkspace");
      IMDHistoWorkspace_sptr temp2 = this->getProperty("RHSWorkspace");
      workspace1 = boost::dynamic_pointer_cast<MDHistoWorkspace>(temp1);
      workspace2 = boost::dynamic_pointer_cast<MDHistoWorkspace>(temp2);
    }

    checkIndividualWorkspace(workspace1);
    checkIndividualWorkspace(workspace2);
    checkBothWorkspaces(workspace1, workspace2);

    double startOverlap = this->getProperty("StartOverlap");
    double endOverlap = this->getProperty("EndOverlap"); 
    bool b_manualScaleFactor = this->getProperty("UseManualScaleFactor");
    bool b_scaleWorkspace2 = this->getProperty("ScaleRHSWorkspace");

    if(startOverlap >= endOverlap)
    {
      throw std::runtime_error("StartOverlap must be < EndOverlap");
    }

    MDHistoWorkspace_sptr ws1_Flattened = this->trimOutIntegratedDimension(workspace1);
    MDHistoWorkspace_sptr ws2_Flattened = this->trimOutIntegratedDimension(workspace2);

    double ws1_Overlap = this->integrateOver(ws1_Flattened, startOverlap, endOverlap);
    double ws2_Overlap = this->integrateOver(ws2_Flattened, startOverlap, endOverlap);

    double scaleFactor = 0;
    MDHistoWorkspace_sptr  scaledWorkspace1;
    MDHistoWorkspace_sptr  scaledWorkspace2;

    if (b_manualScaleFactor)
    {
      scaleFactor = this->getProperty("ManualScaleFactor");
      if (b_scaleWorkspace2)
      {
        scaledWorkspace1 = ws1_Flattened;
        scaledWorkspace2 = ws2_Flattened;
        scaledWorkspace2->multiply(scaleFactor, 0);
      }
      else
      {
        scaledWorkspace1 = ws1_Flattened;
        scaledWorkspace1->multiply(scaleFactor, 0);
        scaledWorkspace2 = ws2_Flattened;
      }
    }
    else
    {
      if (b_scaleWorkspace2)
      {
        scaleFactor = (ws1_Overlap / ws2_Overlap);
        scaledWorkspace1 = ws1_Flattened;
        scaledWorkspace2 = ws2_Flattened;
        scaledWorkspace2->multiply(scaleFactor, 0);
      }
      else
      {
        scaleFactor = (ws2_Overlap / ws1_Overlap);
        scaledWorkspace1 = ws1_Flattened;
        scaledWorkspace1->multiply(scaleFactor, 0);
        scaledWorkspace2 = ws2_Flattened;
      }
    } 

    this->setProperty("OutScaleFactor", scaleFactor);
    auto workspace1Overlap = this->extractOverlapAsWorkspace(scaledWorkspace1, startOverlap, endOverlap);
    auto workspace2Overlap = this->extractOverlapAsWorkspace(scaledWorkspace2, startOverlap, endOverlap);

    IAlgorithm_sptr weightedMeanMD = this->createChildAlgorithm("WeightedMeanMD");
    weightedMeanMD->initialize();
    weightedMeanMD->setProperty("LHSWorkspace", workspace1Overlap);
    weightedMeanMD->setProperty("RHSWorkspace", workspace2Overlap);
    weightedMeanMD->executeAsChildAlg();
    IMDWorkspace_sptr weightedMeanOverlap = weightedMeanMD->getProperty("OutputWorkspace");

    IAlgorithm_sptr plusMD = this->createChildAlgorithm("PlusMD");
    plusMD->initialize();
    plusMD->setProperty("LHSWorkspace", scaledWorkspace1);
    plusMD->setProperty("RHSWorkspace", scaledWorkspace2);
    plusMD->executeAsChildAlg();
    IMDWorkspace_sptr sum = plusMD->getProperty("OutputWorkspace");

    overlayOverlap(boost::dynamic_pointer_cast<MDHistoWorkspace>(sum), boost::dynamic_pointer_cast<IMDHistoWorkspace>(weightedMeanOverlap));
    this->setProperty("OutputWorkspace", sum);
  }

} // namespace MDAlgorithms
} // namespace Mantid
