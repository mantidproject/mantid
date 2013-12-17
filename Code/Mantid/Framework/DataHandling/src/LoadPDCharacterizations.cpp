/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidDataHandling/LoadPDCharacterizations.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Property.h"

using Mantid::API::FileProperty;
using Mantid::API::ITableWorkspace;
using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::ArrayProperty;
using Mantid::Kernel::Direction;

namespace Mantid
{
namespace DataHandling
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(LoadPDCharacterizations)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  LoadPDCharacterizations::LoadPDCharacterizations()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  LoadPDCharacterizations::~LoadPDCharacterizations()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string LoadPDCharacterizations::name() const { return "LoadPDCharacterizations";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int LoadPDCharacterizations::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string LoadPDCharacterizations::category() const { return "DataHandling";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void LoadPDCharacterizations::initDocs()
  {
    this->setWikiSummary("TODO: Enter a quick description of your algorithm.");
    this->setOptionalMessage("TODO: Enter a quick description of your algorithm.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void LoadPDCharacterizations::init()
  {
    std::vector<std::string> exts;
    exts.push_back(".txt");
    declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
                    "Characterizations file");

    declareProperty(new WorkspaceProperty<ITableWorkspace>("CharacterizationWorkspace", "",
                                                                     Direction::Output),
                    "Output for the information of characterizations and runs");

    declareProperty("IParmFilename", std::string(""), "Name of the gsas instrument parameter file.", Direction::Output);
    declareProperty("PrimaryFlightPath", EMPTY_DBL(), "Primary flight path L1 of the powder diffractomer. ", Direction::Output);
    declareProperty(new ArrayProperty<int32_t>("SpectrumIDs", Direction::Output),
                    "Spectrum IDs (note that it is not detector ID or workspace indices). The list must be either empty or have a size equal to input workspace's histogram number. ");
    declareProperty(new ArrayProperty<double>("L2", Direction::Output),
                    "Seconary flight (L2) paths for each detector.  Number of L2 given must be same as number of histogram.");
    declareProperty(new ArrayProperty<double>("Polar", Direction::Output),
                    "Polar angles (two thetas) for detectors. Number of 2theta given must be same as number of histogram.");
    declareProperty(new ArrayProperty<double>("Azimuthal", Direction::Output),
                    "Azimuthal angles (out-of-plane) for detectors. "
                    "Number of azimuthal angles given must be same as number of histogram.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void LoadPDCharacterizations::exec()
  {
    // TODO Auto-generated execute stub
  }



} // namespace DataHandling
} // namespace Mantid
