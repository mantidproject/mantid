/*WIKI* 


Modifies an ISIS detector dot data file, so that the detector positions are as in the given workspace.


*WIKI*/
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/ModifyDetectorDotDatFile.h"
// #include "MantidDataHandling/LoadDetectorInfo.h"
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
    out << str << "\n";
    // check that we have at least 1 detector and six columns
    if( detectorCount < 1 || numColumns < 6) {
          out.close();
          in.close();
          throw Exception::FileError("Incompatible file format found when reading line 2 in the input file", inputFilename);
    }

    // Copy column title line
    getline( in, str );
    out << str << "\n";

    int i=0;

    // Format details
    int pOffset = 3; // Precision of Offset
    int pOther = 5; // Precision of Other floats
    int wDet = 9; // Field width of Detector ID
    int wOff = 8; // Field width of Offset
    int wRad = 10; // Field width of Radius
    int wCode = 6; // Field width of Code
    int wAng = 12; // Field width of angles

    // Read input file line by line, modify line as necessary and put line into output file
    while( getline( in, str ) ){

       std::istringstream istr(str);
    
       int detID;
       float offset;
       int code;
       float dump; // ignored data

       if (str.empty() || str[0] == '#')
       { // comments and empty lines are allowed and just copied
           out << str << "\n";
           continue;
       }

       // First six columns in the file, the detector ID and a code for the type of detector CODE = 3 (psd gas tube)
       istr >> detID >> offset >> dump >> code >> dump >> dump;

       if( code == 3 ){
          // This is detector will look for it in workspace and if found use its position
          std::streampos width = istr.tellg(); // Amount of string to replace
          // Some experimenting with line manipulation
          std::ostringstream oss;
          oss  << std::fixed << std::right ;
          oss.precision(pOffset);
          oss << std::setw(wDet) << detID << std::setw(wOff) << offset;
          oss.precision(pOther);
          oss << std::setw(wRad) << (55.0+i)/6.0 << std::setw(wCode) << code << std::setw(wAng) << (360.0+i)/7.01 << std::setw(wAng) << -(400.0-i)/7.0 ;
          std::string prefix = oss.str();
          std::string suffix = str.substr( width, std::string::npos );
          out << prefix << suffix << "\n";
          i++;
       } else {
       // We do not modify any other type of line
          out << str << "\n";
       }
    }

    out.close();
    in.close();

  }



} // namespace Mantid
} // namespace DataHandling

