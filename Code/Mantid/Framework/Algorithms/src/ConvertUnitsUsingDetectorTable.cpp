#include "MantidAlgorithms/ConvertUnitsUsingDetectorTable.h"

namespace Mantid
{
namespace Algorithms
{

  using Mantid::Kernel::Direction;
  using Mantid::API::WorkspaceProperty;

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(ConvertUnitsUsingDetectorTable)



  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ConvertUnitsUsingDetectorTable::ConvertUnitsUsingDetectorTable()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ConvertUnitsUsingDetectorTable::~ConvertUnitsUsingDetectorTable()
  {
  }


  //----------------------------------------------------------------------------------------------

  /// Algorithms name for identification. @see Algorithm::name
  const std::string ConvertUnitsUsingDetectorTable::name() const { return "ConvertUnitsUsingDetectorTable"; }

  /// Algorithm's version for identification. @see Algorithm::version
  int ConvertUnitsUsingDetectorTable::version() const { return 1;}

  /// Algorithm's category for identification. @see Algorithm::category
  const std::string ConvertUnitsUsingDetectorTable::category() const { return "Transforms\\Units";}

  /// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
  const std::string ConvertUnitsUsingDetectorTable::summary() const { return "Performs a unit change on the X values of a workspace";}

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void ConvertUnitsUsingDetectorTable::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input), "An input workspace.");
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void ConvertUnitsUsingDetectorTable::exec()
  {
    // TODO Auto-generated execute stub
  }



} // namespace Algorithms
} // namespace Mantid
