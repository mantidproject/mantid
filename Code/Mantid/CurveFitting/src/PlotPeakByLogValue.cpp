//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <cmath>
#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>

#include "MantidCurveFitting/PlotPeakByLogValue.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/TableRow.h"

namespace Mantid
{
  namespace CurveFitting
  {

    using namespace Kernel;
    using namespace API;

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(PlotPeakByLogValue)

    /** Initialisation method. Declares properties to be used in algorithm.
    *
    */
    void PlotPeakByLogValue::init()
    {
      declareProperty(new WorkspaceProperty<WorkspaceGroup>("InputWorkspace","",Direction::Input),
        "Name of the input workspace group");
      declareProperty("WorkspaceIndex", 0, "The index of a spectrum to be fitted in"
        " each workspace of the input workspace group");
      declareProperty(new WorkspaceProperty<ITableWorkspace>("OutputWorkspace","",Direction::Output));
      declareProperty("Function","",new MandatoryValidator<std::string>(),
        "The fitting function, common for all workspaces");
      declareProperty("LogValue","","Name of the log value to plot the parameters against");
      declareProperty("StartX", EMPTY_DBL(),
        "A value of x in, or on the low x boundary of, the first bin to include in\n"
        "the fit (default lowest value of x)" );
      declareProperty("EndX", EMPTY_DBL(),
        "A value in, or on the high x boundary of, the last bin the fitting range\n"
        "(default the highest value of x)" );
    }

    /** 
    *   Executes the algorithm
    */
    void PlotPeakByLogValue::exec()
    {
      std::string wsName = getPropertyValue("InputWorkspace");
      API::WorkspaceGroup_sptr wsg = boost::dynamic_pointer_cast<API::WorkspaceGroup>(
        API::AnalysisDataService::Instance().retrieve(wsName));

      std::string fun = getPropertyValue("Function");
      ITableWorkspace_sptr result = WorkspaceFactory::Instance().createTable("TableWorkspace");
      result->addColumn("double","LogValue");
      // Create an instance of the fitting function to obtain the names of fitting parameters
      IFunction* ifun = FunctionFactory::Instance().createInitialized(fun);
      if (!ifun)
      {
        throw std::invalid_argument("Fitting function failed to initialize");
      }
      for(int iPar=0;iPar<ifun->nParams();++iPar)
      {
        result->addColumn("double",ifun->parameterName(iPar));
        result->addColumn("double",ifun->parameterName(iPar)+"_Err");
      }
      delete ifun;
      setProperty("OutputWorkspace",result);

      int wi = getProperty("WorkspaceIndex");
      std::string logName = getProperty("LogValue");
      const std::vector<std::string> wsNames = wsg->getNames();
      double dProg = 1./wsNames.size();
      for(int i=0;i<wsNames.size();++i)
      {
        if (wsNames[i] == wsName) continue;
        DataObjects::Workspace2D_sptr ws = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
          API::AnalysisDataService::Instance().retrieve(wsNames[i]));

        // Find the log value: it is either a log-file value or simply the workspace number
        double logValue;
        if (!logName.empty())
        {
          Kernel::Property* prop = ws->sample().getLogData(logName);
          if (!prop)
          {
            throw std::invalid_argument("Log value "+logName+" does not exist");
          }
          TimeSeriesProperty<double>* logp = 
            dynamic_cast<TimeSeriesProperty<double>*>(prop); 
          logValue = logp->lastValue();
        }
        else
        {
          logValue = i;
        }

        // Fit the function
        API::IAlgorithm_sptr fit = createSubAlgorithm("Fit",i*dProg,(i+1)*dProg);
        fit->initialize();
        fit->setProperty("InputWorkspace",ws);
        fit->setProperty("WorkspaceIndex",wi);
        fit->setPropertyValue("Function",fun);
        fit->setPropertyValue("StartX",getPropertyValue("StartX"));
        fit->setPropertyValue("EndX",getPropertyValue("EndX"));
        fit->execute();

        // Extract the fitted parameters and put them into the result table
        TableRow row = result->appendRow();
        row << logValue;
        ifun = FunctionFactory::Instance().createInitialized(fit->getPropertyValue("Function"));
        std::vector<double> errors = fit->getProperty("Errors");
        for(int iPar=0;iPar<ifun->nParams();++iPar)
        {
          row << ifun->getParameter(iPar) << errors[iPar];
        }
        delete ifun;
      }
    }

  } // namespace CurveFitting
} // namespace Mantid




