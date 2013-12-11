
#include "MantidAlgorithms/ReflectometryWorkflowBase.h"
#include <boost/assign/list_of.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;


namespace Mantid
{
namespace Algorithms
{

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ReflectometryWorkflowBase::ReflectometryWorkflowBase()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ReflectometryWorkflowBase::~ReflectometryWorkflowBase()
  {
  }
  


  /**
   * Convert the TOF workspace into a monitor workspace. Crops to the monitorIndex and applying flat background correction as part of the process.
   * @param toConvert : TOF wavlength to convert.
   * @param monitorIndex : Monitor index to crop to
   * @param backgroundMinMax : Min and Max Lambda range for Flat background correction.
   * @return The cropped and corrected monitor workspace.
   */
  MatrixWorkspace_sptr ReflectometryWorkflowBase::toLamMonitor(const MatrixWorkspace_sptr& toConvert, const int monitorIndex, const MinMax& backgroundMinMax)
  {
    // Convert Units.
    auto convertUnitsAlg = this->createChildAlgorithm("ConvertUnits");
    convertUnitsAlg->initialize();
    convertUnitsAlg->setProperty("InputWorkspace", toConvert);
    convertUnitsAlg->setProperty("Target", "Wavelength");
    convertUnitsAlg->setProperty("AlignBins", true);
    convertUnitsAlg->execute();

    // Crop the to the monitor index.
    MatrixWorkspace_sptr monitorWS = convertUnitsAlg->getProperty("OutputWorkspace");
    auto cropWorkspaceAlg = this->createChildAlgorithm("CropWorkspace");
    cropWorkspaceAlg->initialize();
    cropWorkspaceAlg->setProperty("InputWorkspace", monitorWS);
    cropWorkspaceAlg->setProperty("StartWorkspaceIndex", monitorIndex);
    cropWorkspaceAlg->setProperty("EndWorkspaceIndex", monitorIndex);
    cropWorkspaceAlg->execute();
    monitorWS = cropWorkspaceAlg->getProperty("OutputWorkspace");

    // Flat background correction
    auto correctMonitorsAlg = this->createChildAlgorithm("CalculateFlatBackground");
    correctMonitorsAlg->initialize();
    correctMonitorsAlg->setProperty("InputWorkspace", monitorWS);
    correctMonitorsAlg->setProperty("WorkspaceIndexList",
        boost::assign::list_of(0).convert_to_container<std::vector<int> >());
    correctMonitorsAlg->setProperty("StartX", backgroundMinMax.get<0>());
    correctMonitorsAlg->setProperty("EndX", backgroundMinMax.get<1>());
    correctMonitorsAlg->execute();
    monitorWS = correctMonitorsAlg->getProperty("OutputWorkspace");

    return monitorWS;
  }

