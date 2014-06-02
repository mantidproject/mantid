/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidCurveFitting/SeqDomainDemonstration.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidCurveFitting/SeqDomainTestFunction.h"


namespace Mantid
{
namespace CurveFitting
{

using namespace API;
using namespace Kernel;
  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(SeqDomainDemonstration)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SeqDomainDemonstration::SeqDomainDemonstration()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SeqDomainDemonstration::~SeqDomainDemonstration()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string SeqDomainDemonstration::name() const { return "SeqDomainDemonstration";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int SeqDomainDemonstration::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string SeqDomainDemonstration::category() const { return "SINQ\\Poldi"; }

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void SeqDomainDemonstration::initDocs()
  {
    this->setWikiSummary("TODO: Enter a quick description of your algorithm.");
    this->setOptionalMessage("TODO: Enter a quick description of your algorithm.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void SeqDomainDemonstration::init()
  {
    declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input), "An input workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SeqDomainDemonstration::exec()
  {
      MatrixWorkspace_sptr ws = getProperty("InputWorkspace");

      std::vector<double> slopes(40);
      for(size_t i = 0; i < slopes.size(); ++i) {
          slopes[i] = static_cast<double>(i);
      }

      IFunction_sptr fun(new SeqDomainTestFunction);
      fun->initialize();
      for(size_t i = 0; i < slopes.size(); ++ i) {
        fun->setParameter(i, static_cast<double>(i) + 1.1);
        std::cout << i << " " << fun->getParameter(i) << std::endl;
      }

      Fit fit;
      fit.initialize();

      fit.setProperty("Function",fun);
      fit.setProperty("InputWorkspace",ws);
      fit.setProperty("CreateOutput",true);
      fit.setProperty("Minimizer", "Levenberg-MarquardtMD");

      fit.execute();
  }



} // namespace CurveFitting
} // namespace Mantid
