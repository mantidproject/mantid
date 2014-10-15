#include "MantidDataHandling/SavePDFGui.h"

namespace Mantid
{
namespace DataHandling
{

  using Mantid::Kernel::Direction;
  using Mantid::API::WorkspaceProperty;

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(SavePDFGui)



  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SavePDFGui::SavePDFGui()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SavePDFGui::~SavePDFGui()
  {
  }


  //----------------------------------------------------------------------------------------------


  /// Algorithm's version for identification. @see Algorithm::version
  int SavePDFGui::version() const { return 1;};

  /// Algorithm's category for identification. @see Algorithm::category
  const std::string SavePDFGui::category() const { return TODO: FILL IN A CATEGORY;}

  /// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
  const std::string SavePDFGui::summary() const { return TODO: FILL IN A SUMMARY;};

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void SavePDFGui::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input), "An input workspace.");
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SavePDFGui::exec()
  {
    // TODO Auto-generated execute stub
  }



} // namespace DataHandling
} // namespace Mantid