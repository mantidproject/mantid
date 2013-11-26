/*WIKI* 


This algorithm calculates asymmetry for a series of muon workspaces. The input workspaces must be in Muon Nexus files which names follow the rule: the filename must begin with at least 1 letter and followed by a number. The input property FirstRun must be set to the file name with the smalest number and the LastRun to the one with the highest number. If the "Green" property is not set the output workspace will contain a single spectrum with asymmetry values. If the "Green" is set the output workspace will contain four spectra with asymmetries: 

{| border="1" cellpadding="5" cellspacing="0"
!Workspace Index
!Spectrum
!Asymmetry
|-
|0
|1
|Difference of Red and Green
|-
|1
|2
|Red only
|-
|2
|3
|Green only
|-
|3
|4
|Sum of red and green asymmetries
|}

If ForwardSpectra and BackwardSpectra are set the muon workspaces will be grouped according to the user input, otherwise the Autogroup option of LoadMuonNexus will be used for grouping.

There is a python script PlotAsymmetryByLogValue.py which if called in MantidPlot runs the algorithm and plots the results.


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <cmath>
#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>

#include "MantidAlgorithms/PlotAsymmetryByLogValue.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/TextAxis.h"

#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>

namespace // anonymous
{

/**
 * Convert a log property to a double value.
 *
 * @param property :: Pointer to a TimeSeriesProperty.
 * @param value :: Returned double value.
 * @return :: True if successful
 */
template<typename T>
bool convertLogToDouble(const Mantid::Kernel::Property *property, double &value)
{
    const Mantid::Kernel::TimeSeriesProperty<T>* log = dynamic_cast<const Mantid::Kernel::TimeSeriesProperty<T>*>( property );
    if ( log )
    {
        value = static_cast<double>( log->lastValue() );
        return true;
    }
    auto tlog = dynamic_cast<const Mantid::Kernel::PropertyWithValue<T>*>( property );
    if ( tlog )
    {
        value = static_cast<double>(*tlog);
        return true;
    }
    return false;
}

} // anonymous

namespace Mantid
{
  namespace Algorithms
  {

    using namespace Kernel;
    using namespace API;
    using namespace DataObjects;

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(PlotAsymmetryByLogValue)
    
    /// Sets documentation strings for this algorithm
    void PlotAsymmetryByLogValue::initDocs()
    {
      this->setWikiSummary("Calculates asymmetry for a series of log values ");
      this->setOptionalMessage("Calculates asymmetry for a series of log values");
    }
    

    /** Initialisation method. Declares properties to be used in algorithm.
    *
    */
    void PlotAsymmetryByLogValue::init()
    {
      std::string nexusExt(".nxs");

      declareProperty(new FileProperty("FirstRun","", FileProperty::Load, nexusExt), "The name of the first workspace in the series.");
      declareProperty(new FileProperty("LastRun","", FileProperty::Load, nexusExt), "The name of the last workspace in the series.");
      declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "The name of the output workspace containing the resulting asymmetries.");
      declareProperty("LogValue","",boost::make_shared<MandatoryValidator<std::string>>(), "The name of the log values which will be used as the x-axis in the output workspace.");
      declareProperty("Red", 1, "The period number for the 'red' data.");
      declareProperty("Green", EMPTY_INT(), "The period number for the 'green' data.");
      
      std::vector<std::string> options;
      options.push_back("Integral");
      options.push_back("Differential");
      declareProperty("Type","Integral",boost::make_shared<StringListValidator>(options),"The calculation type: 'Integral' or 'Differential'.");

      declareProperty("TimeMin",EMPTY_DBL(),"The beginning of the time interval used in the calculations.");
      declareProperty("TimeMax",EMPTY_DBL(),"The end of the time interval used in the calculations.");

