#include "MantidAlgorithms/Stitch1DMany.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/BoundedValidator.h"

#include <boost/make_shared.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
  namespace Algorithms
  {
    DECLARE_ALGORITHM(Stitch1DMany)

    /** Initialize the algorithm's properties.
     */
    void Stitch1DMany::init()
    {

      declareProperty(new ArrayProperty<std::string>("InputWorkspaces"),
          "Input Workspaces. List of histogram workspaces to stitch together.");

      declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "", Direction::Output),
          "Output stitched workspace.");

      declareProperty(new ArrayProperty<double>("Params", boost::make_shared<RebinParamsValidator>(true)),
          "Rebinning Parameters. See Rebin for format.");

      declareProperty(new ArrayProperty<double>("StartOverlaps"), "Start overlaps for stitched workspaces.");

      declareProperty(new ArrayProperty<double>("EndOverlaps"), "End overlaps for stitched workspaces.");

      declareProperty(new PropertyWithValue<bool>("ScaleRHSWorkspace", true, Direction::Input),
          "Scaling either with respect to workspace 1 or workspace 2");

      declareProperty(new PropertyWithValue<bool>("UseManualScaleFactor", false, Direction::Input),
          "True to use a provided value for the scale factor.");

      auto manualScaleFactorValidator = boost::make_shared<BoundedValidator<double> >();
      manualScaleFactorValidator->setLower(0);
      manualScaleFactorValidator->setExclusive(true);
      declareProperty(new PropertyWithValue<double>("ManualScaleFactor", 1.0, manualScaleFactorValidator, Direction::Input),
          "Provided value for the scale factor.");

      declareProperty(new ArrayProperty<double>("OutScaleFactors", Direction::Output),
          "The actual used values for the scaling factores at each stitch step.");
    }

    /** Load and validate the algorithm's properties.
     */
    std::map<std::string, std::string> Stitch1DMany::validateInputs()
    {
      std::map<std::string, std::string> errors;

      m_inputWorkspaces.clear();

      const std::vector<std::string> inputWorkspacesStr = this->getProperty("InputWorkspaces");
      if(inputWorkspacesStr.size() < 2)
        errors["InputWorkspaces"] = "At least 2 input workspaces required.";

      for(auto ws = inputWorkspacesStr.begin(); ws != inputWorkspacesStr.end(); ++ws)
      {
        if(AnalysisDataService::Instance().doesExist(*ws))
        {
          m_inputWorkspaces.push_back(AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(*ws));
        }
        else
        {
          errors["InputWorkspaces"] = *ws + " is not a valid workspace.";
          break;
        }
      }

      m_numWorkspaces = m_inputWorkspaces.size();

      m_startOverlaps = this->getProperty("StartOverlaps");
      m_endOverlaps = this->getProperty("EndOverlaps");

      if(m_startOverlaps.size() > 0 && m_startOverlaps.size() != m_numWorkspaces - 1)
        errors["StartOverlaps"] = "If given, StartOverlaps must have one fewer entries than the number of input workspaces.";

      if(m_startOverlaps.size() != m_endOverlaps.size())
        errors["EndOverlaps"] = "EndOverlaps must have the same number of entries as StartOverlaps.";


      m_scaleRHSWorkspace = this->getProperty("ScaleRHSWorkspace");
      m_useManualScaleFactor = this->getProperty("UseManualScaleFactor");
      m_manualScaleFactor = this->getProperty("ManualScaleFactor");
      m_params = this->getProperty("Params");

      if(m_params.size() < 1)
        errors["Params"] = "At least one parameter must be given.";

      if(!m_scaleRHSWorkspace)
      {
        //Flip these around for processing
        std::reverse(m_inputWorkspaces.begin(), m_inputWorkspaces.end());
        std::reverse(m_startOverlaps.begin(), m_startOverlaps.end());
        std::reverse(m_endOverlaps.begin(), m_endOverlaps.end());
      }

      m_scaleFactors.clear();
      m_outputWorkspace.reset();

      return errors;
    }

    /** Execute the algorithm.
     */
    void Stitch1DMany::exec()
    {
    }

  } // namespace Algorithms
} // namespace Mantid
