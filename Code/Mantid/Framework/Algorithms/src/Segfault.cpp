#include "MantidAlgorithms/Segfault.h"

namespace Mantid
{
namespace Algorithms
{

  using Mantid::Kernel::Direction;
  using Mantid::API::WorkspaceProperty;

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(Segfault)



  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  Segfault::Segfault()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  Segfault::~Segfault()
  {
  }


  //----------------------------------------------------------------------------------------------

  /// Algorithms name for identification. @see Algorithm::name
  const std::string Segfault::name() const { return "Segfault"; }

  /// Algorithm's version for identification. @see Algorithm::version
  int Segfault::version() const { return 1;};

  /// Algorithm's category for identification. @see Algorithm::category
  const std::string Segfault::category() const { return TODO: FILL IN A CATEGORY;}

  /// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
  const std::string Segfault::summary() const { return TODO: FILL IN A SUMMARY;};

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void Segfault::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input), "An input workspace.");
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void Segfault::exec()
  {
    // TODO Auto-generated execute stub
  }



} // namespace Algorithms
} // namespace Mantid