       declareProperty(new ArrayProperty<int> ("ForwardSpectra"),
         "The list of spectra for the forward group. If not specified the following happens. The data will be grouped according to grouping information in the data, if available. The forward will use the first of these groups.");
       declareProperty(new ArrayProperty<int> ("BackwardSpectra"),
         "The list of spectra for the backward group. If not specified the following happens. The data will be grouped according to grouping information in the data, if available. The backward will use the second of these groups.");
    
      std::vector<std::string> deadTimeCorrTypes;
      deadTimeCorrTypes.push_back("None");
      deadTimeCorrTypes.push_back("FromRunData");
      deadTimeCorrTypes.push_back("FromSpecifiedFile");

      declareProperty("DeadTimeCorrType", deadTimeCorrTypes[0], 
        boost::make_shared<StringListValidator>(deadTimeCorrTypes), 
        "Type of Dead Time Correction to apply.");

      declareProperty(new FileProperty("DeadTimeCorrFile", "", FileProperty::OptionalLoad, nexusExt), 
        "Custom file with Dead Times. Will be used only if appropriate DeadTimeCorrType is set.");
    }

    /** 
    *   Executes the algorithm
    */
    void PlotAsymmetryByLogValue::exec()
    {
      m_forward_list = getProperty("ForwardSpectra");		
      m_backward_list = getProperty("BackwardSpectra");
      m_autogroup = ( m_forward_list.size() == 0 && m_backward_list.size() == 0);

      std::string logName = getProperty("LogValue");

      int red = getProperty("Red");
      int green = getProperty("Green");

      std::string stype = getProperty("Type");
      m_int = stype == "Integral";

      std::string dtcType = getProperty("DeadTimeCorrType");

      Workspace_sptr customDTT; // Workspace with custom Dead Time Table[s]

      // If use wants to use Dead Times from his own file, load them
      if(dtcType == "FromSpecifiedFile")
      {
        const std::string dtcFile = getProperty("DeadTimeCorrFile");

        try
        {
          IAlgorithm_sptr loadDeadTimes = createChildAlgorithm("LoadNexusProcessed");
          loadDeadTimes->setPropertyValue("Filename", dtcFile);
          loadDeadTimes->execute();

          customDTT = loadDeadTimes->getProperty("OutputWorkspace");
        }
        catch(...)
        {
          throw std::runtime_error("Unable to load Dead Time Table from the file. Please use correct file or set Deat Time Correction to None");
        }
      }

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
      int w  = static_cast<int>(firstFN.size());

      // The number of runs
      size_t npoints = ie - is + 1;

      // Create the 2D workspace for the output
      int nplots = green != EMPTY_INT() ? 4 : 1;
      MatrixWorkspace_sptr outWS = WorkspaceFactory::Instance().create("Workspace2D",
        nplots,          //  the number of plots
        npoints,    //  the number of data points on a plot
        npoints     //  it's not a histogram
        );
      TextAxis* tAxis = new TextAxis(nplots);
      if (nplots == 1)
      {
        tAxis->setLabel(0,"Asymmetry");
      }
      else
      {
        tAxis->setLabel(0,"Red-Green");
        tAxis->setLabel(1,"Red");
        tAxis->setLabel(2,"Green");
        tAxis->setLabel(3,"Red+Green");
      }
      outWS->replaceAxis(1,tAxis);

      Progress progress(this,0,1,ie-is+2);
      for(size_t i=is;i<=ie;i++)
      {
        std::ostringstream fn,fnn;
        fnn << std::setw(w) << std::setfill('0') << i ;
        fn << fnBase << fnn.str() << ext;

        // Load a muon nexus file with auto_group set to true
        IAlgorithm_sptr loadNexus = createChildAlgorithm("LoadMuonNexus");
        loadNexus->initialize();
        loadNexus->setPropertyValue("Filename", fn.str());
        loadNexus->execute();

        Workspace_sptr loadedWs = loadNexus->getProperty("OutputWorkspace");

        // Apply Dead Time Correction if necessary
        if(dtcType != "None")
        {
          try
          {
            if ( dtcType == "FromRunData" )
            {
              Workspace_sptr runDataDTT = loadNexus->getProperty("DeadTimeTable");
              loadedWs = applyDeadTimeCorrection(runDataDTT, loadedWs);
            }
            else
            {
              loadedWs = applyDeadTimeCorrection(customDTT, loadedWs);
            }
          }
          catch(...)
          {
            throw std::runtime_error("Unable to apply Dead Time correction. Please change the Dead Time Table used or set Deat Time Correction to None");
          }
        }

        if(m_autogroup)
        {
          IAlgorithm_sptr applyGrouping = createChildAlgorithm("ApplyGroupingFromMuonNexus");
          applyGrouping->initialize();
          applyGrouping->setProperty("InputWorkspace", loadedWs);
          applyGrouping->setPropertyValue("Filename", fn.str());

          try
          {
            applyGrouping->execute();
          }
          catch(...)
          {
            throw std::runtime_error("Unable to auto-group the workspace. Please specify grouping manually");
          }

          loadedWs = applyGrouping->getProperty("OutputWorkspace");
        }

        WorkspaceGroup_sptr loadedGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(loadedWs);

        if (!loadedGroup)
        {
          Workspace2D_sptr loadedWs2D = boost::dynamic_pointer_cast<Workspace2D>(loadedWs);

          double Y,E; 
          calcIntAsymmetry(loadedWs2D, Y, E);
          outWS->dataY(0)[i-is] = Y;
          outWS->dataX(0)[i-is] = getLogValue(*loadedWs2D, logName);
          outWS->dataE(0)[i-is] = E;
        }
        else
        {
          DataObjects::Workspace2D_sptr ws_red;
          DataObjects::Workspace2D_sptr ws_green;

          // Run through the periods of the loaded file and do calculations on the selected ones
          for(int mi = 0; mi < loadedGroup->getNumberOfEntries(); mi++)
          {
            Workspace2D_sptr memberWs = 
              boost::dynamic_pointer_cast<Workspace2D>(loadedGroup->getItem(mi));

            int period = mi + 1;

            // Do only one period
            if (green == EMPTY_INT() && period == red)
            {
              ws_red = memberWs;
              double Y,E; 
              calcIntAsymmetry(ws_red,Y,E);
              outWS->dataY(0)[i-is] = Y;
              outWS->dataX(0)[i-is] = getLogValue(*ws_red, logName);
              outWS->dataE(0)[i-is] = E;
            }
            else // red & green
            {
              if (period == red)
                ws_red = memberWs;
              if (period == green)
                ws_green = memberWs;
            }

          }
          // red & green claculation
          if (green != EMPTY_INT())
          {
            if (!ws_red || !ws_green)
              throw std::invalid_argument("Red or green period is out of range");
            double Y,E; 
            double Y1,E1;
            double logValue = getLogValue( *ws_red, logName );
            calcIntAsymmetry(ws_red,Y,E);
            calcIntAsymmetry(ws_green,Y1,E1);
            outWS->dataY(1)[i-is] = Y;
            outWS->dataX(1)[i-is] = logValue;
            outWS->dataE(1)[i-is] = E;

            outWS->dataY(2)[i-is] = Y1;
            outWS->dataX(2)[i-is] = logValue;
            outWS->dataE(2)[i-is] = E1;

            outWS->dataY(3)[i-is] = Y + Y1;
            outWS->dataX(3)[i-is] = logValue;
            outWS->dataE(3)[i-is] = sqrt(E*E+E1*E1);

            // move to last for safety since some grouping takes place in the
            // calcIntAsymmetry call below
            calcIntAsymmetry(ws_red,ws_green,Y,E);
            outWS->dataY(0)[i-is] = Y;
            outWS->dataX(0)[i-is] = logValue;
            outWS->dataE(0)[i-is] = E;
          }
          else
            if (!ws_red)
              throw std::invalid_argument("Red period is out of range");

        }
        progress.report();
      }

      outWS->getAxis(0)->title() = logName;
      outWS->setYUnitLabel("Asymmetry");
      
      // Assign the result to the output workspace property
      setProperty("OutputWorkspace", outWS);

    }

