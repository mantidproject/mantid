//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/SaveNISTDAT.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include <fstream>  // used to get ofstream
#include <iostream>

namespace Mantid
{
namespace DataHandling
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveNISTDAT)

/// Sets documentation strings for this algorithm
void SaveNISTDAT::initDocs()
{
  this->setWikiSummary("Save I(Qx,Qy) data to a text file compatible with NIST and DANSE readers.");
  this->setOptionalMessage("Save I(Qx,Qy) data to a text file compatible with NIST and DANSE readers.");
}

using namespace Kernel;
using namespace API;
using namespace Geometry;

void SaveNISTDAT::init()
{
  CompositeValidator<> *wsValidator = new CompositeValidator<>;
  wsValidator->add(new WorkspaceUnitValidator<>("MomentumTransfer"));
  wsValidator->add(new HistogramValidator<>);
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input, wsValidator));
  declareProperty(new FileProperty("Filename", "",FileProperty::Save, ".dat"),
    "The filename of the output text file" );
}

void SaveNISTDAT::exec()
{
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  std::string filename = getPropertyValue("Filename");

  // prepare to save to file
  std::ofstream out_File(filename.c_str());
  if (!out_File)
  {
    g_log.error("Failed to open file:" + filename);
    throw Exception::FileError("Failed to open file:" , filename);
  }
  out_File << "Qx - Qy - I(Qx,Qy)\r\n";
  out_File << "ASCII data\r\n";

  // Set up the progress reporting object
  Progress progress(this,0.0,1.0,1.0);

  if ( inputWS->axes() > 1 && inputWS->getAxis(1)->isNumeric() )
  {
    const Axis& axis = *inputWS->getAxis(1);
    const int axisLength = axis.length();
    for (int i = 0; i < axisLength-1; i++)
    {
      const double qy = (axis(i)+axis(i+1))/2.0;
      const MantidVec& XIn = inputWS->readX(i);
      const MantidVec& YIn = inputWS->readY(i);

      for ( unsigned int j = 0; j < XIn.size()-1; j++)
      {
        // Exclude NaNs
        if (YIn[j]==YIn[j])
        {
          out_File << (XIn[j]+XIn[j+1])/2.0;
          out_File << "  " << qy;
          out_File << "  " << YIn[j] << "\r\n";
        }
      }
    }
  }
  out_File.close();
  progress.report("Save I(Qx,Qy)");
}

} // namespace Algorithms
} // namespace Mantid
