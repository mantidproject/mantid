/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidCurveFitting/SplineInterpolation.h"

namespace Mantid
{
namespace CurveFitting
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(SplineInterpolation);
  
  using namespace API;
  using namespace Kernel;

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SplineInterpolation::SplineInterpolation()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SplineInterpolation::~SplineInterpolation()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string SplineInterpolation::name() const { return "SplineInterpolation";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int SplineInterpolation::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string SplineInterpolation::category() const { return "General";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void SplineInterpolation::initDocs()
  {
    this->setWikiSummary("TODO: Enter a quick description of your algorithm.");
    this->setOptionalMessage("TODO: Enter a quick description of your algorithm.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void SplineInterpolation::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input), "The workspace on which to perform interpolation the algorithm.");
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "The workspace containing the calculated points and derivatives");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SplineInterpolation::exec()
  {
    // TODO Auto-generated execute stub
  }



} // namespace CurveFitting
} // namespace Mantid
