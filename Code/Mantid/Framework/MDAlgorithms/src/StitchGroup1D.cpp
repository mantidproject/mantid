/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidMDAlgorithms/StitchGroup1D.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/MandatoryValidator.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace MDAlgorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(StitchGroup1D)
  
  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  StitchGroup1D::StitchGroup1D()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  StitchGroup1D::~StitchGroup1D()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string StitchGroup1D::name() const { return "StitchGroup1D";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int StitchGroup1D::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string StitchGroup1D::category() const { return "Reflectometry\\ISIS";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void StitchGroup1D::initDocs()
  {
    this->setWikiSummary("Stitch two MD ReflectometryQ group workspaces together");
    this->setOptionalMessage("Sticch two MD ReflectometryQ group workspaces together.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void StitchGroup1D::init()
  {
    declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("RHSWorkspace", "", Direction::Input), "Input MD Histo Workspace");
    declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("LHSWorkspace", "", Direction::Input), "Input MD Histo Workspace");
    declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("OutputWorkspace", "", Direction::Input), "Input MD Histo Workspace");
    auto overlap_validator = new CompositeValidator();
    overlap_validator->add(boost::make_shared<BoundedValidator<double> >(0.0, 1.0));
    overlap_validator->add(boost::make_shared<MandatoryValidator<double> >());    

    declareProperty("StartOverlap", 0.0, overlap_validator->clone(), "Fraction along axis to start overlap. 0 to 1.");
    declareProperty("EndOverlap", 0.1, overlap_validator->clone(), "Fraction along axis to end overlap. 0 to 1.");
    declareProperty("ExpectGroupWorkspaces", false, "True if the input workspaces expected to be group workspaces.");
    declareProperty("GroupWorkspaceIndex", 0, "Index of the workspace in the group workspaces");
    declareProperty("ScaleRHSWorkspace", true, "Scaling either with respect to RHS or LHS Workspace.");
    declareProperty("UseManualScaleFactor", false, "True to use a provided value for the scale factor.");
    declareProperty("ManualScaleFactor", 1.0, "Provided value for the scale factor.");
    declareProperty("OutScaleFactor", -2.0, "The actual used value for the scaling factor.", Direction::Output); 
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void StitchGroup1D::exec()
  {
    // TODO Auto-generated execute stub
  }



} // namespace MDAlgorithms
} // namespace Mantid