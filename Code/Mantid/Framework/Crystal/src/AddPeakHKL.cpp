#include "MantidCrystal/AddPeakHKL.h"

namespace Mantid
{
namespace Crystal
{

  using Mantid::Kernel::Direction;
  using Mantid::API::WorkspaceProperty;

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(AddPeakHKL)



  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  AddPeakHKL::AddPeakHKL()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  AddPeakHKL::~AddPeakHKL()
  {
  }


  //----------------------------------------------------------------------------------------------

  /// Algorithms name for identification. @see Algorithm::name
  const std::string AddPeakHKL::name() const { return "AddPeakHKL"; }

  /// Algorithm's version for identification. @see Algorithm::version
  int AddPeakHKL::version() const { return 1;};

  /// Algorithm's category for identification. @see Algorithm::category
  const std::string AddPeakHKL::category() const { return TODO: FILL IN A CATEGORY;}

  /// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
  const std::string AddPeakHKL::summary() const { return TODO: FILL IN A SUMMARY;};

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void AddPeakHKL::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input), "An input workspace.");
    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void AddPeakHKL::exec()
  {
    // TODO Auto-generated execute stub
  }



} // namespace Crystal
} // namespace Mantid