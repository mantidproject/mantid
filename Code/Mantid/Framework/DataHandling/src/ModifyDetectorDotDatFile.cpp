/*WIKI* 


Modifies an ISIS detector dot data file, so that the detector positions are as in the given workspace.


*WIKI*/
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/ModifyDetectorDotDatFile.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/System.h"
#include <fstream>
#include "MantidAPI/Workspace.h"
#include "MantidAPI/ExperimentInfo.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace Mantid
{
namespace DataHandling
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(ModifyDetectorDotDatFile)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ModifyDetectorDotDatFile::ModifyDetectorDotDatFile()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ModifyDetectorDotDatFile::~ModifyDetectorDotDatFile()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void ModifyDetectorDotDatFile::initDocs()
  {
    this->setWikiSummary("Modifies an ISIS detector dot data file, so that the detector positions are as in the given workspace");
    this->setOptionalMessage("Modifies an ISIS detector dot data file, so that the detector positions are as in the given workspace");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void ModifyDetectorDotDatFile::init()
  {
    declareProperty(new WorkspaceProperty<Workspace>("InputWorkspace","",Direction::Input), "Workspace with detectors in the positions to be put into the detector dot dat file");

    std::vector<std::string> exts;
    exts.push_back(".dat");
    exts.push_back(".txt");

    declareProperty(new FileProperty("InputFilename", "", FileProperty::Load, exts), 
        "Path to a detector dot dat file.");

    declareProperty(new FileProperty("OutputFilename", "", FileProperty::Save, exts),
        "Path to the modified detector dot dat file.");

  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void ModifyDetectorDotDatFile::exec()
  {
    std::string inputFilename = getPropertyValue("InputFilename");
    std::string outputFilename = getPropertyValue("OutputFilename");

    Workspace_sptr ws1 = getProperty("InputWorkspace");
    ExperimentInfo_sptr ws = boost::dynamic_pointer_cast<ExperimentInfo>(ws1);

    // Check instrument
    Instrument_const_sptr inst = ws->getInstrument();
    if (!inst) throw std::runtime_error("No instrument in the Workspace. Cannot modify detector dot dat file");

    // Open files
    std::ifstream in;
    in.open( inputFilename.c_str());
    if(!in) {
        throw Exception::FileError("Can't open input file", inputFilename);
    }
    std::ofstream out;
    out.open( outputFilename.c_str());
    if(!out) {
        in.close();
        throw Exception::FileError("Can't open output file", outputFilename);
    }

    // Read first line, modify it and put into output file
    std::string str;
    getline( in, str );
    out << str << " and modified by MANTID algorithm ModifyDetectorDotDatFile \n";

    // Read second line to check number of detectors and columns
    int detectorCount, numColumns;
    getline( in, str );
    std::istringstream header2(str);
    header2 >> detectorCount >> numColumns;
    // check that we have at least 1 detector and five columns
    if( detectorCount < 1 || numColumns < 5) {
          throw Exception::FileError("Incompatible file format found when reading line 2 in the input file", inputFilename);
    }
    out << str << "\n";

    // Read input file line by line, modify line as necessary and put line into output file
    while( getline( in, str ) ){
       // We do not yet modify
       out << str << "\n";
    }

    out.close();
    in.close();

  }



} // namespace Mantid
} // namespace DataHandling

