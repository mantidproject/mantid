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
      declareProperty(new WorkspaceProperty<WorkspaceGroup>("InputWorkspace","",Direction::Input));
      declareProperty("WorkspaceIndex", 0, Direction::Input);
      declareProperty(new WorkspaceProperty<ITableWorkspace>("OutputWorkspace","",Direction::Output));
      declareProperty("Function","",new MandatoryValidator<std::string>());
      declareProperty("LogValue","");
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
      const std::vector<std::string>& wsNames = wsg->getNames();
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




