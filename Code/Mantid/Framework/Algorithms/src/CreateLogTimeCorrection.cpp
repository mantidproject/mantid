#include "MantidAlgorithms/CreateLogTimeCorrection.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/FileProperty.h"


using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

using namespace std;

namespace Mantid
{
namespace Algorithms
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  CreateLogTimeCorrection::CreateLogTimeCorrection()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  CreateLogTimeCorrection::~CreateLogTimeCorrection()
  {
  }
  
  //----------------------------------------------------------------------------------------------
  void CreateLogTimeCorrection::init()
  {
    declareProperty("Instrument", "", "Name of the instrument to create the correction from.");

    auto fileprop = new FileProperty("OutputFilename", "", FileProperty::Save);
    declareProperty(fileprop, "Name of the output time correction file.");

    auto wsprop = new WorkspaceProperty<Workspace2D>("OutpoutWorkspace", "Anonymous", Direction::Output);
    declareProperty(wsprop, "Name of the output workspace containing the corrections.");

    return;
  }

  //----------------------------------------------------------------------------------------------
  void CreateLogTimeCorrection::exec()
  {



  }


} // namespace Algorithms
} // namespace Mantid