    /**  Calculate the integral asymmetry for a workspace. 
    *   The calculation is done by MuonAsymmetryCalc and SimpleIntegration algorithms.
    *   @param ws :: The workspace
    *   @param Y :: Reference to a variable receiving the value of asymmetry
    *   @param E :: Reference to a variable receiving the value of the error
    */
    void PlotAsymmetryByLogValue::calcIntAsymmetry(API::MatrixWorkspace_sptr ws, double& Y, double& E)
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
        IAlgorithm_sptr asym = createChildAlgorithm("AsymmetryCalc");
        asym->initialize();
        asym->setProperty("InputWorkspace",ws);
        asym->setPropertyValue("OutputWorkspace","tmp");
        if ( !m_autogroup )
        {
          asym->setProperty("ForwardSpectra",m_forward_list);
          asym->setProperty("BackwardSpectra",m_backward_list);
        }
        asym->execute();
        MatrixWorkspace_sptr asymWS = asym->getProperty("OutputWorkspace");

        IAlgorithm_sptr integr = createChildAlgorithm("Integration");
        integr->setProperty("InputWorkspace",asymWS);
        integr->setPropertyValue("OutputWorkspace","tmp");
        if (setX)
        {
          integr->setProperty("RangeLower",startX);
          integr->setProperty("RangeUpper",endX);
        }
        integr->execute();
        API::MatrixWorkspace_sptr out = integr->getProperty("OutputWorkspace");

