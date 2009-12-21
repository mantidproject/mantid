//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <math.h>
#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>

#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/FileProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAlgorithms/PlotAsymmetryByLogValue.h"
#include "MantidAPI/Progress.h"

namespace Mantid
{
  namespace Algorithms
  {

    using namespace Kernel;
    using namespace API;

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(PlotAsymmetryByLogValue)

    /** Initialisation method. Declares properties to be used in algorithm.
    *
    */
    void PlotAsymmetryByLogValue::init()
    {
      std::vector<std::string> exts(1, "nxs");
      declareProperty(new FileProperty("FirstRun","", FileProperty::Load, exts));
      declareProperty(new FileProperty("LastRun","", FileProperty::Load, exts));
      declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>("OutputWorkspace","",Direction::Output));
      declareProperty("LogValue","",new MandatoryValidator<std::string>());
      declareProperty("Red", 1, Direction::Input);
      declareProperty("Green", EMPTY_INT(), Direction::Input);

      std::vector<std::string> options;
      options.push_back("Integral");
      options.push_back("Differential");
      declareProperty("Type","Integral",new ListValidator(options));

      declareProperty("TimeMin",EMPTY_DBL(),"Starting X value for integration");
      declareProperty("TimeMax",EMPTY_DBL(),"Ending X value for integration");

       declareProperty(new ArrayProperty<int> ("ForwardSpectra"),
         "The spectra numbers of the forward group (default 0)");
       declareProperty(new ArrayProperty<int> ("BackwardSpectra"),
         "The spectra numbers of the backward group (default 1)");
      //declareProperty("Alpha",1.0,Direction::Input);
    }

