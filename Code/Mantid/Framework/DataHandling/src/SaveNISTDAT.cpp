//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/SaveNISTDAT.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataObjects/Histogram1D.h"
#include <fstream>  // used to get ofstream
#include <iomanip>  // setw() used below
#include <iostream>
#include <vector>

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveNISTDAT)

/// Sets documentation strings for this algorithm
void SaveNISTDAT::initDocs()
{
  this->setWikiSummary("Save 2D data to a text file compatible with NIST and DANSE readers.");
  this->setOptionalMessage("Save 2D data to a text file compatible with NIST and DANSE readers.");
}

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

void SaveNISTDAT::init()
{
  CompositeValidator<> *wsValidator = new CompositeValidator<>;
  wsValidator->add(new WorkspaceUnitValidator<>("Wavelength"));
  wsValidator->add(new HistogramValidator<>);
  wsValidator->add(new InstrumentValidator<>);
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,wsValidator));
  declareProperty(new FileProperty("Filename", "",FileProperty::Save, ".dat"),
    "The filename of the output text file" );
  declareProperty(new ArrayProperty<double>("OutputBinning", new RebinParamsValidator));
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

  const int numSpec = inputWS->getNumberHistograms();

  // Set up the progress reporting object
  Progress progress(this,0.0,0.5,numSpec);

  const V3D sourcePos = inputWS->getInstrument()->getSource()->getPos();
  const V3D samplePos = inputWS->getInstrument()->getSample()->getPos();

  const int xLength = inputWS->readX(0).size();
  const double fmp=4.0*M_PI;

  //TODO: Determine the binning from the data instead of asking the user.
  // We only need to know how many bins the user wants in each direction.
  /*
  double qx_min = 0;
  double qx_max = 0;
  double qy_min = 0;
  double qy_max = 0;
  // Get the min and max Q-values to get the output binning
  PARALLEL_FOR1(inputWS)
  for (int i = 0; i < numSpec; i++)
  {
    PARALLEL_START_INTERUPT_REGION
    // Get the pixel relating to this spectrum
    IDetector_const_sptr det;
    try {
      det = inputWS->getDetector(i);
    } catch (Exception::NotFoundError&) {
      g_log.warning() << "Spectrum index " << i << " has no detector assigned to it - discarding" << std::endl;
      continue;
    }
    // If this detector is masked, skip to the next one
    if ( det->isMasked() ) continue;
    // If this detector is a monitor, skip to the next one
    if ( det->isMonitor() ) continue;

    // Calculate the Q values for the current spectrum
    V3D detPos = det->getPos()-samplePos;
    double phi = atan2(detPos.Y(),detPos.X());
    double a = cos(phi);
    double b = sin(phi);
    double sinTheta = sin( inputWS->detectorTwoTheta(det)/2.0 );
    double factor = fmp*sinTheta;
    const MantidVec& XIn = inputWS->readX(i);

    // First, first the min and max q-values
    for ( int j = 0; j < xLength-1; j++)
    {
      double q = factor*2.0/(XIn[j]+XIn[j+1]);
      double qx = q*a;
      double qy = q*b;

      if (j==0) {
        qx_min = qx;
        qx_max = qx_min;
        qy_min = qy;
        qy_max = qy_min;
      } else {
        if (qx<qx_min) qx_min = qx;
        if (qx>qx_max) qx_max = qx;
        if (qy<qy_min) qy_min = qy;
        if (qy>qy_max) qy_max = qy;
      }
    }
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  */

  // Calculate the output binning
  const std::vector<double> binParams = getProperty("OutputBinning");
  MantidVecPtr qx_vector;
  MantidVecPtr qy_vector;
  const int sizeOut = VectorHelper::createAxisFromRebinParams(binParams,qx_vector.access());
  VectorHelper::createAxisFromRebinParams(binParams,qy_vector.access());

  // Count matrix and normalization
  std::vector<std::vector<int> > normalization(sizeOut-1, std::vector<int>(sizeOut-1,0.0));
  std::vector<std::vector<double> > counts(sizeOut-1, std::vector<double>(sizeOut-1,0.0));

  PARALLEL_FOR1(inputWS)
  for (int i = 0; i < numSpec; i++)
  {
    PARALLEL_START_INTERUPT_REGION
    // Get the pixel relating to this spectrum
    IDetector_const_sptr det;
    try {
      det = inputWS->getDetector(i);
    } catch (Exception::NotFoundError&) {
      g_log.warning() << "Spectrum index " << i << " has no detector assigned to it - discarding" << std::endl;
      continue;
    }
    // If this detector is masked, skip to the next one
    if ( det->isMasked() ) continue;
    // If this detector is a monitor, skip to the next one
    if ( det->isMonitor() ) continue;

    // Get the current spectrum for both input workspaces
    const MantidVec& XIn = inputWS->readX(i);
    const MantidVec& YIn = inputWS->readY(i);
    //const MantidVec& EIn = inputWS->readE(i);


    // Calculate the Q values for the current spectrum
    V3D detPos = det->getPos()-samplePos;
    double phi = atan2(detPos.Y(),detPos.X());
    double a = cos(phi);
    double b = sin(phi);
    double sinTheta = sin( inputWS->detectorTwoTheta(det)/2.0 );
    double factor = fmp*sinTheta;

    // Loop over all xLength-1 detector channels
    // Note: xLength -1, because X is a histogram and has a number of boundaries
    // equal to the number of detector channels + 1.
    for ( int j = 0; j < xLength-1; j++)
    {
      double q = factor*2.0/(XIn[j]+XIn[j+1]);
      int iqx, iqy;
      // Bin assignment depends on whether we have log or linear bins
      if(binParams[1]>0.0)
      {
        iqx = (int)floor( (q*a-binParams[0])/ binParams[1] );
        iqy = (int)floor( (q*b-binParams[0])/ binParams[1] );
      } else {
        iqx = (int)floor(log(q*a/binParams[0])/log(1.0-binParams[1]));
        iqy = (int)floor(log(q*b/binParams[0])/log(1.0-binParams[1]));
      }

      if (iqx>=0 && iqy>=0 && iqx < sizeOut-1 && iqy < sizeOut-1)
      {
        counts[iqx][iqy] += YIn[j];
        normalization[iqx][iqy] += 1;
      }
    }

    progress.report("Computing Q(x,y)");
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // Normalize and write to file
  for ( int i = 0; i<sizeOut-1; i++ )
  {
    for ( int j = 0; j<sizeOut-1; j++ )
    {
      if ( normalization[i][j] > 0 )
      {
        counts[i][j] /= normalization[i][j];
        double qx = (qx_vector.access()[i]+qx_vector.access()[i+1])/2.0;
        double qy = (qy_vector.access()[j]+qy_vector.access()[j+1])/2.0;
        out_File << qx;
        out_File << "  " << qy;
        out_File << "  " << counts[i][j] << "\r\n";
      }
    }
  }
  out_File.close();

}

} // namespace Algorithms
} // namespace Mantid
