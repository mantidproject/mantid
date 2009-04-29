//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <math.h>
#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAlgorithms/PlotAsymmetryByLogValue.h"

namespace Mantid
{
  namespace Algorithms
  {

    using namespace Kernel;
    using namespace API;

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(PlotAsymmetryByLogValue)

    // Get a reference to the logger
    Logger& PlotAsymmetryByLogValue::g_log = Logger::get("PlotAsymmetryByLogValue");

    /** Initialisation method. Declares properties to be used in algorithm.
    *
    */
    void PlotAsymmetryByLogValue::init()
    {
        std::vector<std::string> exts;
        exts.push_back("NXS");
        exts.push_back("nxs");
        declareProperty("FirstRun","",new FileValidator(exts));
        declareProperty("LastRun","",new FileValidator(exts));
        declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>("OutputWorkspace","",Direction::Output));
        declareProperty("LogValue","",new MandatoryValidator<std::string>());
        declareProperty("Period", 1, Direction::Input);
        declareProperty("Int",true);
	   
        //BoundedValidator<int> *zeroOrGreater = new BoundedValidator<int>();
        //zeroOrGreater->setLower(0);
        //declareProperty("ForwardSpectra", 0, Direction::Input);
        //declareProperty("BackwardSpectra", 1, Direction::Input);
        //declareProperty("Alpha",1.0,Direction::Input);
    }

    /** 
     *   Executes the algorithm
     */
    void PlotAsymmetryByLogValue::exec()
    {
	    //int forward = getProperty("ForwardSpectra");		
	    //int backward = getProperty("BackwardSpectra");   
	    //double alpha = getProperty("Alpha");
        
        int Period = getProperty("Period");
        m_int = getProperty("Int");

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
        DataObjects::Workspace2D_sptr outWS = boost::dynamic_pointer_cast<DataObjects::Workspace2D>
                 (WorkspaceFactory::Instance().create("Workspace2D",
                   1,          //  the number of plots
                   npoints,    //  the number of data points on a plot
                   npoints     //  it's not a histogram
                 ));
        //localWorkspace->setTitle(title);
        //localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");

        for(size_t i=is;i<=ie;i++)
        {
            std::ostringstream fn,fnn;
            fnn << std::setw(w) << std::setfill('0') << i ;
            fn << fnBase << fnn.str() << ext;

            // Load a muon nexus file with auto_group set to true
            IAlgorithm_sptr loadNexus = createSubAlgorithm("LoadMuonNexus");
            loadNexus->setPropertyValue("Filename", fn.str());
            loadNexus->setPropertyValue("OutputWorkspace","tmp"+fnn.str());
            loadNexus->setPropertyValue("auto_group","1");
            loadNexus->execute();

            std::string wsProp = "OutputWorkspace";
            //std::string wsName = "tmp"+fnn.str();

            std::string logName = getProperty("LogValue");

            // Run through the periods of the loaded file and do calculatons on the selected ones
            int period = 1;
            while( loadNexus->existsProperty(wsProp) )
            {
                if (period == Period)
                {
                    DataObjects::Workspace2D_sptr ws = loadNexus->getProperty(wsProp);
                    //AnalysisDataService::Instance().add(wsName,ws);
                    TimeSeriesProperty<double>* logp = dynamic_cast<TimeSeriesProperty<double>*>(ws->getSample()->getLogData(logName));
                    double Y,E; 
                    calcIntAsymmetry(ws,Y,E);
                    outWS->dataY(0)[i-is] = Y;
                    outWS->dataX(0)[i-is] = logp->lastValue();
                    outWS->dataE(0)[i-is] = E;
                }
                
                std::stringstream suffix;
                suffix << (++period);
                wsProp = "OutputWorkspace" + suffix.str();// form the property name for higher periods
                //wsName = "tmp"+fnn.str() + "_" + suffix.str();
            }
        }

        // Assign the result to the output workspace property
        setProperty("OutputWorkspace",outWS);
	    
    }

    /**  Calculate the integral asymmetry for a workspace. 
     *   The calculation is done by MuonAsymmetryCalc and SimpleIntegration algorithms.
     */
    void PlotAsymmetryByLogValue::calcIntAsymmetry(boost::shared_ptr<DataObjects::Workspace2D> ws, double& Y, double& E)
    {
        if (!m_int)
        {   //  "Differential asymmetry"
            IAlgorithm_sptr asym = createSubAlgorithm("AsymmetryCalc");
            asym->initialize();
            MatrixWorkspace_sptr mws = boost::dynamic_pointer_cast<MatrixWorkspace>(ws);
            asym->setProperty("InputWorkspace",mws);
            asym->setPropertyValue("OutputWorkspace","tmp");
            asym->execute();
            MatrixWorkspace_sptr asymWS = asym->getProperty("OutputWorkspace");

            IAlgorithm_sptr integr = createSubAlgorithm("Integration");
            integr->setProperty("InputWorkspace",asymWS);
            integr->setPropertyValue("OutputWorkspace","tmp");
            integr->execute();
            MatrixWorkspace_sptr out = integr->getProperty("OutputWorkspace");

            Y = out->readY(0)[0];
            E = out->readE(0)[0];
        }
        else
        {   
            //double dlt = 0.1;
            //for(int i=0;i<ws->blocksize();i++)
            //{
            //    double f = ws->dataY(0)[i];
            //    double fd = (1. - dlt*f);
            //    ws->dataY(0)[i] = fd? f / fd : 0;

            //    double b = ws->dataY(1)[i];
            //    double bd = (1. - dlt*b);
            //    ws->dataY(1)[i] = bd? b / bd : 0;
            //}
            //  "Integral asymmetry"
            IAlgorithm_sptr integr = createSubAlgorithm("Integration");
            integr->setProperty("InputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(ws));
            integr->setPropertyValue("OutputWorkspace","tmp");
            integr->execute();
            MatrixWorkspace_sptr intWS = integr->getProperty("OutputWorkspace");

            IAlgorithm_sptr asym = createSubAlgorithm("AsymmetryCalc");
            asym->initialize();
            asym->setProperty("InputWorkspace",intWS);
            asym->setPropertyValue("OutputWorkspace","tmp");
            asym->execute();
            MatrixWorkspace_sptr out = asym->getProperty("OutputWorkspace");

            Y = out->readY(0)[0];
            E = out->readE(0)[0];

        }


    }

  } // namespace Algorithm
} // namespace Mantid