    /** 
    *   Executes the algorithm
    */
    void PlotAsymmetryByLogValue::exec()
    {
      m_forward_list = getProperty("ForwardSpectra");		
	    m_backward_list = getProperty("BackwardSpectra");   
      m_autogroup = ( m_forward_list.size() == 0 && m_backward_list.size() == 0);

      //double alpha = getProperty("Alpha");

      std::string logName = getProperty("LogValue");

      int red = getProperty("Red");
      int green = getProperty("Green");

      std::string stype = getProperty("Type");
      m_int = stype == "Integral";

      std::string firstFN = getProperty("FirstRun");
      std::string lastFN = getProperty("LastRun");

      std::string ext = firstFN.substr(firstFN.find_last_of("."));

      firstFN.erase(firstFN.size()-4);
      lastFN.erase(lastFN.size()-4);

      std::string fnBase = firstFN;
      size_t i = fnBase.size()-1;
      while(isdigit(fnBase[i]))  i--;
      if (i == fnBase.size()-1) 
      {
        g_log.error("File name must end with a number.");
        throw Exception::FileError("File name must end with a number.",firstFN);
      }
      fnBase.erase(i+1);

      firstFN.erase(0,fnBase.size());
      lastFN.erase(0,fnBase.size());

      size_t is = atoi(firstFN.c_str());  // starting run number
      size_t ie = atoi(lastFN.c_str());   // last run number
      size_t w  = firstFN.size();

      // The number of runs
      int npoints = ie - is + 1;

      // Create the 2D workspace for the output
      int nplots = green != EMPTY_INT() ? 4 : 1;
      DataObjects::Workspace2D_sptr outWS = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
        (WorkspaceFactory::Instance().create("Workspace2D",
        nplots,          //  the number of plots
        npoints,    //  the number of data points on a plot
        npoints     //  it's not a histogram
        ));

      Progress progress(this,0,1,is,ie+1,1);
      for(size_t i=is;i<=ie;i++)
      {
        std::ostringstream fn,fnn;
        fnn << std::setw(w) << std::setfill('0') << i ;
        fn << fnBase << fnn.str() << ext;

        // Load a muon nexus file with auto_group set to true
        IAlgorithm_sptr loadNexus = createSubAlgorithm("LoadMuonNexus");
        loadNexus->setPropertyValue("Filename", fn.str());
        loadNexus->setPropertyValue("OutputWorkspace","tmp"+fnn.str());
        if (m_autogroup)
          loadNexus->setPropertyValue("AutoGroup","1");
        loadNexus->execute();

        std::string wsProp = "OutputWorkspace";
        //std::string wsName = "tmp"+fnn.str();

        DataObjects::Workspace2D_sptr ws_red;
        DataObjects::Workspace2D_sptr ws_green;

        // Run through the periods of the loaded file and do calculatons on the selected ones
        Workspace_sptr tmp = loadNexus->getProperty(wsProp);
        WorkspaceGroup_sptr wsGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(tmp);
        if (!wsGroup)
        {
          ws_red = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(tmp);
          TimeSeriesProperty<double>* logp = 
            //dynamic_cast<TimeSeriesProperty<double>*>(ws_red->getSample()->getLogData(logName));
			dynamic_cast<TimeSeriesProperty<double>*>(ws_red->sample().getLogData(logName));
          double Y,E; 
          calcIntAsymmetry(ws_red,Y,E);
          outWS->dataY(0)[i-is] = Y;
          outWS->dataX(0)[i-is] = logp->lastValue();
          outWS->dataE(0)[i-is] = E;
        }
        else
        {

          for( int period = 1; period < wsGroup->getNumberOfEntries(); ++period )
          {
            std::stringstream suffix;
            suffix << period;
            wsProp = "OutputWorkspace_" + suffix.str();// form the property name for higher periods
            //wsName = "tmp"+fnn.str() + "_" + suffix.str();
            // Do only one period
            if (green == EMPTY_INT() && period == red)
            {
              ws_red = loadNexus->getProperty(wsProp);
              //AnalysisDataService::Instance().add(wsName,ws);
              TimeSeriesProperty<double>* logp = 
                //dynamic_cast<TimeSeriesProperty<double>*>(ws_red->getSample()->getLogData(logName));
				dynamic_cast<TimeSeriesProperty<double>*>(ws_red->sample().getLogData(logName));
              double Y,E; 
              calcIntAsymmetry(ws_red,Y,E);
              outWS->dataY(0)[i-is] = Y;
              outWS->dataX(0)[i-is] = logp->lastValue();
              outWS->dataE(0)[i-is] = E;
            }
            else // red & green
            {
              if (period == red)
              {
                ws_red = loadNexus->getProperty(wsProp);
              }
              if (period == green)
              {
                ws_green = loadNexus->getProperty(wsProp);
              }
            }

          }
          // red & green claculation
          if (green != EMPTY_INT())
          {
            if (!ws_red || !ws_green)
              throw std::invalid_argument("Red or green period is out of range");
            TimeSeriesProperty<double>* logp = 
              //dynamic_cast<TimeSeriesProperty<double>*>(ws_red->getSample()->getLogData(logName));
			  dynamic_cast<TimeSeriesProperty<double>*>(ws_red->sample().getLogData(logName));
            double Y,E; 
            calcIntAsymmetry(ws_red,ws_green,Y,E);
            outWS->dataY(0)[i-is] = Y;
            outWS->dataX(0)[i-is] = logp->lastValue();
            outWS->dataE(0)[i-is] = E;

            double Y1,E1;
            calcIntAsymmetry(ws_red,Y,E);
            calcIntAsymmetry(ws_green,Y1,E1);
            outWS->dataY(1)[i-is] = Y;
            outWS->dataX(1)[i-is] = logp->lastValue();
            outWS->dataE(1)[i-is] = E;

            outWS->dataY(2)[i-is] = Y1;
            outWS->dataX(2)[i-is] = logp->lastValue();
            outWS->dataE(2)[i-is] = E1;

            outWS->dataY(3)[i-is] = Y + Y1;
            outWS->dataX(3)[i-is] = logp->lastValue();
            outWS->dataE(3)[i-is] = E + E1;
          }
          else
            if (!ws_red)
              throw std::invalid_argument("Red period is out of range");

        }
        progress.report();
      }

      outWS->getAxis(0)->title() = logName;
      outWS->setYUnit("Asymmetry");
      
      // Assign the result to the output workspace property
      setProperty("OutputWorkspace", boost::dynamic_pointer_cast<DataObjects::Workspace2D>(outWS));

    }

