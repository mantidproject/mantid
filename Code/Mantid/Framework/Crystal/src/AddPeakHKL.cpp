#include "MantidCrystal/AddPeakHKL.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/IPeak.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/V3D.h"

namespace Mantid
{
namespace Crystal
{

  using namespace Mantid::Kernel;
  using namespace Mantid::API;

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
  const std::string AddPeakHKL::category() const { return "Crystal";}

  /// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
  const std::string AddPeakHKL::summary() const { return "Add a peak in the hkl frame";};

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void AddPeakHKL::init()
  {
    declareProperty(new WorkspaceProperty<Mantid::API::IPeaksWorkspace>("Workspace","",Direction::InOut), "An input workspace.");
    declareProperty(new ArrayProperty<double>("HKL", boost::make_shared<ArrayLengthValidator<double> > (3)), "HKL point to add");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void AddPeakHKL::exec()
  {
    IPeaksWorkspace_sptr peakWS = this->getProperty("Workspace");
    const std::vector<double> hklValue = this->getProperty("HKL");
    IPeak * peak = peakWS->createPeakHKL(V3D(hklValue[0], hklValue[1], hklValue[2]));
    peakWS->addPeak(*peak);
    delete peak;
  }



} // namespace Crystal
} // namespace Mantid
