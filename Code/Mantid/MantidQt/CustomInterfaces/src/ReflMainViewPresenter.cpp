#include "MantidQtCustomInterfaces/ReflMainViewPresenter.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Utils.h"
#include "MantidQtCustomInterfaces/ReflMainView.h"

#include <boost/regex.hpp>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace MantidQt
{
  namespace CustomInterfaces
  {
    ReflMainViewPresenter::ReflMainViewPresenter(ReflMainView* view): m_view(view)
    {
    }

    ReflMainViewPresenter::ReflMainViewPresenter(ITableWorkspace_sptr model, ReflMainView* view): m_model(model), m_view(view)
    {
    }

    ReflMainViewPresenter::~ReflMainViewPresenter()
    {
    }

    /**
     * Finds the first unused group id
     */
    int ReflMainViewPresenter::getUnusedGroup(std::vector<size_t> ignoredRows) const
    {
      std::vector<int> usedGroups;

      //Scan through all the rows, working out which group ids are used
      for(size_t idx = 0; idx < m_model->rowCount(); ++idx)
      {
        if(std::find(ignoredRows.begin(), ignoredRows.end(), idx) != ignoredRows.end())
          continue;

        //This is an unselected row. Add it to the list of used group ids
        usedGroups.push_back(m_model->Int(idx, COL_GROUP));
      }

      int groupId = 0;

      //While the group id is one of the used ones, increment it by 1
      while(std::find(usedGroups.begin(), usedGroups.end(), groupId) != usedGroups.end())
        groupId++;

      return groupId;
    }

    /**
    Process selected rows
    */
    void ReflMainViewPresenter::process()
    {
      if(m_model->rowCount() == 0)
      {
        m_view->giveUserWarning("Cannot process an empty Table","Warning");
        return;
      }

      std::vector<size_t> rows = m_view->getSelectedRowIndexes();
      if(rows.size() == 0)
      {
        //Does the user want to abort?
        if(!m_view->askUserYesNo("This will process all rows in the table. Continue?","Process all rows?"))
          return;

        //They want to process all rows, so populate rows with every index in the model
        for(size_t idx = 0; idx < m_model->rowCount(); ++idx)
          rows.push_back(idx);
      }

      //Maps group numbers to the list of rows in that group we want to process
      std::map<int,std::vector<size_t> > groups;
      for(auto it = rows.begin(); it != rows.end(); ++it)
      {
        try
        {
          validateRow(*it);
          autofillRow(*it);

          const int group = m_model->Int(*it, COL_GROUP);
          groups[group].push_back(*it);
        }
        catch(std::exception& ex)
        {
          const std::string rowNo = Mantid::Kernel::Strings::toString<size_t>(*it + 1);
          m_view->giveUserCritical("Error found in row " + rowNo + ":\n" + ex.what(), "Error");
          return;
        }
      }

      int progress = 0;
      //Each group and each row within count as a progress step.
      const int maxProgress = (int)(rows.size() + groups.size());
      m_view->setProgressRange(progress, maxProgress);
      m_view->setProgress(progress);

      for(auto gIt = groups.begin(); gIt != groups.end(); ++gIt)
      {
        const std::vector<size_t> groupRows = gIt->second;

        //Process each row individually
        for(auto rIt = groupRows.begin(); rIt != groupRows.end(); ++rIt)
        {
          try
          {
            processRow(*rIt);
            m_view->setProgress(++progress);
          }
          catch(std::exception& ex)
          {
            const std::string rowNo = Mantid::Kernel::Strings::toString<size_t>(*rIt + 1);
            const std::string message = "Error encountered while processing row " + rowNo + ":\n";
            m_view->giveUserCritical(message + ex.what(), "Error");
            m_view->setProgress(0);
            return;
          }
        }

        try
        {
          stitchRows(groupRows);
          m_view->setProgress(++progress);
        }
        catch(std::exception& ex)
        {
          const std::string groupNo = Mantid::Kernel::Strings::toString<int>(gIt->first);
          const std::string message = "Error encountered while stitching group " + groupNo + ":\n";
          m_view->giveUserCritical(message + ex.what(), "Error");
          m_view->setProgress(0);
          return;
        }
      }
    }

    /**
    Validate a row.
    If a row passes validation, it is ready to be autofilled, but
    not necessarily ready for processing.
    @param rowNo : The row in the model to validate
    @throws std::invalid_argument if the row fails validation
    */
    void ReflMainViewPresenter::validateRow(size_t rowNo) const
    {
      if(rowNo >= m_model->rowCount())
        throw std::invalid_argument("Invalid row");

      if(m_model->String(rowNo, COL_RUNS).empty())
        throw std::invalid_argument("Run column may not be empty.");
    }

    /**
    Autofill a row
    @param rowNo : The row in the model to autofill
    @throws std::runtime_error if the row could not be auto-filled
    */
    void ReflMainViewPresenter::autofillRow(size_t rowNo)
    {
      if(rowNo >= m_model->rowCount())
        throw std::runtime_error("Invalid row");

      const std::string runStr = m_model->String(rowNo, COL_RUNS);
      MatrixWorkspace_sptr run = boost::dynamic_pointer_cast<MatrixWorkspace>(loadRun(runStr, m_view->getProcessInstrument()));

      //Fetch two theta from the log if needed
      if(m_model->String(rowNo, COL_ANGLE).empty())
      {
        Property* logData = NULL;

        //First try TwoTheta
        try
        {
          logData = run->mutableRun().getLogData("Theta");
        }
        catch(std::exception&)
        {
          throw std::runtime_error("Value for two theta could not be found in log.");
        }

        auto logPWV = dynamic_cast<const PropertyWithValue<double>*>(logData);
        auto logTSP = dynamic_cast<const TimeSeriesProperty<double>*>(logData);

        double thetaVal;
        if(logPWV)
          thetaVal = *logPWV;
        else if(logTSP && logTSP->realSize() > 0)
          thetaVal = logTSP->lastValue();
        else
          throw std::runtime_error("Value for two theta could not be found in log.");

        //Update the model
        m_model->String(rowNo, COL_ANGLE) = Strings::toString<double>(Utils::roundToDP(thetaVal, 3));
      }

      //If we need to calculate the resolution, do.
      if(m_model->String(rowNo, COL_DQQ).empty())
      {
        IAlgorithm_sptr calcResAlg = AlgorithmManager::Instance().create("CalculateResolution");
        calcResAlg->setProperty("Workspace", run);
        calcResAlg->setProperty("TwoTheta", m_model->String(rowNo, COL_ANGLE));
        calcResAlg->execute();

        //Update the model
        double dqqVal = calcResAlg->getProperty("Resolution");
        m_model->String(rowNo, COL_DQQ) = Strings::toString<double>(dqqVal);
      }

      //Make sure the view updates
      m_view->showTable(m_model);
    }

    /**
    Extracts the run number of a workspace
    @param ws : The workspace to fetch the run number from
    @returns The run number of the workspace
    */
    std::string ReflMainViewPresenter::getRunNumber(const Workspace_sptr& ws)
    {
      //If we can, use the run number from the workspace's sample log
      MatrixWorkspace_sptr mws = boost::dynamic_pointer_cast<MatrixWorkspace>(ws);
      if(mws)
      {
        try
        {
          const Property* runProperty = mws->mutableRun().getLogData("run_number");
          auto runNumber = dynamic_cast<const PropertyWithValue<std::string>*>(runProperty);
          if(runNumber)
            return *runNumber;
        }
        catch(Mantid::Kernel::Exception::NotFoundError&)
        {
          //We'll just fall back to looking at the workspace's name
        }
      }

      //Okay, let's see what we can get from the workspace's name
      const std::string wsName = ws->name();

      //Matches TOF_13460 -> 13460
      boost::regex outputRegex("(TOF|IvsQ|IvsLam)_([0-9]+)");

      //Matches INTER13460 -> 13460
      boost::regex instrumentRegex("[a-zA-Z]{3,}([0-9]{3,})");

      boost::smatch matches;

      if(boost::regex_match(wsName, matches, outputRegex))
      {
        return matches[2].str();
      }
      else if(boost::regex_match(wsName, matches, instrumentRegex))
      {
        return matches[1].str();
      }

      //Resort to using the workspace name
      return wsName;
    }

    /**
    Loads a run from disk or fetches it from the AnalysisDataService
    @param run : The name of the run
    @param instrument : The instrument the run belongs to
    @throws std::runtime_error if the run could not be loaded
    @returns a shared pointer to the workspace
    */
    Workspace_sptr ReflMainViewPresenter::loadRun(const std::string& run, const std::string& instrument = "")
    {
      //First, let's see if the run given is the name of a workspace in the ADS
      if(AnalysisDataService::Instance().doesExist(run))
        return AnalysisDataService::Instance().retrieveWS<Workspace>(run);

      //Is the run string is numeric
      if(boost::regex_match(run, boost::regex("\\d+")))
      {
        std::string wsName;

        //Look "TOF_<run_number>" in the ADS
        wsName = "TOF_" + run;
        if(AnalysisDataService::Instance().doesExist(wsName))
          return AnalysisDataService::Instance().retrieveWS<Workspace>(wsName);

        //Look for "<instrument><run_number>" in the ADS
        wsName = instrument + run;
        if(AnalysisDataService::Instance().doesExist(wsName))
          return AnalysisDataService::Instance().retrieveWS<Workspace>(wsName);
      }

      //We'll just have to load it ourselves
      const std::string filename = instrument + run;
      IAlgorithm_sptr algLoadRun = AlgorithmManager::Instance().create("Load");
      algLoadRun->initialize();
      algLoadRun->setProperty("Filename", filename);
      algLoadRun->setProperty("OutputWorkspace", "TOF_" + run);
      algLoadRun->execute();

      if(!algLoadRun->isExecuted())
        throw std::runtime_error("Could not open " + filename);

      return AnalysisDataService::Instance().retrieveWS<Workspace>("TOF_" + run);
    }

    /**
    Process a row
    @param rowNo : The row in the model to process
    @throws std::runtime_error if processing fails
    */
    void ReflMainViewPresenter::processRow(size_t rowNo)
    {
      const std::string         run = m_model->String(rowNo, COL_RUNS);
      const std::string    transStr = m_model->String(rowNo, COL_TRANSMISSION);
      const std::string     options = m_model->String(rowNo, COL_OPTIONS);

      double theta = 0;

      const bool thetaGiven = !m_model->String(rowNo, COL_ANGLE).empty();

      if(thetaGiven)
        Mantid::Kernel::Strings::convert<double>(m_model->String(rowNo, COL_ANGLE), theta);

      Workspace_sptr runWS = loadRun(run, m_view->getProcessInstrument());
      const std::string runNo = getRunNumber(runWS);

      MatrixWorkspace_sptr transWS;
      if(!transStr.empty())
        transWS = makeTransWS(transStr);

      IAlgorithm_sptr algReflOne = AlgorithmManager::Instance().create("ReflectometryReductionOneAuto");
      algReflOne->initialize();
      algReflOne->setProperty("InputWorkspace", runWS);
      if(transWS)
        algReflOne->setProperty("FirstTransmissionRun", transWS);
      algReflOne->setProperty("OutputWorkspace", "IvsQ_" + runNo);
      algReflOne->setProperty("OutputWorkspaceWaveLength", "IvsLam_" + runNo);
      algReflOne->setProperty("ThetaIn", theta);

      //Parse and set any user-specified options
      auto optionsMap = Mantid::Kernel::Strings::splitToKeyValues(options);
      for(auto kvp = optionsMap.begin(); kvp != optionsMap.end(); ++kvp)
      {
        try
        {
          algReflOne->setProperty(kvp->first, kvp->second);
        }
        catch(Mantid::Kernel::Exception::NotFoundError&)
        {
          throw std::runtime_error("Invalid property in options column: " + kvp->first);
        }
      }

      algReflOne->execute();

      if(!algReflOne->isExecuted())
        throw std::runtime_error("Failed to run ReflectometryReductionOneAuto.");

      //Processing has completed. Put Qmin and Qmax into the table if needed, for stitching.
      if(m_model->String(rowNo, COL_QMIN).empty() || m_model->String(rowNo, COL_QMAX).empty())
      {
        MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("IvsQ_" + runNo);
        std::vector<double> qrange = calcQRange(ws, theta);

        if(m_model->String(rowNo, COL_QMIN).empty())
          m_model->String(rowNo, COL_QMIN) = Strings::toString<double>(qrange[0]);

        if(m_model->String(rowNo, COL_QMAX).empty())
          m_model->String(rowNo, COL_QMAX) = Strings::toString<double>(qrange[1]);

        m_view->showTable(m_model);
      }
    }

    /**
    Calculates the minimum and maximum values for Q
    @param ws : The workspace to fetch the instrument values from
    @param theta : The value of two theta to use in calculations
    */
    std::vector<double> ReflMainViewPresenter::calcQRange(MatrixWorkspace_sptr ws, double theta)
    {
      double lmin, lmax;
      try
      {
        const Instrument_const_sptr instrument = ws->getInstrument();
        lmin = instrument->getNumberParameter("LambdaMin")[0];
        lmax = instrument->getNumberParameter("LambdaMax")[0];
      }
      catch(std::exception&)
      {
        throw std::runtime_error("LambdaMin/LambdaMax instrument parameters are required to calculate qmin/qmax");
      }

      double qmin = 4 * M_PI / lmax * sin(theta * M_PI / 180.0);
      double qmax = 4 * M_PI / lmin * sin(theta * M_PI / 180.0);
      qmin = Utils::roundToDP(qmin, 3);
      qmax = Utils::roundToDP(qmax, 3);

      std::vector<double> ret;
      ret.push_back(qmin);
      ret.push_back(qmax);
      return ret;
    }

    /**
    Stitches the workspaces created by the given rows together.
    @param rows : the list of rows
    */
    void ReflMainViewPresenter::stitchRows(std::vector<size_t> rows)
    {
      //If we can get away with doing nothing, do.
      if(rows.size() < 2)
        return;

      //Ensure the rows are in order.
      std::sort(rows.begin(), rows.end());

      //Properties for Stitch1DMany
      std::vector<std::string> workspaceNames;
      std::vector<std::string> runs;

      std::vector<double> params;
      std::vector<double> startOverlaps;
      std::vector<double> endOverlaps;

      //Go through each row and prepare the properties
      for(auto rowIt = rows.begin(); rowIt != rows.end(); ++rowIt)
      {
        const std::string  runStr = m_model->String(*rowIt, COL_RUNS);
        const std::string qMinStr = m_model->String(*rowIt, COL_QMIN);
        const std::string qMaxStr = m_model->String(*rowIt, COL_QMAX);

        double qmin, qmax;
        Mantid::Kernel::Strings::convert<double>(qMinStr, qmin);
        Mantid::Kernel::Strings::convert<double>(qMaxStr, qmax);

        Workspace_sptr runWS = loadRun(runStr);
        if(runWS)
        {
          const std::string runNo = getRunNumber(runWS);
          if(AnalysisDataService::Instance().doesExist("IvsQ_" + runNo))
          {
            runs.push_back(runNo);
            workspaceNames.push_back("IvsQ_" + runNo);
          }
        }

        startOverlaps.push_back(qmin);
        endOverlaps.push_back(qmax);
      }

      double dqq;
      std::string dqqStr = m_model->String(rows.front(), COL_DQQ);
      Mantid::Kernel::Strings::convert<double>(dqqStr, dqq);

      //params are qmin, -dqq, qmax for the final output
      params.push_back(*std::min_element(startOverlaps.begin(), startOverlaps.end()));
      params.push_back(-dqq);
      params.push_back(*std::max_element(endOverlaps.begin(), endOverlaps.end()));

      //startOverlaps and endOverlaps need to be slightly offset from each other
      //See usage examples of Stitch1DMany to see why we discard first qmin and last qmax
      startOverlaps.erase(startOverlaps.begin());
      endOverlaps.pop_back();

      std::string outputWSName = "IvsQ_" + boost::algorithm::join(runs, "_");

      IAlgorithm_sptr algStitch = AlgorithmManager::Instance().create("Stitch1DMany");
      algStitch->initialize();
      algStitch->setProperty("InputWorkspaces", workspaceNames);
      algStitch->setProperty("OutputWorkspace", outputWSName);
      algStitch->setProperty("Params", params);
      algStitch->setProperty("StartOverlaps", startOverlaps);
      algStitch->setProperty("EndOverlaps", endOverlaps);

      algStitch->execute();

      if(!algStitch->isExecuted())
        throw std::runtime_error("Failed to run Stitch1DMany on IvsQ workspaces.");
    }

    /**
    Create a transmission workspace
    @param transString : the numbers of the transmission runs to use
    */
    MatrixWorkspace_sptr ReflMainViewPresenter::makeTransWS(const std::string& transString)
    {
      const size_t maxTransWS = 2;

      std::vector<std::string> transVec;
      std::vector<Workspace_sptr> transWSVec;

      //Take the first two run numbers
      boost::split(transVec, transString, boost::is_any_of(","));
      if(transVec.size() > maxTransWS)
        transVec.resize(maxTransWS);

      if(transVec.size() == 0)
        throw std::runtime_error("Failed to parse the transmission run list.");

      for(auto it = transVec.begin(); it != transVec.end(); ++it)
        transWSVec.push_back(loadRun(*it, m_view->getProcessInstrument()));

      //If the transmission workspace is already in the ADS, re-use it
      std::string lastName = "TRANS_" + boost::algorithm::join(transVec, "_");
      if(AnalysisDataService::Instance().doesExist(lastName))
        return AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(lastName);

      //We have the runs, so we can create a TransWS
      IAlgorithm_sptr algCreateTrans = AlgorithmManager::Instance().create("CreateTransmissionWorkspaceAuto");
      algCreateTrans->initialize();
      algCreateTrans->setProperty("FirstTransmissionRun", transWSVec[0]->name());
      if(transWSVec.size() > 1)
        algCreateTrans->setProperty("SecondTransmissionRun", transWSVec[1]->name());

      std::string wsName = "TRANS_" + getRunNumber(transWSVec[0]);
      if(transWSVec.size() > 1)
        wsName += "_" + getRunNumber(transWSVec[1]);

      algCreateTrans->setProperty("OutputWorkspace", wsName);

      if(!algCreateTrans->isInitialized())
        throw std::runtime_error("Could not initialize CreateTransmissionWorkspaceAuto");

      algCreateTrans->execute();

      if(!algCreateTrans->isExecuted())
        throw std::runtime_error("CreateTransmissionWorkspaceAuto failed to execute");

      return AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);
    }

    /**
    Add row(s) to the model
    */
    void ReflMainViewPresenter::addRow()
    {
      std::vector<size_t> rows = m_view->getSelectedRowIndexes();
      std::sort(rows.begin(), rows.end());

      const int groupId = getUnusedGroup();
      size_t row = 0;

      if(rows.size() == 0)
      {
        //No rows selected, just append a new row
        row = m_model->rowCount();
        m_model->appendRow();
      }
      else
      {
        //One or more rows selected, insert after the last row
        row = m_model->insertRow(*rows.rbegin() + 1);
      }

      //Set the group id of the new row
      m_model->Int(row, COL_GROUP) = groupId;

      //Make sure the view updates
      m_view->showTable(m_model);
    }

    /**
    Delete row(s) from the model
    */
    void ReflMainViewPresenter::deleteRow()
    {
      std::vector<size_t> rows = m_view->getSelectedRowIndexes();
      std::sort(rows.begin(), rows.end());
      for(size_t idx = rows.size(); 0 < idx; --idx)
        m_model->removeRow(rows.at(0));

      m_view->showTable(m_model);
    }

    /**
    Group rows together
    */
    void ReflMainViewPresenter::groupRows()
    {
      const std::vector<size_t> rows = m_view->getSelectedRowIndexes();
      //Find the first unused group id, ignoring the selected rows
      const int groupId = getUnusedGroup(rows);

      //Now we just have to set the group id on the selected rows
      for(auto it = rows.begin(); it != rows.end(); ++it)
        m_model->Int(*it, COL_GROUP) = groupId;

      //Make sure the view updates
      m_view->showTable(m_model);
    }

    /**
    Used by the view to tell the presenter something has changed
    */
    void ReflMainViewPresenter::notify(int flag)
    {
      switch(flag)
      {
      case ReflMainView::SaveAsFlag:    saveAs();     break;
      case ReflMainView::SaveFlag:      save();       break;
      case ReflMainView::AddRowFlag:    addRow();     break;
      case ReflMainView::DeleteRowFlag: deleteRow();  break;
      case ReflMainView::ProcessFlag:   process();    break;
      case ReflMainView::GroupRowsFlag: groupRows();  break;

      case ReflMainView::NoFlags:       return;
      }
      //Not having a 'default' case is deliberate. gcc issues a warning if there's a flag we aren't handling.
    }

    /**
    Load the model into the table
    */
    void ReflMainViewPresenter::load()
    {
      m_view->showTable(m_model);
    }
  }
}
