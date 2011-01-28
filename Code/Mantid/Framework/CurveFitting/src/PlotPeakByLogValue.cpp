//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <cmath>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <Poco/StringTokenizer.h>
#include <boost/lexical_cast.hpp>

#include "MantidCurveFitting/PlotPeakByLogValue.h"
#include "MantidCurveFitting/FuncMinimizerFactory.h"
#include "MantidCurveFitting/CostFunctionFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFitFunction.h"
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
      declareProperty("Input","",new MandatoryValidator<std::string>(),
        "List of input names");

      declareProperty("Spectrum", 1, "The spectrum number to be fitted in"
        " each workspace of the input list");
      declareProperty("WorkspaceIndex", 0, "The index of a spectrum to be fitted in"
        " each workspace of the input list");
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

      std::vector<std::string> fitOptions;
      fitOptions.push_back("Sequential");
      fitOptions.push_back("Individual");
      declareProperty("FitType","Sequential",new ListValidator(fitOptions),
        "Specifies the way the initial guesses are set for each fit. Individual means that "
        "all fits use the same initial values while in Sequential the result of the previous "
        "fit becomes initial guesses for the next one.");

      std::vector<std::string> minimizerOptions = FuncMinimizerFactory::Instance().getKeys();
      declareProperty("Minimizer","Levenberg-Marquardt",new ListValidator(minimizerOptions),
        "The minimizer method applied to do the fit, default is Levenberg-Marquardt", Direction::InOut);

      std::vector<std::string> costFuncOptions = CostFunctionFactory::Instance().getKeys();;
      declareProperty("CostFunction","Least squares",new ListValidator(costFuncOptions),
        "The cost function to be used for the fit, default is Least squares", Direction::InOut);
    }

    /** 
    *   Executes the algorithm
    */
    void PlotPeakByLogValue::exec()
    {

      // Create a list of the input workspace
      const std::vector<InputData> wsNames = makeNames();

      std::string fun = getPropertyValue("Function");
      //int wi = getProperty("WorkspaceIndex");
      std::string logName = getProperty("LogValue");
      bool sequential = getPropertyValue("FitType") == "Sequential";

      ITableWorkspace_sptr result = WorkspaceFactory::Instance().createTable("TableWorkspace");
      if (logName.empty())
      {
        result->addColumn("double","axis-1");
      }
      else
      {
        result->addColumn("double",logName);
      }
      // Create an instance of the fitting function to obtain the names of fitting parameters
      IFitFunction* ifun = FunctionFactory::Instance().createInitialized(fun);
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

      double dProg = 1./wsNames.size();
      double Prog = 0.;
      for(int i=0;i<static_cast<int>(wsNames.size());++i)
      {
        InputData data = getWorkspace(wsNames[i]);

        if (!data.ws)
        {
          g_log.warning() << "Cannot access workspace " << wsNames[i].name << '\n';
          continue;
        }

        if (data.i < 0 && data.indx.empty())
        {
          g_log.warning() << "Zero spectra selected for fitting in workspace " << wsNames[i].name << '\n';
          continue;
        }

        int j,jend;
        if (data.i >= 0)
        {
          j = data.i;
          jend = j + 1;
        }
        else
        {// no need to check data.indx.empty()
          j = data.indx.front();
          jend = data.indx.back() + 1;
        }

        dProg /= abs(jend - j);
        for(;j < jend;++j)
        {

          // Find the log value: it is either a log-file value or simply the workspace number
          double logValue;
          if (logName.empty())
          {
            API::Axis* axis = data.ws->getAxis(1);
            logValue = (*axis)(j);
          }
          else
          {
            Kernel::Property* prop = data.ws->run().getLogData(logName);
            if (!prop)
            {
              throw std::invalid_argument("Log value "+logName+" does not exist");
            }
            TimeSeriesProperty<double>* logp = 
              dynamic_cast<TimeSeriesProperty<double>*>(prop); 
            logValue = logp->lastValue();
          }

          std::string resFun = fun;
          std::vector<double> errors;

          try
          {
            // Fit the function
            API::IAlgorithm_sptr fit = createSubAlgorithm("Fit");
            fit->initialize();
            //fit->setProperty("InputWorkspace",data.ws);
            fit->setPropertyValue("InputWorkspace",data.ws->getName());
            fit->setProperty("WorkspaceIndex",j);
            fit->setPropertyValue("Function",fun);
            fit->setPropertyValue("StartX",getPropertyValue("StartX"));
            fit->setPropertyValue("EndX",getPropertyValue("EndX"));
            fit->setPropertyValue("Minimizer",getPropertyValue("Minimizer"));
            fit->setPropertyValue("CostFunction",getPropertyValue("CostFunction"));
            fit->execute();
            resFun = fit->getPropertyValue("Function");
            errors = fit->getProperty("Errors");
          }
          catch(...)
          {
            g_log.error("Error in Fit subalgorithm");
            throw;
          }

          if (sequential)
          {
            fun = resFun;
          }

          // Extract the fitted parameters and put them into the result table
          TableRow row = result->appendRow();
          row << logValue;
          ifun = FunctionFactory::Instance().createInitialized(resFun);
          for(int iPar=0;iPar<ifun->nParams();++iPar)
          {
            row << ifun->getParameter(iPar) << errors[iPar];
          }
          delete ifun;
          Prog += dProg;
          progress(Prog);
          interruption_point();
        } // for(;j < jend;++j)
      }
    }

    /** Get a workspace identified by an InputData structure. 
      * @param data :: InputData with name and either spec or i fields defined. 
      * @return InputData structure with the ws field set if everything was OK.
      */
    PlotPeakByLogValue::InputData PlotPeakByLogValue::getWorkspace(const InputData& data)
    {
      InputData out(data);
      if (API::AnalysisDataService::Instance().doesExist(data.name))
      {
        DataObjects::Workspace2D_sptr ws = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
          API::AnalysisDataService::Instance().retrieve(data.name));
        if (ws)
        {
          out.ws = ws;
        }
        else
        {
          return data;
        }
      }
      else
      {
        std::ifstream fil(data.name.c_str());
        if (!fil)
        {
          g_log.warning() << "File "<<data.name<<" does not exist\n";
          return data;
        }
        fil.close();
        std::string::size_type i = data.name.find_last_of('.');
        if (i == std::string::npos)
        {
          g_log.warning() << "Cannot open file "<<data.name<<"\n";
          return data;
        }
        std::string ext = data.name.substr(i);
        try
        {
          API::IAlgorithm_sptr load;
          if (ext.size() >= 2 && (ext == ".raw" || ext == ".RAW" || ext == ".add" || ext[1] == 's'))
          {// raw file
            load = createSubAlgorithm("LoadRaw");
          }
          else
          {// nexus file
            load = createSubAlgorithm("LoadNexus");
          }
          load->initialize();
          load->setPropertyValue("FileName",data.name);
          //if (data.spec >= 0)
          //{
          //  load->setProperty("SpectrumMin",data.spec);
          //  load->setProperty("SpectrumMax",data.spec);
          //}
          load->execute();
          if (load->isExecuted())
          {
            API::Workspace_sptr rws = load->getProperty("OutputWorkspace");
            if (rws)
            {
              DataObjects::Workspace2D_sptr ws = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(rws);
              if (ws) 
              {
                out.ws = ws;
              }
              else
              {
                API::WorkspaceGroup_sptr gws = boost::dynamic_pointer_cast<API::WorkspaceGroup>(rws);
                if (gws)
                {
                  std::vector<std::string> wsNames = gws->getNames();
                  std::string propName = "OUTPUTWORKSPACE_" + boost::lexical_cast<std::string>(data.period);
                  if (load->existsProperty(propName))
                  {
                    DataObjects::Workspace2D_sptr rws1 = load->getProperty(propName);
                    out.ws = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(rws1);
                  }
                }
              }
            }
          }
        }
        catch(std::exception& e)
        {
          g_log.error(e.what());
          return data;
        }
      }

      if (!out.ws) return data;

      API::Axis* axis = out.ws->getAxis(1);
      if (axis->isSpectra())
      {// spectra axis
        if (out.spec < 0)
        {
          if (out.i >= 0)
          {
            out.spec = axis->spectraNo(out.i);
          }
          else
          {// i < 0 && spec < 0 => use start and end
            for(int i=0;i<axis->length();++i)
            {
              double s = double(axis->spectraNo(i));
              if (s >= out.start && s <= out.end)
              {
                out.indx.push_back(i);
              }
            }
          }
        }
        else
        {
          for(int i=0;i<axis->length();++i)
          {
            int j = axis->spectraNo(i);
            if (j == out.spec)
            {
              out.i = i;
              break;
            }
          }
        }
        if (out.i < 0 && out.indx.empty())
        {
          return data;
        }
      }
      else
      {// numeric axis
        out.spec = -1;
        if (out.i >= 0)
        {
          out.indx.clear();
        }
        else
        {
          if (out.i < -1)
          {
            out.start = (*axis)(0);
            out.end = (*axis)(axis->length()-1);
          }
          for(int i=0;i<axis->length();++i)
          {
            double s = (*axis)(i);
            if (s >= out.start && s <= out.end)
            {
              out.indx.push_back(i);
            }
          }
        }
      }

      return out;
    }

    /// Create a list of input workspace names
    std::vector<PlotPeakByLogValue::InputData> PlotPeakByLogValue::makeNames()const
    {
      std::vector<InputData> nameList;
      std::string inputList = getPropertyValue("Input");
      int default_wi = getProperty("WorkspaceIndex");
      int default_spec = getProperty("Spectrum");
      double start = 0;
      double end = 0;

      typedef Poco::StringTokenizer tokenizer;
      tokenizer names(inputList, ";", tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);
      for (tokenizer::Iterator it = names.begin(); it != names.end(); ++it)
      {
        tokenizer params(*it, ",", tokenizer::TOK_TRIM);
        std::string name = params[0];
        int wi = default_wi;
        int spec = default_spec;
        if (params.count() > 1)
        {
          std::string index = params[1]; // spectrum or workspace index with a prefix
          if (index.size() > 2 && index.substr(0,2) == "sp")
          {// spectrum number
            spec = boost::lexical_cast<int>(index.substr(2));
            wi = -1; // undefined yet
          }
          else if (index.size() > 1 && index[0] == 'i')
          {// workspace index
            wi = boost::lexical_cast<int>(index.substr(1));
            spec = -1; // undefined yet
          }
          else if (index.size() > 0 && index[0] == 'v')
          {
            if (index.size() > 1)
            {// there is some text after 'v'
              tokenizer range(index.substr(1), ":", tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);
              if (range.count() < 1)
              {
                wi = -2; // means use the whole range
              }
              else if (range.count() == 1)
              {
                start = boost::lexical_cast<double>(range[0]);
                end = start;
                wi = -1;
                spec = -1;
              }
              else if (range.count() > 1)
              {
                start = boost::lexical_cast<double>(range[0]);
                end = boost::lexical_cast<double>(range[1]);
                if (start > end) std::swap(start,end);
                wi = -1;
                spec = -1;
              }
            }
            else
            {
              wi = -2;
            }
          }
          else
          {// error
            //throw std::invalid_argument("Malformed spectrum identifier ("+index+"). "
            //  "It must be either \"sp\" followed by a number for a spectrum number or"
            //  "\"i\" followed by a number for a workspace index.");
            wi = default_wi;
          }
          
        }
        int period = (params.count() > 2)? boost::lexical_cast<int>(params[2]) : 1;
        if (API::AnalysisDataService::Instance().doesExist(name))
        {
          API::Workspace_sptr ws = API::AnalysisDataService::Instance().retrieve(name);
          API::WorkspaceGroup_sptr wsg = boost::dynamic_pointer_cast<API::WorkspaceGroup>(ws);
          if (wsg)
          {
            std::vector<std::string> wsNames = wsg->getNames();
            for(std::vector<std::string>::iterator i=wsNames.begin();i!=wsNames.end();++i)
            {
             nameList.push_back(InputData(*i,wi,-1,period,start,end));
            }
            continue;
          }
        }
        nameList.push_back(InputData(name,wi,spec,period,start,end));
      }
      return nameList;
    }

  } // namespace CurveFitting
} // namespace Mantid




