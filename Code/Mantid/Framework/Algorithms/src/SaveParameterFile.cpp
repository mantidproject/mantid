#include "MantidAlgorithms/SaveParameterFile.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidGeometry/Instrument.h"

namespace Mantid
{
namespace Algorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(SaveParameterFile)

  using namespace Kernel;
  using namespace API;

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SaveParameterFile::SaveParameterFile()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SaveParameterFile::~SaveParameterFile()
  { }

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string SaveParameterFile::name() const { return "SaveParameterFile";};

  /// Algorithm's version for identification. @see Algorithm::version
  int SaveParameterFile::version() const { return 1;};

  /// Algorithm's category for identification. @see Algorithm::category
  const std::string SaveParameterFile::category() const { return "DataHandling\\Instrument";}

  //----------------------------------------------------------------------------------------------

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void SaveParameterFile::init()
  {
    declareProperty(new WorkspaceProperty<>("Workspace","",Direction::Input,boost::make_shared<InstrumentValidator>()),
    "Workspace to save the instrument parameters from.");

    std::vector<std::string> exts;
    exts.push_back(".xml");

    declareProperty(new API::FileProperty("Filename","", API::FileProperty::Save, exts),
    "The name of the file into which the instrument parameters will be saved.");

    declareProperty("SaveLocationParameters", false, "Save the location parameters used to calibrate the instrument.", Direction::Input);
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SaveParameterFile::exec()
  {
    throw Kernel::Exception::NotImplementedError("This algorithm has not yet been implemented.");
  }

} // namespace Algorithms
} // namespace Mantid