  /**
   * Convert to a detector workspace in lambda.
   * @param detectorIndexRange : Workspace index ranges to keep
   * @param toConvert : TOF wavelength to convert.
   * @param wavelengthMinMax : Wavelength minmax to keep. Crop out the rest.
   * @param wavelengthStep : Wavelength step for rebinning
   * @return Detector workspace in wavelength
   */
  MatrixWorkspace_sptr ReflectometryWorkflowBase::toLamDetector(
      const WorkspaceIndexList& detectorIndexRange, const MatrixWorkspace_sptr& toConvert,
      const MinMax& wavelengthMinMax, const double& wavelengthStep)
  {
    // Detector Workspace Processing
    MatrixWorkspace_sptr detectorWS;

    // Loop over pairs of detector index ranges. Peform the cropping and then conjoin the results into a single workspace.
    for (size_t i = 0; i < detectorIndexRange.size(); i += 2)
    {
      auto cropWorkspaceAlg = this->createChildAlgorithm("CropWorkspace");
      cropWorkspaceAlg->initialize();
      cropWorkspaceAlg->setProperty("InputWorkspace", toConvert);
      cropWorkspaceAlg->setProperty("StartWorkspaceIndex", detectorIndexRange[i]);
      cropWorkspaceAlg->setProperty("EndWorkspaceIndex", detectorIndexRange[i + 1]);
      cropWorkspaceAlg->execute();
      MatrixWorkspace_sptr subRange = cropWorkspaceAlg->getProperty("OutputWorkspace");
      if (i == 0)
      {
        detectorWS = subRange;
      }
      else
      {
        auto conjoinWorkspaceAlg = this->createChildAlgorithm("ConjoinWorkspaces");
        conjoinWorkspaceAlg->initialize();
        conjoinWorkspaceAlg->setProperty("InputWorkspace1", detectorWS);
        conjoinWorkspaceAlg->setProperty("InputWorkspace2", subRange);
        conjoinWorkspaceAlg->execute();
        detectorWS = conjoinWorkspaceAlg->getProperty("InputWorkspace1");
      }
    }
    // Now convert units. Do this after the conjoining step otherwise the x bins will not match up.
    auto convertUnitsAlg = this->createChildAlgorithm("ConvertUnits");
    convertUnitsAlg->initialize();
    convertUnitsAlg->setProperty("InputWorkspace", detectorWS);
    convertUnitsAlg->setProperty("Target", "Wavelength");
    convertUnitsAlg->setProperty("AlignBins", true);
    convertUnitsAlg->execute();
    detectorWS = convertUnitsAlg->getProperty("OutputWorkspace");

    // Crop out the lambda x-ranges now that the workspace is in wavelength.
    auto cropWorkspaceAlg = this->createChildAlgorithm("CropWorkspace");
    cropWorkspaceAlg->initialize();
    cropWorkspaceAlg->setProperty("InputWorkspace", detectorWS);
    cropWorkspaceAlg->setProperty("XMin", wavelengthMinMax.get<0>());
    cropWorkspaceAlg->setProperty("XMax", wavelengthMinMax.get<1>());
    cropWorkspaceAlg->execute();
    detectorWS = cropWorkspaceAlg->getProperty("OutputWorkspace");

    auto rebinWorkspaceAlg = this->createChildAlgorithm("Rebin");
    rebinWorkspaceAlg->initialize();
    std::vector<double> params = boost::assign::list_of(wavelengthStep);
    rebinWorkspaceAlg->setProperty("Params", params);
    rebinWorkspaceAlg->setProperty("InputWorkspace", detectorWS);
    rebinWorkspaceAlg->execute();
    detectorWS = rebinWorkspaceAlg->getProperty("OutputWorkspace");

    return detectorWS;
  }

  /**
   * Convert From a TOF workspace into a detector and monitor workspace both in Lambda.
   * @param toConvert: TOF workspace to convert
   * @param detectorIndexRange : Detector index ranges
   * @param monitorIndex : Monitor index
   * @param wavelengthMinMax : Wavelength min max for detector workspace
   * @param backgroundMinMax : Wavelength min max for flat background correction of monitor workspace
   * @param wavelengthStep : Wavlength step size for rebinning.
   * @return Tuple of detector and monitor workspaces
   */
  ReflectometryWorkflowBase::DetectorMonitorWorkspacePair ReflectometryWorkflowBase::toLam(MatrixWorkspace_sptr toConvert,
      const WorkspaceIndexList& detectorIndexRange, const int monitorIndex,
      const MinMax& wavelengthMinMax, const MinMax& backgroundMinMax, const double& wavelengthStep)
  {
    // Detector Workspace Processing
    MatrixWorkspace_sptr detectorWS = toLamDetector(detectorIndexRange, toConvert, wavelengthMinMax, wavelengthStep);

    // Monitor Workspace Processing
    MatrixWorkspace_sptr monitorWS = toLamMonitor(toConvert, monitorIndex, backgroundMinMax);

    // Rebin the Monitor Workspace to match the Detector Workspace.
    auto rebinToWorkspaceAlg = this->createChildAlgorithm("RebinToWorkspace");
    rebinToWorkspaceAlg->initialize();
    rebinToWorkspaceAlg->setProperty("WorkspaceToRebin", monitorWS);
    rebinToWorkspaceAlg->setProperty("WorkspaceToMatch", detectorWS);
    rebinToWorkspaceAlg->execute();
    monitorWS = rebinToWorkspaceAlg->getProperty("OutputWorkspace");

    return DetectorMonitorWorkspacePair( detectorWS, monitorWS );
  }





} // namespace Algorithms
} // namespace Mantid
