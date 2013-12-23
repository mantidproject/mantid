/*WIKI*
 *
 * Peforms index operations on a workspace which involve cropping out spectra and summing spectra together. See [[MultiFileLoading]] for the syntax to use.
 *
*WIKI*/

#include "MantidAlgorithms/PerformIndexOperations.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Strings.h"
#include "boost/regex.hpp"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace Algorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(PerformIndexOperations)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  PerformIndexOperations::PerformIndexOperations()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  PerformIndexOperations::~PerformIndexOperations()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string PerformIndexOperations::name() const { return "PerformIndexOperations";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int PerformIndexOperations::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string PerformIndexOperations::category() const { return "Algorithms;Transforms;Splitting";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void PerformIndexOperations::initDocs()
  {
    this->setWikiSummary("Process the workspace according to the Index operations provided.");
    this->setOptionalMessage(this->getWikiSummary());
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void PerformIndexOperations::init()
  {
    declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input), "Input to processes workspace.");
    declareProperty(new PropertyWithValue<std::string>("ProcessingInstructions", "", Direction::Input), "Processing instructions. See full instruction list.");
    declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output), "Output processed workspace");
  }

    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
     */
    void PerformIndexOperations::exec()
    {
      MatrixWorkspace_sptr inputWorkspace = this->getProperty("InputWorkspace");
      const std::string processingInstructions = this->getProperty("ProcessingInstructions");

      boost::regex re("^\\s*[0-9]\\s*$|^(\\s*,*[0-9](\\s*(,|:|\\+|\\-)\\s*)[0-9])*$");
      if (!boost::regex_match(processingInstructions, re))
      {
        throw std::invalid_argument(
            "ProcessingInstructions are not well formed: " + processingInstructions);
      }

      auto cloneWS = this->createChildAlgorithm("CloneWorkspace");
      cloneWS->initialize();
      cloneWS->setProperty("InputWorkspace", inputWorkspace);
      cloneWS->execute();
      Workspace_sptr tmp = cloneWS->getProperty("OutputWorkspace");
      MatrixWorkspace_sptr outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(tmp);

      // Loop over pairs of detector index ranges. Peform the cropping and then conjoin the results into a single workspace.
      auto allIndexes = Kernel::Strings::parseRange(processingInstructions, ",", ":");
      for (size_t i = 0; i < allIndexes.size(); ++i)
      {
        auto cropWorkspaceAlg = this->createChildAlgorithm("CropWorkspace");
        cropWorkspaceAlg->initialize();
        cropWorkspaceAlg->setProperty("InputWorkspace", inputWorkspace);
        cropWorkspaceAlg->setProperty("StartWorkspaceIndex", allIndexes[i]);
        cropWorkspaceAlg->setProperty("EndWorkspaceIndex", allIndexes[i]);
        cropWorkspaceAlg->execute();
        MatrixWorkspace_sptr subRange = cropWorkspaceAlg->getProperty("OutputWorkspace");
        if (i == 0)
        {
          outWS = subRange;
        }
        else
        {
          auto conjoinWorkspaceAlg = this->createChildAlgorithm("ConjoinWorkspaces");
          conjoinWorkspaceAlg->initialize();
          conjoinWorkspaceAlg->setProperty("InputWorkspace1", outWS);
          conjoinWorkspaceAlg->setProperty("InputWorkspace2", subRange);
          conjoinWorkspaceAlg->execute();
          outWS = conjoinWorkspaceAlg->getProperty("InputWorkspace1");
        }
      }

      this->setProperty("OutputWorkspace", outWS);
    }



} // namespace Algorithms
} // namespace Mantid