    /**  Calculate the integral asymmetry for a workspace. 
    *   The calculation is done by MuonAsymmetryCalc and SimpleIntegration algorithms.
    *   @param ws The workspace
    *   @param Y Reference to a variable receiving the value of asymmetry
    *   @param E Reference to a variable receiving the value of the error
    */
    void PlotAsymmetryByLogValue::calcIntAsymmetry(boost::shared_ptr<DataObjects::Workspace2D> ws, double& Y, double& E)
    {
      Property* startXprop = getProperty("TimeMin");
      Property* endXprop = getProperty("TimeMax");
      bool setX = !startXprop->isDefault() && !endXprop->isDefault();
      double startX(0.0),endX(0.0);
      if (setX)
      {
        startX = getProperty("TimeMin");
        endX = getProperty("TimeMax");
      }
      if (!m_int)
      {   //  "Differential asymmetry"
        IAlgorithm_sptr asym = createSubAlgorithm("AsymmetryCalc");
        asym->initialize();
        asym->setProperty("InputWorkspace",ws);
        asym->setPropertyValue("OutputWorkspace","tmp");
        if ( !m_autogroup )
        {
          asym->setProperty("ForwardSpectra",m_forward_list);
          asym->setProperty("BackwardSpectra",m_backward_list);
        }
        asym->execute();
        DataObjects::Workspace2D_sptr asymWS = asym->getProperty("OutputWorkspace");

        IAlgorithm_sptr integr = createSubAlgorithm("Integration");
        integr->setProperty("InputWorkspace",asymWS);
        integr->setPropertyValue("OutputWorkspace","tmp");
        if (setX)
        {
          integr->setProperty("RangeLower",startX);
          integr->setProperty("RangeUpper",endX);
        }
        integr->execute();
        DataObjects::Workspace2D_sptr out = integr->getProperty("OutputWorkspace");

        Y = out->readY(0)[0];
        E = out->readE(0)[0];
      }
      else
      {   
        //  "Integral asymmetry"
        IAlgorithm_sptr integr = createSubAlgorithm("Integration");
        integr->setProperty("InputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(ws));
        integr->setPropertyValue("OutputWorkspace","tmp");
        if (setX)
        {
          integr->setProperty("RangeLower",startX);
          integr->setProperty("RangeUpper",endX);
        }
        integr->execute();
        API::MatrixWorkspace_sptr intWS = integr->getProperty("OutputWorkspace");

        IAlgorithm_sptr asym = createSubAlgorithm("AsymmetryCalc");
        asym->initialize();
        asym->setProperty("InputWorkspace",boost::dynamic_pointer_cast<DataObjects::Workspace2D>(intWS));
        asym->setPropertyValue("OutputWorkspace","tmp");
        if ( !m_autogroup )
        {
          asym->setProperty("ForwardSpectra",m_forward_list);
          asym->setProperty("BackwardSpectra",m_backward_list);
        }
        asym->execute();
        DataObjects::Workspace2D_sptr out = asym->getProperty("OutputWorkspace");

        Y = out->readY(0)[0];
        E = out->readE(0)[0];

      }


    }

    /**  Calculate the integral asymmetry for a workspace (red & green). 
    *   The calculation is done by MuonAsymmetryCalc and SimpleIntegration algorithms.
    *   @param ws_red The red workspace
    *   @param ws_green The green workspace
    *   @param Y Reference to a variable receiving the value of asymmetry
    *   @param E Reference to a variable receiving the value of the error
    */
    void PlotAsymmetryByLogValue::calcIntAsymmetry(boost::shared_ptr<DataObjects::Workspace2D> ws_red, 
      boost::shared_ptr<DataObjects::Workspace2D> ws_green,double& Y, double& E)
    {
      if ( !m_autogroup )
      {
        groupDetectors(ws_red,m_backward_list);
        groupDetectors(ws_red,m_forward_list);
        groupDetectors(ws_green,m_backward_list);
        groupDetectors(ws_green,m_forward_list);
      }

      Property* startXprop = getProperty("TimeMin");
      Property* endXprop = getProperty("TimeMax");
      bool setX = !startXprop->isDefault() && !endXprop->isDefault();
      double startX(0.0),endX(0.0);
      if (setX)
      {
        startX = getProperty("TimeMin");
        endX = getProperty("TimeMax");
      }
      if (!m_int)
      {   //  "Differential asymmetry"

        DataObjects::Workspace2D_sptr tmpWS = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
          API::WorkspaceFactory::Instance().create(ws_red,1,ws_red->readX(0).size(),ws_red->readY(0).size())
          );

        for(size_t i=0;i<tmpWS->dataY(0).size();i++)
        {
          double FNORM = ws_green->readY(0)[i] + ws_red->readY(0)[i];
          FNORM = FNORM != 0.0 ? 1.0 / FNORM : 1.0;
          double BNORM = ws_green->readY(1)[i] + ws_red->readY(1)[i];
          BNORM = BNORM != 0.0 ? 1.0 / BNORM : 1.0;
          double ZF = ( ws_green->readY(0)[i] - ws_red->readY(0)[i] ) * FNORM;
          double ZB = ( ws_green->readY(1)[i] - ws_red->readY(1)[i] ) * BNORM;
          tmpWS->dataY(0)[i] = ZB - ZF;
          tmpWS->dataE(0)[i] = (1.0+ZF*ZF)*FNORM+(1.0+ZB*ZB)*BNORM;
        }

        IAlgorithm_sptr integr = createSubAlgorithm("Integration");
        integr->setProperty("InputWorkspace",tmpWS);
        integr->setPropertyValue("OutputWorkspace","tmp");
        if (setX)
        {
          integr->setProperty("RangeLower",startX);
          integr->setProperty("RangeUpper",endX);
        }
        integr->execute();
        MatrixWorkspace_sptr out = integr->getProperty("OutputWorkspace");

        Y = out->readY(0)[0] / tmpWS->dataY(0).size();
        E = out->readE(0)[0] / tmpWS->dataY(0).size();
      }
      else
      {   
        //  "Integral asymmetry"
        IAlgorithm_sptr integr = createSubAlgorithm("Integration");
        integr->setProperty("InputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(ws_red));
        integr->setPropertyValue("OutputWorkspace","tmp");
        if (setX)
        {
          integr->setProperty("RangeLower",startX);
          integr->setProperty("RangeUpper",endX);
        }
        integr->execute();
        API::MatrixWorkspace_sptr intWS_red = integr->getProperty("OutputWorkspace");

        integr = createSubAlgorithm("Integration");
        integr->setProperty("InputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(ws_green));
        integr->setPropertyValue("OutputWorkspace","tmp");
        if (setX)
        {
          integr->setProperty("RangeLower",startX);
          integr->setProperty("RangeUpper",endX);
        }
        integr->execute();
        API::MatrixWorkspace_sptr intWS_green = integr->getProperty("OutputWorkspace");

        double YIF = ( intWS_green->readY(0)[0] - intWS_red->readY(0)[0] ) / ( intWS_green->readY(0)[0] + intWS_red->readY(0)[0] );
        double YIB = ( intWS_green->readY(1)[0] - intWS_red->readY(1)[0] ) / ( intWS_green->readY(1)[0] + intWS_red->readY(1)[0] );

        Y = YIB - YIF;

        double VARIF = (1.0 + YIF*YIF) / ( intWS_green->readY(0)[0] + intWS_red->readY(0)[0] );
        double VARIB = (1.0 + YIB*YIB) / ( intWS_green->readY(1)[0] + intWS_red->readY(1)[0] );

        E = sqrt( VARIF + VARIB );
      }

    }

    /**  Group detectors in the workspace.
     *  @param ws A local workspace
     *  @param spectraList A list of spectra to group.
     */
    void PlotAsymmetryByLogValue::groupDetectors(boost::shared_ptr<DataObjects::Workspace2D> ws,const std::vector<int>& spectraList)
    {
      API::IAlgorithm_sptr group = createSubAlgorithm("GroupDetectors");
      group->setProperty("InputWorkspace",ws);
      group->setProperty("SpectraList",spectraList);
      group->setProperty("KeepUngroupedSpectra",true);
      group->execute();
      ws = group->getProperty("OutputWorkspace");
    }

  } // namespace Algorithm
} // namespace Mantid




