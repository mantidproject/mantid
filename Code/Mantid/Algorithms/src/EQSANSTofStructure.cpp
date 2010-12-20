//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/EQSANSTofStructure.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/TimeSeriesProperty.h"

using namespace Mantid::Kernel;

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(EQSANSTofStructure)

using namespace Kernel;
using namespace API;
using namespace Geometry;

void EQSANSTofStructure::init()
{
  CompositeValidator<> *wsValidator = new CompositeValidator<>;
  wsValidator->add(new WorkspaceUnitValidator<>("TOF"));
  wsValidator->add(new HistogramValidator<>);
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input,wsValidator),
      "Workspace to apply the TOF correction to");
  declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
      "Workspace to store the corrected data in");
  declareProperty("TOFOffset", 0.0, "TOF offset" );
}

void EQSANSTofStructure::exec()
{
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  const double frame_tof0 = getProperty("TOFOffset");

  // Now create the output workspace
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if ( outputWS != inputWS )
  {
    outputWS = WorkspaceFactory::Instance().create(inputWS);
    setProperty("OutputWorkspace",outputWS);
  }

  const int numHists = inputWS->getNumberHistograms();

  Progress progress(this,0.0,1.0,numHists);

  // Calculate the frame width
  const double frequency = dynamic_cast<TimeSeriesProperty<double>*>(inputWS->run().getLogData("frequency"))->getStatistics().mean;
  double tof_frame_width = 1.0e6/frequency;
  double tmp_frame_width = tof_frame_width;
  //double tmp_frame_width=frame_skipping_option>=FRAME_SKIPPING_YES ? tof_frame_width * 2 : tof_frame_width;

  double frame_offset=0.0;
  if (frame_tof0 >= tmp_frame_width) frame_offset = tmp_frame_width * ( (int)( frame_tof0/tmp_frame_width ) );

  // Find the new binning first
  //dataX = mantid.getMatrixWorkspace(input_ws).readX(0).copy()
  const MantidVec& XIn = inputWS->readX(0);
  const int nTOF = XIn.size();

  // Loop through each bin
  int cutoff = 0;
  double threshold = frame_tof0-frame_offset;
  for (int i=0; i<nTOF; i++)
  {
      if (XIn[i] < threshold) cutoff = i;
  }

  g_log.information() << "Cutoff " << cutoff << " at " << threshold << std::endl;
  g_log.information() << "Frame offset " << frame_offset << std::endl;
  g_log.information() << "Tmp width " << tmp_frame_width << std::endl;

  // Since we are swapping the low-TOF and high-TOF regions around the cutoff value,
  // there is the potential for having an overlap between the two regions. We exclude
  // the region beyond a single frame by considering only the first 1/60 sec of the
  // TOF histogram. (Bins 1 to 1666, as opposed to 1 to 2000)
  int tof_bin_range = (int)(100000.0/frequency);
  //int tof_bin_range = nTOF;

  g_log.information() << "Low TOFs: old = [" << (cutoff+1) << ", " << (tof_bin_range-2) << "]  ->  new = [0, " << (tof_bin_range-3-cutoff) << "]" << std::endl;
  g_log.information() << "High bin boundary of the Low TOFs: old = " << tof_bin_range-1 << "; new = " << (tof_bin_range-2-cutoff) << std::endl;
  g_log.information() << "High TOFs: old = [0, " << (cutoff-1) << "]  ->  new = [" << (tof_bin_range-1-cutoff) << ", " << (tof_bin_range-2) << "]" << std::endl;
  g_log.information() << "Overlap: new = [" << (tof_bin_range-1) << ", " << (nTOF-2) << "]" << std::endl;

  // Loop through the spectra and apply correction
  for (int ispec = 0; ispec < numHists; ispec++)
  {
    // Keep a copy of the input data since we may end up overwriting it
    // if the input workspace is equal to the output workspace.
    // This is necessary since we are shuffling around the TOF bins.
    MantidVec YCopy = MantidVec(inputWS->readY(ispec));
    MantidVec& YIn = YCopy;
    MantidVec ECopy = MantidVec(inputWS->readE(ispec));
    MantidVec& EIn = ECopy;

    MantidVec& XOut = outputWS->dataX(ispec);
    MantidVec& YOut = outputWS->dataY(ispec);
    MantidVec& EOut = outputWS->dataE(ispec);

    // Move up the low TOFs
    for (int i=0; i<cutoff; i++)
    {
      XOut[i+tof_bin_range-1-cutoff] = XIn[i] + frame_offset + tmp_frame_width;
      YOut[i+tof_bin_range-1-cutoff] = YIn[i];
      EOut[i+tof_bin_range-1-cutoff] = EIn[i];
    }

    // Get rid of extra bins
    for (int i=tof_bin_range-1; i<nTOF-1; i++)
    {
      XOut[i] = XOut[i-1]+10.0;
      YOut[i] = 0.0;
      EOut[i] = 0.0;
    }
    XOut[nTOF-1] = XOut[nTOF-2]+10.0;

    // Move down the high TOFs
    for (int i=cutoff+1; i<tof_bin_range-1; i++)
    {
      XOut[i-cutoff-1] = XIn[i] + frame_offset;
      YOut[i-cutoff-1] = YIn[i];
      EOut[i-cutoff-1] = EIn[i];
    }
    // Don't forget the low boundary
    XOut[tof_bin_range-2-cutoff] = XIn[tof_bin_range-1] + frame_offset;

    // Zero out the cutoff bin, which no longer makes sense
    YOut[tof_bin_range-2-cutoff] = 0.0;
    EOut[tof_bin_range-2-cutoff] = 0.0;

    progress.report();
  }
}

} // namespace Algorithms
} // namespace Mantid