        Y = out->readY(0)[0];
        E = out->readE(0)[0];
      }
      else
      {   
        //  "Integral asymmetry"
        IAlgorithm_sptr integr = createChildAlgorithm("Integration");
        integr->setProperty("InputWorkspace", ws);
        integr->setPropertyValue("OutputWorkspace","tmp");
        if (setX)
        {
          integr->setProperty("RangeLower",startX);
          integr->setProperty("RangeUpper",endX);
        }
        integr->execute();
        API::MatrixWorkspace_sptr intWS = integr->getProperty("OutputWorkspace");

        IAlgorithm_sptr asym = createChildAlgorithm("AsymmetryCalc");
        asym->initialize();
        asym->setProperty("InputWorkspace",intWS);
        asym->setPropertyValue("OutputWorkspace","tmp");
        if ( !m_autogroup )
        {
          asym->setProperty("ForwardSpectra",m_forward_list);
          asym->setProperty("BackwardSpectra",m_backward_list);
        }
        asym->execute();
        MatrixWorkspace_sptr out = asym->getProperty("OutputWorkspace");

        Y = out->readY(0)[0];
        E = out->readE(0)[0];

      }


    }

    /**  Calculate the integral asymmetry for a workspace (red & green). 
    *   The calculation is done by MuonAsymmetryCalc and SimpleIntegration algorithms.
    *   @param ws_red :: The red workspace
    *   @param ws_green :: The green workspace
    *   @param Y :: Reference to a variable receiving the value of asymmetry
    *   @param E :: Reference to a variable receiving the value of the error
    */
    void PlotAsymmetryByLogValue::calcIntAsymmetry(API::MatrixWorkspace_sptr ws_red,
      API::MatrixWorkspace_sptr ws_green,double& Y, double& E)
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

        API::MatrixWorkspace_sptr tmpWS = API::WorkspaceFactory::Instance().create(
                                            ws_red,1,ws_red->readX(0).size(),ws_red->readY(0).size());

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

        IAlgorithm_sptr integr = createChildAlgorithm("Integration");
        integr->setProperty("InputWorkspace",tmpWS);
        integr->setPropertyValue("OutputWorkspace","tmp");
        if (setX)
        {
          integr->setProperty("RangeLower",startX);
          integr->setProperty("RangeUpper",endX);
        }
        integr->execute();
        MatrixWorkspace_sptr out = integr->getProperty("OutputWorkspace");

        Y = out->readY(0)[0] / static_cast<double>(tmpWS->dataY(0).size());
        E = out->readE(0)[0] / static_cast<double>(tmpWS->dataY(0).size());
      }
      else
      {   
        //  "Integral asymmetry"
        IAlgorithm_sptr integr = createChildAlgorithm("Integration");
        integr->setProperty("InputWorkspace", ws_red);
        integr->setPropertyValue("OutputWorkspace","tmp");
        if (setX)
        {
          integr->setProperty("RangeLower",startX);
          integr->setProperty("RangeUpper",endX);
        }
        integr->execute();
        API::MatrixWorkspace_sptr intWS_red = integr->getProperty("OutputWorkspace");

        integr = createChildAlgorithm("Integration");
        integr->setProperty("InputWorkspace", ws_green);
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
     *  @param ws :: A local workspace
     *  @param spectraList :: A list of spectra to group.
     */
    void PlotAsymmetryByLogValue::groupDetectors(API::MatrixWorkspace_sptr& ws,const std::vector<int>& spectraList)
    {
      API::IAlgorithm_sptr group = createChildAlgorithm("GroupDetectors");
      group->setProperty("InputWorkspace",ws);
      group->setProperty("SpectraList",spectraList);
      group->setProperty("KeepUngroupedSpectra",true);
      group->execute();
      ws = group->getProperty("OutputWorkspace");
    }

    /**
     * Get log value from a workspace. Convert to double if possible.
     *
     * @param ws :: The input workspace.
     * @param logName :: Name of the log file.
     * @return :: Log value.
     * @throw :: std::invalid_argument if the log cannot be converted to a double or doesn't exist.
     */
    double PlotAsymmetryByLogValue::getLogValue(MatrixWorkspace &ws, const std::string &logName)
    {
        auto *property = ws.run().getLogData(logName);
        if ( !property )
        {
            throw std::invalid_argument("Log "+logName+" does not exist.");
        }

        double value = 0;
        // try different property types
        if ( convertLogToDouble<double>(property, value) )              return value;
        if ( convertLogToDouble<float>(property, value) )               return value;
        if ( convertLogToDouble<int>(property, value) )                 return value;
        if ( convertLogToDouble<long>(property, value) )                return value;
        if ( convertLogToDouble<long long>(property, value) )           return value;
        if ( convertLogToDouble<unsigned int>(property, value) )        return value;
        if ( convertLogToDouble<unsigned long>(property, value) )       return value;
        if ( convertLogToDouble<unsigned long long>(property, value) )  return value;
        // try if it's a string and can be lexically cast to double
        auto slog = dynamic_cast<const Mantid::Kernel::PropertyWithValue<std::string>*>( property );
        if ( slog )
        {
            try
            {
                value = boost::lexical_cast<double>(slog->value());
                return value;
            }
            catch(std::exception&)
            {
                // do nothing, goto throw
            }
        }

        throw std::invalid_argument("Log "+logName+" cannot be converted to a double type.");
    }

    /**
     * Runs an appropriate applyDeadTimeCorrection function depending on the type of workspaces.
     *
     * @param deadTimeTable :: 
     */ 
    Workspace_sptr PlotAsymmetryByLogValue::applyDeadTimeCorrection(Workspace_sptr deadTimeWs, 
      Workspace_sptr ws)
    {
      WorkspaceGroup_sptr deadTimeGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(deadTimeWs);
      ITableWorkspace_sptr deadTimeTable = boost::dynamic_pointer_cast<ITableWorkspace>(deadTimeWs);

      if(!deadTimeGroup && !deadTimeTable)
        throw std::invalid_argument("Unsupported type of Dead Time Table");

      WorkspaceGroup_sptr wsGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(ws);
      Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);

      if(wsGroup)
      {
        if(deadTimeGroup)
          return applyDeadTimeCorrection(deadTimeGroup, wsGroup);
        else
          return applyDeadTimeCorrection(deadTimeTable, wsGroup);
      }
      else if(ws2D)
      {
        if(deadTimeTable)
          return applyDeadTimeCorrection(deadTimeTable, ws2D);
        else
          throw std::invalid_argument("Can't apply group of tables to a single workspace");
      }
      else
      {
        throw std::invalid_argument("Unsupported type of the input workspace");
      }
    }

    /**
     * Applies a Dead Time Correction to a group of workspace using a single table.
     *
     * @param deadTimeTable :: Dead Time Table to be applied
     * @param       wsGroup :: Group of workspaces to apply correction to
     *
     * @return Group of workspaces with DTC applied
     */
    WorkspaceGroup_sptr PlotAsymmetryByLogValue::applyDeadTimeCorrection(
      ITableWorkspace_sptr deadTimeTable, WorkspaceGroup_sptr wsGroup)
    {
      WorkspaceGroup_sptr outputGroup = boost::make_shared<WorkspaceGroup>();

      for(size_t i = 0; i < wsGroup->size(); i++)
      {
        Workspace2D_sptr member = boost::dynamic_pointer_cast<Workspace2D>(wsGroup->getItem(i));

        if(!member)
          throw std::invalid_argument("Group contains unsupported type of workspace");
        
        Workspace2D_sptr outputWs = applyDeadTimeCorrection(deadTimeTable, member);

        outputGroup->addWorkspace(outputWs);
      }

      return outputGroup;
    }

    /**
     * Applies Dead Time Correction to a group workspaces using a group of tables. Each table
     * is applied to the corresponding ws from the group.
     *
     * @param deadTimeGroup :: Group of Dead Time Tables to be applied
     * @param       wsGroup :: Group of workspaces to apply correction to
     *
     * @return Group of workspaces with DTC applied
     */
    WorkspaceGroup_sptr PlotAsymmetryByLogValue::applyDeadTimeCorrection(
      WorkspaceGroup_sptr deadTimeGroup, WorkspaceGroup_sptr wsGroup)
    {
      if(deadTimeGroup->size() != wsGroup->size())
        throw std::invalid_argument("Dead Time Table group size is not equal to ws group sizes");

      WorkspaceGroup_sptr outputGroup = boost::make_shared<WorkspaceGroup>();

      for(size_t i = 0; i < wsGroup->size(); i++)
      {
        Workspace2D_sptr wsMember = boost::dynamic_pointer_cast<Workspace2D>(wsGroup->getItem(i));

        if(!wsMember)
          throw std::invalid_argument("Group contains unsupported type of workspace");

        ITableWorkspace_sptr deadTimeMember = 
          boost::dynamic_pointer_cast<ITableWorkspace>(deadTimeGroup->getItem(i));

        if(!deadTimeMember)
          throw std::invalid_argument("Dead Time Table group contains workspace which is not a table");
          
        Workspace2D_sptr outputWs = applyDeadTimeCorrection(deadTimeMember, wsMember);

        outputGroup->addWorkspace(outputWs);
      }

      return outputGroup;
    }

    /**
     * Runs ApplyDeadTimeCorr algorithm to apply Dead Time Correction to a ws using a given table.
     *
     * @param deadTimeTable :: Dead Time Table to be applied
     * @param            ws :: Workspace to apply correction to
     *
     * @return Workspace with DTC applied
     */
    Workspace2D_sptr PlotAsymmetryByLogValue::applyDeadTimeCorrection(
      ITableWorkspace_sptr deadTimeTable, Workspace2D_sptr ws)
    {
      IAlgorithm_sptr applyDtc = createChildAlgorithm("ApplyDeadTimeCorr");

      applyDtc->setProperty<MatrixWorkspace_sptr>("InputWorkspace", ws);
      applyDtc->setProperty<ITableWorkspace_sptr>("DeadTimeTable", deadTimeTable);
      applyDtc->execute();

      MatrixWorkspace_sptr output = applyDtc->getProperty("OutputWorkspace");

      return boost::dynamic_pointer_cast<Workspace2D>(output);
    }

  } // namespace Algorithm
} // namespace Mantid




