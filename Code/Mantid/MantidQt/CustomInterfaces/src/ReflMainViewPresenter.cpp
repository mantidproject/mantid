#include "MantidQtCustomInterfaces/ReflMainViewPresenter.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/CatalogManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Utils.h"
#include "MantidQtCustomInterfaces/ReflCatalogSearcher.h"
#include "MantidQtCustomInterfaces/ReflLegacyTransferStrategy.h"
#include "MantidQtCustomInterfaces/ReflMainView.h"
#include "MantidQtCustomInterfaces/ReflSearchModel.h"
#include "MantidQtCustomInterfaces/QReflTableModel.h"
#include "MantidQtCustomInterfaces/QtReflOptionsDialog.h"
#include "MantidQtMantidWidgets/AlgorithmHintStrategy.h"

#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>

#include <QSettings>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace MantidQt::MantidWidgets;

namespace
{
  const QString ReflSettingsGroup = "Mantid/CustomInterfaces/ISISReflectometry";

  void validateModel(ITableWorkspace_sptr model)
  {
    if(!model)
      throw std::runtime_error("Null pointer");

    if(model->columnCount() != 9)
      throw std::runtime_error("Selected table has the incorrect number of columns (9) to be used as a reflectometry table.");

    try
    {
      model->String(0,0);
      model->String(0,1);
      model->String(0,2);
      model->String(0,3);
      model->String(0,4);
      model->String(0,5);
      model->Double(0,6);
      model->Int(0,7);
      model->String(0,8);
    }
    catch(const std::runtime_error&)
    {
      throw std::runtime_error("Selected table does not meet the specifications to become a model for this interface.");
    }
  }

  bool isValidModel(Workspace_sptr model)
  {
    try
    {
      validateModel(boost::dynamic_pointer_cast<ITableWorkspace>(model));
    }
    catch(...)
    {
      return false;
    }
    return true;
  }

  ITableWorkspace_sptr createWorkspace()
  {
    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();
    auto colRuns = ws->addColumn("str","Run(s)");
    auto colTheta = ws->addColumn("str","ThetaIn");
    auto colTrans = ws->addColumn("str","TransRun(s)");
    auto colQmin = ws->addColumn("str","Qmin");
    auto colQmax = ws->addColumn("str","Qmax");
    auto colDqq = ws->addColumn("str","dq/q");
    auto colScale = ws->addColumn("double","Scale");
    auto colStitch = ws->addColumn("int","StitchGroup");
    auto colOptions = ws->addColumn("str","Options");

    colRuns->setPlotType(0);
    colTheta->setPlotType(0);
    colTrans->setPlotType(0);
    colQmin->setPlotType(0);
    colQmax->setPlotType(0);
    colDqq->setPlotType(0);
    colScale->setPlotType(0);
    colStitch->setPlotType(0);
    colOptions->setPlotType(0);

    return ws;
  }

  ITableWorkspace_sptr createDefaultWorkspace()
  {
    //Create a blank workspace with one line and set the scale column to 1
    auto ws = createWorkspace();
    ws->appendRow();
    ws->Double(0, MantidQt::CustomInterfaces::ReflMainViewPresenter::COL_SCALE) = 1.0;
    return ws;
  }
}

namespace MantidQt
{
  namespace CustomInterfaces
  {
    ReflMainViewPresenter::ReflMainViewPresenter(ReflMainView* view, boost::shared_ptr<IReflSearcher> searcher):
      m_view(view),
      m_tableDirty(false),
      m_searcher(searcher),
      m_transferStrategy(new ReflLegacyTransferStrategy()),
      m_addObserver(*this, &ReflMainViewPresenter::handleAddEvent),
      m_remObserver(*this, &ReflMainViewPresenter::handleRemEvent),
      m_clearObserver(*this, &ReflMainViewPresenter::handleClearEvent),
      m_renameObserver(*this, &ReflMainViewPresenter::handleRenameEvent),
      m_replaceObserver(*this, &ReflMainViewPresenter::handleReplaceEvent)
    {
      //Initialise options
      initOptions();

      //Set up the instrument selectors
      std::vector<std::string> instruments;
      instruments.push_back("INTER");
      instruments.push_back("SURF");
      instruments.push_back("CRISP");
      instruments.push_back("POLREF");
      instruments.push_back("OFFSPEC");

      //If the user's configured default instrument is in this list, set it as the default, otherwise use INTER
      const std::string defaultInst = Mantid::Kernel::ConfigService::Instance().getString("default.instrument");
      if(std::find(instruments.begin(), instruments.end(), defaultInst) != instruments.end())
        m_view->setInstrumentList(instruments, defaultInst);
      else
        m_view->setInstrumentList(instruments, "INTER");

      //Populate an initial list of valid tables to open, and subscribe to the ADS to keep it up to date
      Mantid::API::AnalysisDataServiceImpl& ads = Mantid::API::AnalysisDataService::Instance();

      std::set<std::string> items;
      items = ads.getObjectNames();
      for(auto it = items.begin(); it != items.end(); ++it )
      {
        const std::string name = *it;
        Workspace_sptr ws = ads.retrieve(name);

        if(isValidModel(ws))
          m_workspaceList.insert(name);
      }

      ads.notificationCenter.addObserver(m_addObserver);
      ads.notificationCenter.addObserver(m_remObserver);
      ads.notificationCenter.addObserver(m_renameObserver);
      ads.notificationCenter.addObserver(m_clearObserver);
      ads.notificationCenter.addObserver(m_replaceObserver);

      m_view->setTableList(m_workspaceList);

      //Provide autocompletion hints for the options column. We use the algorithm's properties minus
      //those we blacklist. We blacklist any useless properties or ones we're handling that the user
      //should'nt touch.
      IAlgorithm_sptr alg = AlgorithmManager::Instance().create("ReflectometryReductionOneAuto");
      std::set<std::string> blacklist;
      blacklist.insert("ThetaIn");
      blacklist.insert("ThetaOut");
      blacklist.insert("InputWorkspace");
      blacklist.insert("OutputWorkspace");
      blacklist.insert("OutputWorkspaceWavelength");
      blacklist.insert("FirstTransmissionRun");
      blacklist.insert("SecondTransmissionRun");
      m_view->setOptionsHintStrategy(new AlgorithmHintStrategy(alg, blacklist));

      //If we don't have a searcher yet, use ReflCatalogSearcher
      if(!m_searcher)
        m_searcher.reset(new ReflCatalogSearcher());

      //Start with a blank table
      newTable();
    }

    ReflMainViewPresenter::~ReflMainViewPresenter()
    {
      Mantid::API::AnalysisDataServiceImpl& ads = Mantid::API::AnalysisDataService::Instance();
      ads.notificationCenter.removeObserver(m_addObserver);
      ads.notificationCenter.removeObserver(m_remObserver);
      ads.notificationCenter.removeObserver(m_clearObserver);
      ads.notificationCenter.removeObserver(m_renameObserver);
      ads.notificationCenter.removeObserver(m_replaceObserver);
    }

    /**
     * Finds the first unused group id
     */
    int ReflMainViewPresenter::getUnusedGroup(std::set<int> ignoredRows) const
    {
      std::set<int> usedGroups;

      //Scan through all the rows, working out which group ids are used
      for(int idx = 0; idx < m_model->rowCount(); ++idx)
      {
        if(ignoredRows.find(idx) != ignoredRows.end())
          continue;

        //This is an unselected row. Add it to the list of used group ids
        usedGroups.insert(m_model->data(m_model->index(idx, COL_GROUP)).toInt());
      }

      int groupId = 0;

      //While the group id is one of the used ones, increment it by 1
      while(usedGroups.find(groupId) != usedGroups.end())
        groupId++;

      return groupId;
    }

    /**
    Parses a string in the format `a = 1,b=2, c = "1,2,3,4", d = 5.0, e='a,b,c'` into a map of key/value pairs
    @param str The input string
    @throws std::runtime_error on an invalid input string
    */
    std::map<std::string,std::string> ReflMainViewPresenter::parseKeyValueString(const std::string& str)
    {
      //Tokenise, using '\' as an escape character, ',' as a delimiter and " and ' as quote characters
      boost::tokenizer<boost::escaped_list_separator<char> > tok(str, boost::escaped_list_separator<char>("\\", ",", "\"'"));

      std::map<std::string,std::string> kvp;

      for(auto it = tok.begin(); it != tok.end(); ++it)
      {
        std::vector<std::string> valVec;
        boost::split(valVec, *it, boost::is_any_of("="));

        if(valVec.size() > 1)
        {
          //We split on all '='s. The first delimits the key, the rest are assumed to be part of the value
          std::string key = valVec[0];
          //Drop the key from the values vector
          valVec.erase(valVec.begin());
          //Join the remaining sections,
          std::string value = boost::algorithm::join(valVec, "=");

          //Remove any unwanted whitespace
          boost::trim(key);
          boost::trim(value);

          if(key.empty() || value.empty())
            throw std::runtime_error("Invalid key value pair, '" + *it + "'");


          kvp[key] = value;
        }
        else
        {
          throw std::runtime_error("Invalid key value pair, '" + *it + "'");
        }
      }
      return kvp;
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

      std::set<int> rows = m_view->getSelectedRows();
      if(rows.empty())
      {
        if(m_options["WarnProcessAll"].toBool())
        {
          //Does the user want to abort?
          if(!m_view->askUserYesNo("This will process all rows in the table. Continue?","Process all rows?"))
            return;
        }

        //They want to process all rows, so populate rows with every index in the model
        for(int idx = 0; idx < m_model->rowCount(); ++idx)
          rows.insert(idx);
      }

      //Map group numbers to the set of rows in that group we want to process
      std::map<int,std::set<int> > groups;
      for(auto it = rows.begin(); it != rows.end(); ++it)
        groups[m_model->data(m_model->index(*it, COL_GROUP)).toInt()].insert(*it);

      //Check each group and warn if we're only partially processing it
      for(auto gIt = groups.begin(); gIt != groups.end(); ++gIt)
      {
        const int& groupId = gIt->first;
        const std::set<int>& groupRows = gIt->second;
        //Are we only partially processing a group?
        if(groupRows.size() < numRowsInGroup(gIt->first) && m_options["WarnProcessPartialGroup"].toBool())
        {
          std::stringstream err;
          err << "You have only selected " << groupRows.size() << " of the ";
          err << numRowsInGroup(groupId) << " rows in group " << groupId << ".";
          err << " Are you sure you want to continue?";
          if(!m_view->askUserYesNo(err.str(), "Continue Processing?"))
            return;
        }
      }

      //Validate the rows
      for(auto it = rows.begin(); it != rows.end(); ++it)
      {
        try
        {
          validateRow(*it);
          autofillRow(*it);
        }
        catch(std::exception& ex)
        {
          //Allow two theta to be blank
          if(ex.what() == std::string("Value for two theta could not be found in log."))
            continue;

          const std::string rowNo = Mantid::Kernel::Strings::toString<int>(*it + 1);
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
        const std::set<int> groupRows = gIt->second;

        //Reduce each row
        for(auto rIt = groupRows.begin(); rIt != groupRows.end(); ++rIt)
        {
          try
          {
            reduceRow(*rIt);
            m_view->setProgress(++progress);
          }
          catch(std::exception& ex)
          {
            const std::string rowNo = Mantid::Kernel::Strings::toString<int>(*rIt + 1);
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
    void ReflMainViewPresenter::validateRow(int rowNo) const
    {
      if(rowNo >= m_model->rowCount())
        throw std::invalid_argument("Invalid row");

      if(m_model->data(m_model->index(rowNo, COL_RUNS)).toString().isEmpty())
        throw std::invalid_argument("Run column may not be empty.");
    }

    /**
    Autofill a row
    @param rowNo : The row in the model to autofill
    @throws std::runtime_error if the row could not be auto-filled
    */
    void ReflMainViewPresenter::autofillRow(int rowNo)
    {
      if(rowNo >= m_model->rowCount())
        throw std::runtime_error("Invalid row");

      const std::string runStr = m_model->data(m_model->index(rowNo, COL_RUNS)).toString().toStdString();
      auto runWS = prepareRunWorkspace(runStr);
      auto runMWS = boost::dynamic_pointer_cast<MatrixWorkspace>(runWS);
      auto runWSG = boost::dynamic_pointer_cast<WorkspaceGroup>(runWS);

      //If we've got a workspace group, use the first workspace in it
      if(!runMWS && runWSG)
        runMWS = boost::dynamic_pointer_cast<MatrixWorkspace>(runWSG->getItem(0));

      if(!runMWS)
        throw std::runtime_error("Could not convert " + runWS->name() + " to a MatrixWorkspace.");

      //Fetch two theta from the log if needed
      if(m_model->data(m_model->index(rowNo, COL_ANGLE)).toString().isEmpty())
      {
        Property* logData = NULL;

        //First try TwoTheta
        try
        {
          logData = runMWS->mutableRun().getLogData("Theta");
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
        if(m_options["RoundAngle"].toBool())
          thetaVal = Utils::roundToDP(thetaVal, m_options["RoundAnglePrecision"].toInt());

        m_model->setData(m_model->index(rowNo, COL_ANGLE), thetaVal);
        m_tableDirty = true;
      }

      //If we need to calculate the resolution, do.
      if(m_model->data(m_model->index(rowNo, COL_DQQ)).toString().isEmpty())
      {
        IAlgorithm_sptr calcResAlg = AlgorithmManager::Instance().create("CalculateResolution");
        calcResAlg->setProperty("Workspace", runMWS);
        calcResAlg->setProperty("TwoTheta", m_model->data(m_model->index(rowNo, COL_ANGLE)).toString().toStdString());
        calcResAlg->execute();

        if(!calcResAlg->isExecuted())
          throw std::runtime_error("CalculateResolution failed. Please manually enter a value in the dQ/Q column.");

        //Update the model
        double dqqVal = calcResAlg->getProperty("Resolution");

        if(m_options["RoundDQQ"].toBool())
          dqqVal = Utils::roundToDP(dqqVal, m_options["RoundDQQPrecision"].toInt());

        m_model->setData(m_model->index(rowNo, COL_DQQ), dqqVal);
        m_tableDirty = true;
      }
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
    Takes a user specified run, or list of runs, and returns a pointer to the desired TOF workspace
    @param runStr : The run or list of runs (separated by '+')
    @throws std::runtime_error if the workspace could not be prepared
    @returns a shared pointer to the workspace
    */
    Workspace_sptr ReflMainViewPresenter::prepareRunWorkspace(const std::string& runStr)
    {
      const std::string instrument = m_view->getProcessInstrument();

      std::vector<std::string> runs;
      boost::split(runs, runStr, boost::is_any_of("+"));

      if(runs.empty())
        throw std::runtime_error("No runs given");

      //Remove leading/trailing whitespace from each run
      for(auto runIt = runs.begin(); runIt != runs.end(); ++runIt)
        boost::trim(*runIt);

      //If we're only given one run, just return that
      if(runs.size() == 1)
        return loadRun(runs[0], instrument);

      const std::string outputName = "TOF_" + boost::algorithm::join(runs, "_");

      //Check if we've already prepared it
      if(AnalysisDataService::Instance().doesExist(outputName))
        return AnalysisDataService::Instance().retrieveWS<Workspace>(outputName);


      /* Ideally, this should be executed as a child algorithm to keep the ADS tidy, but
       * that doesn't preserve history nicely, so we'll just take care of tidying up in
       * the event of failure.
       */
      IAlgorithm_sptr algPlus = AlgorithmManager::Instance().create("Plus");
      algPlus->initialize();
      algPlus->setProperty("LHSWorkspace", loadRun(runs[0], instrument)->name());
      algPlus->setProperty("OutputWorkspace", outputName);

      //Drop the first run from the runs list
      runs.erase(runs.begin());

      try
      {
        //Iterate through all the remaining runs, adding them to the first run
        for(auto runIt = runs.begin(); runIt != runs.end(); ++runIt)
        {
          algPlus->setProperty("RHSWorkspace", loadRun(*runIt, instrument)->name());
          algPlus->execute();

          //After the first execution we replace the LHS with the previous output
          algPlus->setProperty("LHSWorkspace", outputName);
        }
      }
      catch(...)
      {
        //If we're unable to create the full workspace, discard the partial version
        AnalysisDataService::Instance().remove(outputName);

        //We've tidied up, now re-throw.
        throw;
      }

      return AnalysisDataService::Instance().retrieveWS<Workspace>(outputName);
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

        //Look for "TOF_<run_number>" in the ADS
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
    Reduce a row
    @param rowNo : The row in the model to reduce
    @throws std::runtime_error if reduction fails
    */
    void ReflMainViewPresenter::reduceRow(int rowNo)
    {
      const std::string   runStr = m_model->data(m_model->index(rowNo, COL_RUNS)).toString().toStdString();
      const std::string transStr = m_model->data(m_model->index(rowNo, COL_TRANSMISSION)).toString().toStdString();
      const std::string  options = m_model->data(m_model->index(rowNo, COL_OPTIONS)).toString().toStdString();

      double theta = 0;

      bool thetaGiven = !m_model->data(m_model->index(rowNo, COL_ANGLE)).toString().isEmpty();

      if(thetaGiven)
        theta = m_model->data(m_model->index(rowNo, COL_ANGLE)).toDouble();

      auto runWS = prepareRunWorkspace(runStr);
      const std::string runNo = getRunNumber(runWS);

      Workspace_sptr transWS;
      if(!transStr.empty())
        transWS = makeTransWS(transStr);

      IAlgorithm_sptr algReflOne = AlgorithmManager::Instance().create("ReflectometryReductionOneAuto");
      algReflOne->initialize();
      algReflOne->setProperty("InputWorkspace", runWS->name());
      if(transWS)
        algReflOne->setProperty("FirstTransmissionRun", transWS->name());
      algReflOne->setProperty("OutputWorkspace", "IvsQ_" + runNo);
      algReflOne->setProperty("OutputWorkspaceWaveLength", "IvsLam_" + runNo);

      if(thetaGiven)
        algReflOne->setProperty("ThetaIn", theta);

      //Parse and set any user-specified options
      auto optionsMap = parseKeyValueString(options);
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

      const double scale = m_model->data(m_model->index(rowNo, COL_SCALE)).toDouble();
      if(scale != 1.0)
      {
        IAlgorithm_sptr algScale = AlgorithmManager::Instance().create("Scale");
        algScale->initialize();
        algScale->setProperty("InputWorkspace", "IvsQ_" + runNo);
        algScale->setProperty("OutputWorkspace", "IvsQ_" + runNo);
        algScale->setProperty("Factor", 1.0 / scale);
        algScale->execute();

        if(!algScale->isExecuted())
          throw std::runtime_error("Failed to run Scale algorithm");
      }

      //Reduction has completed. Put Qmin and Qmax into the table if needed, for stitching.
      if(m_model->data(m_model->index(rowNo, COL_QMIN)).toString().isEmpty() || m_model->data(m_model->index(rowNo, COL_QMAX)).toString().isEmpty())
      {
        Workspace_sptr ws = AnalysisDataService::Instance().retrieveWS<Workspace>("IvsQ_" + runNo);
        std::vector<double> qrange = calcQRange(ws, theta);

        if(m_model->data(m_model->index(rowNo, COL_QMIN)).toString().isEmpty())
          m_model->setData(m_model->index(rowNo, COL_QMIN), qrange[0]);

        if(m_model->data(m_model->index(rowNo, COL_QMAX)).toString().isEmpty())
          m_model->setData(m_model->index(rowNo, COL_QMAX), qrange[1]);

        m_tableDirty = true;
      }

      //Also fill in theta if needed
      if(m_model->data(m_model->index(rowNo, COL_ANGLE)).toString().isEmpty() && thetaGiven)
        m_model->setData(m_model->index(rowNo, COL_ANGLE), theta);
    }

    /**
    Calculates the minimum and maximum values for Q
    @param ws : The workspace to fetch the instrument values from
    @param theta : The value of two theta to use in calculations
    */
    std::vector<double> ReflMainViewPresenter::calcQRange(Workspace_sptr ws, double theta)
    {
      auto mws = boost::dynamic_pointer_cast<MatrixWorkspace>(ws);
      auto wsg = boost::dynamic_pointer_cast<WorkspaceGroup>(ws);

      //If we've got a workspace group, use the first workspace in it
      if(!mws && wsg)
        mws = boost::dynamic_pointer_cast<MatrixWorkspace>(wsg->getItem(0));

      if(!mws)
        throw std::runtime_error("Could not convert " + ws->name() + " to a MatrixWorkspace.");

      double lmin, lmax;
      try
      {
        const Instrument_const_sptr instrument = mws->getInstrument();
        lmin = instrument->getNumberParameter("LambdaMin")[0];
        lmax = instrument->getNumberParameter("LambdaMax")[0];
      }
      catch(std::exception&)
      {
        throw std::runtime_error("LambdaMin/LambdaMax instrument parameters are required to calculate qmin/qmax");
      }

      double qmin = 4 * M_PI / lmax * sin(theta * M_PI / 180.0);
      double qmax = 4 * M_PI / lmin * sin(theta * M_PI / 180.0);

      if(m_options["RoundQMin"].toBool())
        qmin = Utils::roundToDP(qmin, m_options["RoundQMinPrecision"].toInt());

      if(m_options["RoundQMax"].toBool())
        qmax = Utils::roundToDP(qmax, m_options["RoundQMaxPrecision"].toInt());

      std::vector<double> ret;
      ret.push_back(qmin);
      ret.push_back(qmax);
      return ret;
    }

    /**
    Stitches the workspaces created by the given rows together.
    @param rows : the list of rows
    */
    void ReflMainViewPresenter::stitchRows(std::set<int> rows)
    {
      //If we can get away with doing nothing, do.
      if(rows.size() < 2)
        return;

      //Properties for Stitch1DMany
      std::vector<std::string> workspaceNames;
      std::vector<std::string> runs;

      std::vector<double> params;
      std::vector<double> startOverlaps;
      std::vector<double> endOverlaps;

      //Go through each row and prepare the properties
      for(auto rowIt = rows.begin(); rowIt != rows.end(); ++rowIt)
      {
        const std::string  runStr = m_model->data(m_model->index(*rowIt, COL_RUNS)).toString().toStdString();
        const double         qmin = m_model->data(m_model->index(*rowIt, COL_QMIN)).toDouble();
        const double         qmax = m_model->data(m_model->index(*rowIt, COL_QMAX)).toDouble();

        Workspace_sptr runWS = prepareRunWorkspace(runStr);
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

      double dqq = m_model->data(m_model->index(*(rows.begin()), COL_DQQ)).toDouble();

      //params are qmin, -dqq, qmax for the final output
      params.push_back(*std::min_element(startOverlaps.begin(), startOverlaps.end()));
      params.push_back(-dqq);
      params.push_back(*std::max_element(endOverlaps.begin(), endOverlaps.end()));

      //startOverlaps and endOverlaps need to be slightly offset from each other
      //See usage examples of Stitch1DMany to see why we discard first qmin and last qmax
      startOverlaps.erase(startOverlaps.begin());
      endOverlaps.pop_back();

      std::string outputWSName = "IvsQ_" + boost::algorithm::join(runs, "_");

      //If the previous stitch result is in the ADS already, we'll need to remove it.
      //If it's a group, we'll get an error for trying to group into a used group name
      if(AnalysisDataService::Instance().doesExist(outputWSName))
        AnalysisDataService::Instance().remove(outputWSName);

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
    Workspace_sptr ReflMainViewPresenter::makeTransWS(const std::string& transString)
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
        return AnalysisDataService::Instance().retrieveWS<Workspace>(lastName);

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

      return AnalysisDataService::Instance().retrieveWS<Workspace>(wsName);
    }

    /**
    Inserts a new row in the specified location
    @param index The index to insert the new row before
    */
    void ReflMainViewPresenter::insertRow(int index)
    {
      const int groupId = getUnusedGroup();
      if(!m_model->insertRow(index))
        return;
      //Set the default scale to 1.0
      m_model->setData(m_model->index(index, COL_SCALE), 1.0);
      //Set the group id of the new row
      m_model->setData(m_model->index(index, COL_GROUP), groupId);
    }

    /**
    Insert a row after the last selected row
    */
    void ReflMainViewPresenter::appendRow()
    {
      std::set<int> rows = m_view->getSelectedRows();
      if(rows.empty())
        insertRow(m_model->rowCount());
      else
        insertRow(*rows.rbegin() + 1);
      m_tableDirty = true;
    }

    /**
    Insert a row before the first selected row
    */
    void ReflMainViewPresenter::prependRow()
    {
      std::set<int> rows = m_view->getSelectedRows();
      if(rows.empty())
        insertRow(0);
      else
        insertRow(*rows.begin());
      m_tableDirty = true;
    }

    /**
    Delete row(s) from the model
    */
    void ReflMainViewPresenter::deleteRow()
    {
      std::set<int> rows = m_view->getSelectedRows();
      for(auto row = rows.rbegin(); row != rows.rend(); ++row)
        m_model->removeRow(*row);

      m_tableDirty = true;
    }

    /**
    Group rows together
    */
    void ReflMainViewPresenter::groupRows()
    {
      const std::set<int> rows = m_view->getSelectedRows();
      //Find the first unused group id, ignoring the selected rows
      const int groupId = getUnusedGroup(rows);

      //Now we just have to set the group id on the selected rows
      for(auto it = rows.begin(); it != rows.end(); ++it)
        m_model->setData(m_model->index(*it, COL_GROUP), groupId);

      m_tableDirty = true;
    }

    /**
    Used by the view to tell the presenter something has changed
    */
    void ReflMainViewPresenter::notify(IReflPresenter::Flag flag)
    {
      switch(flag)
      {
      case IReflPresenter::SaveAsFlag:          saveTableAs();       break;
      case IReflPresenter::SaveFlag:            saveTable();         break;
      case IReflPresenter::AppendRowFlag:       appendRow();         break;
      case IReflPresenter::PrependRowFlag:      prependRow();        break;
      case IReflPresenter::DeleteRowFlag:       deleteRow();         break;
      case IReflPresenter::ProcessFlag:         process();           break;
      case IReflPresenter::GroupRowsFlag:       groupRows();         break;
      case IReflPresenter::OpenTableFlag:       openTable();         break;
      case IReflPresenter::NewTableFlag:        newTable();          break;
      case IReflPresenter::TableUpdatedFlag:    m_tableDirty = true; break;
      case IReflPresenter::ExpandSelectionFlag: expandSelection();   break;
      case IReflPresenter::OptionsDialogFlag:   showOptionsDialog(); break;
      case IReflPresenter::ClearSelectedFlag:   clearSelected();     break;
      case IReflPresenter::CopySelectedFlag:    copySelected();      break;
      case IReflPresenter::CutSelectedFlag:     cutSelected();       break;
      case IReflPresenter::PasteSelectedFlag:   pasteSelected();     break;
      case IReflPresenter::SearchFlag:          search();            break;
      case IReflPresenter::TransferFlag:        transfer();          break;
      case IReflPresenter::ImportTableFlag:     importTable();       break;
      case IReflPresenter::ExportTableFlag:     exportTable();       break;
      case IReflPresenter::PlotRowFlag:         plotRow();           break;
      case IReflPresenter::PlotGroupFlag:       plotGroup();         break;
      }
      //Not having a 'default' case is deliberate. gcc issues a warning if there's a flag we aren't handling.
    }

    /**
    Press changes to the same item in the ADS
    */
    void ReflMainViewPresenter::saveTable()
    {
      if(!m_wsName.empty())
      {
        AnalysisDataService::Instance().addOrReplace(m_wsName,boost::shared_ptr<ITableWorkspace>(m_ws->clone()));
        m_tableDirty = false;
      }
      else
      {
        saveTableAs();
      }
    }

    /**
    Press changes to a new item in the ADS
    */
    void ReflMainViewPresenter::saveTableAs()
    {
      const std::string userString = m_view->askUserString("Save As", "Enter a workspace name:", "Workspace");
      if(!userString.empty())
      {
        m_wsName = userString;
        saveTable();
      }
    }

    /**
    Start a new, untitled table
    */
    void ReflMainViewPresenter::newTable()
    {
      if(m_tableDirty && m_options["WarnDiscardChanges"].toBool())
        if(!m_view->askUserYesNo("Your current table has unsaved changes. Are you sure you want to discard them?","Start New Table?"))
          return;

      m_ws = createDefaultWorkspace();
      m_model.reset(new QReflTableModel(m_ws));
      m_wsName.clear();
      m_view->showTable(m_model);

      m_tableDirty = false;
    }

    /**
    Open a table from the ADS
    */
    void ReflMainViewPresenter::openTable()
    {
      if(m_tableDirty && m_options["WarnDiscardChanges"].toBool())
        if(!m_view->askUserYesNo("Your current table has unsaved changes. Are you sure you want to discard them?","Open Table?"))
          return;

      auto& ads = AnalysisDataService::Instance();
      const std::string toOpen = m_view->getWorkspaceToOpen();

      if(toOpen.empty())
        return;

      if(!ads.isValid(toOpen).empty())
      {
        m_view->giveUserCritical("Could not open workspace: " + toOpen, "Error");
        return;
      }

      ITableWorkspace_sptr origTable = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(toOpen);

      //We create a clone of the table for live editing. The original is not updated unless we explicitly save.
      ITableWorkspace_sptr newTable = boost::shared_ptr<ITableWorkspace>(origTable->clone());
      try
      {
        validateModel(newTable);
        m_ws = newTable;
        m_model.reset(new QReflTableModel(m_ws));
        m_wsName = toOpen;
        m_view->showTable(m_model);
        m_tableDirty = false;
      }
      catch(std::runtime_error& e)
      {
        m_view->giveUserCritical("Could not open workspace: " + std::string(e.what()), "Error");
      }
    }

    /**
    Import a table from TBL file
    */
    void ReflMainViewPresenter::importTable()
    {
      m_view->showAlgorithmDialog("LoadReflTBL");
    }

    /**
    Export a table to TBL file
    */
    void ReflMainViewPresenter::exportTable()
    {
      m_view->showAlgorithmDialog("SaveReflTBL");
    }

    /**
    Handle ADS add events
    */
    void ReflMainViewPresenter::handleAddEvent(Mantid::API::WorkspaceAddNotification_ptr pNf)
    {
      const std::string name = pNf->objectName();

      if(Mantid::API::AnalysisDataService::Instance().isHiddenDataServiceObject(name))
        return;

      if(!isValidModel(pNf->object()))
        return;

      m_workspaceList.insert(name);
      m_view->setTableList(m_workspaceList);
    }

    /**
    Handle ADS remove events
    */
    void ReflMainViewPresenter::handleRemEvent(Mantid::API::WorkspacePostDeleteNotification_ptr pNf)
    {
      const std::string name = pNf->objectName();
      m_workspaceList.erase(name);
      m_view->setTableList(m_workspaceList);
    }

    /**
    Handle ADS clear events
    */
    void ReflMainViewPresenter::handleClearEvent(Mantid::API::ClearADSNotification_ptr)
    {
      m_workspaceList.clear();
      m_view->setTableList(m_workspaceList);
    }

    /**
    Handle ADS rename events
    */
    void ReflMainViewPresenter::handleRenameEvent(Mantid::API::WorkspaceRenameNotification_ptr pNf)
    {
      //If we have this workspace, rename it
      const std::string name = pNf->objectName();
      const std::string newName = pNf->newObjectName();

      if(m_workspaceList.find(name) == m_workspaceList.end())
        return;

      m_workspaceList.erase(name);
      m_workspaceList.insert(newName);
      m_view->setTableList(m_workspaceList);
    }

    /**
    Handle ADS replace events
    */
    void ReflMainViewPresenter::handleReplaceEvent(Mantid::API::WorkspaceAfterReplaceNotification_ptr pNf)
    {
      const std::string name = pNf->objectName();
      //Erase it
      m_workspaceList.erase(name);

      //If it's a table workspace, bring it back
      if(isValidModel(pNf->object()))
        m_workspaceList.insert(name);

      m_view->setTableList(m_workspaceList);
    }

    /** Returns how many rows there are in a given group
        @param groupId : The id of the group to count the rows of
        @returns The number of rows in the group
     */
    size_t ReflMainViewPresenter::numRowsInGroup(int groupId) const
    {
      size_t count = 0;
      for(int i = 0; i < m_model->rowCount(); ++i)
        if(m_model->data(m_model->index(i, COL_GROUP)).toInt() == groupId)
          count++;
      return count;
    }

    /** Expands the current selection to all the rows in the selected groups */
    void ReflMainViewPresenter::expandSelection()
    {
      std::set<int> groupIds;

      std::set<int> rows = m_view->getSelectedRows();
      for(auto row = rows.begin(); row != rows.end(); ++row)
        groupIds.insert(m_model->data(m_model->index(*row, COL_GROUP)).toInt());

      std::set<int> selection;

      for(int i = 0; i < m_model->rowCount(); ++i)
        if(groupIds.find(m_model->data(m_model->index(i, COL_GROUP)).toInt()) != groupIds.end())
          selection.insert(i);

      m_view->setSelection(selection);
    }

    /** Clear the currently selected rows */
    void ReflMainViewPresenter::clearSelected()
    {
      std::set<int> rows = m_view->getSelectedRows();
      std::set<int> ignore;
      for(auto row = rows.begin(); row != rows.end(); ++row)
      {
        ignore.clear();
        ignore.insert(*row);

        m_model->setData(m_model->index(*row, COL_RUNS), "");
        m_model->setData(m_model->index(*row, COL_ANGLE), "");
        m_model->setData(m_model->index(*row, COL_TRANSMISSION), "");
        m_model->setData(m_model->index(*row, COL_QMIN), "");
        m_model->setData(m_model->index(*row, COL_QMAX), "");
        m_model->setData(m_model->index(*row, COL_SCALE), 1.0);
        m_model->setData(m_model->index(*row, COL_DQQ), "");
        m_model->setData(m_model->index(*row, COL_GROUP), getUnusedGroup(ignore));
        m_model->setData(m_model->index(*row, COL_OPTIONS), "");
      }
      m_tableDirty = true;
    }

    /** Copy the currently selected rows to the clipboard */
    void ReflMainViewPresenter::copySelected()
    {
      std::vector<std::string> lines;

      std::set<int> rows = m_view->getSelectedRows();
      for(auto rowIt = rows.begin(); rowIt != rows.end(); ++rowIt)
      {
        std::vector<std::string> line;
        for(int col = COL_RUNS; col <= COL_OPTIONS; ++col)
          line.push_back(m_model->data(m_model->index(*rowIt, col)).toString().toStdString());
        lines.push_back(boost::algorithm::join(line, "\t"));
      }

      m_view->setClipboard(boost::algorithm::join(lines, "\n"));
    }

    /** Copy currently selected rows to the clipboard, and then delete them. */
    void ReflMainViewPresenter::cutSelected()
    {
      copySelected();
      deleteRow();
    }

    /** Paste the contents of the clipboard into the currently selected rows, or append new rows */
    void ReflMainViewPresenter::pasteSelected()
    {
      const std::string text = m_view->getClipboard();
      std::vector<std::string> lines;
      boost::split(lines, text, boost::is_any_of("\n"));

      //If we have rows selected, we'll overwrite them. If not, we'll append new rows to write to.
      std::set<int> rows = m_view->getSelectedRows();
      if(rows.empty())
      {
        //Add as many new rows as required
        for(size_t i = 0; i < lines.size(); ++i)
        {
          int index = m_model->rowCount();
          insertRow(index);
          rows.insert(index);
        }
      }

      //Iterate over rows and lines simultaneously, stopping when we reach the end of either
      auto rowIt = rows.begin();
      auto lineIt = lines.begin();
      for(; rowIt != rows.end() && lineIt != lines.end(); rowIt++, lineIt++)
      {
        std::vector<std::string> values;
        boost::split(values, *lineIt, boost::is_any_of("\t"));

        //Paste as many columns as we can from this line
        for(int col = COL_RUNS; col <= COL_OPTIONS && col < static_cast<int>(values.size()); ++col)
          m_model->setData(m_model->index(*rowIt, col), QString::fromStdString(values[col]));
      }
    }

    /** Searches for runs that can be used */
    void ReflMainViewPresenter::search()
    {
      const std::string searchString = m_view->getSearchString();
      const std::string searchInstr  = m_view->getSearchInstrument();

      //Don't bother searching if they're not searching for anything
      if(searchString.empty())
        return;

      //This is breaking the abstraction provided by IReflSearcher, but provides a nice usability win
      //If we're not logged into a catalog, prompt the user to do so
      if(CatalogManager::Instance().getActiveSessions().empty())
        m_view->showAlgorithmDialog("CatalogLogin");

      try
      {
        auto results = m_searcher->search(searchString, searchInstr);
        m_searchModel = ReflSearchModel_sptr(new ReflSearchModel(results));
        m_view->showSearch(m_searchModel);
      }
      catch(std::runtime_error& e)
      {
        m_view->giveUserCritical("Error running search:\n" + std::string(e.what()), "Search Failed");
      }
    }

    /** Transfers the selected runs in the search results to the processing table */
    void ReflMainViewPresenter::transfer()
    {
      //Build the input for the transfer strategy
      std::map<std::string,std::string> runs;
      auto selectedRows = m_view->getSelectedSearchRows();
      for(auto rowIt = selectedRows.begin(); rowIt != selectedRows.end(); ++rowIt)
      {
        const int row = *rowIt;
        const std::string run = m_searchModel->data(m_searchModel->index(row, 0)).toString().toStdString();
        const std::string description = m_searchModel->data(m_searchModel->index(row, 1)).toString().toStdString();
        runs[run] = description;
      }

      auto newRows = m_transferStrategy->transferRuns(runs);

      std::map<std::string,int> groups;
      for(auto rowIt = newRows.begin(); rowIt != newRows.end(); ++rowIt)
      {
        auto& row = *rowIt;

        if(groups.count(row["group"]) == 0)
          groups[row["group"]] = getUnusedGroup();

        const int rowIndex = m_model->rowCount();
        m_model->insertRow(rowIndex);
        m_model->setData(m_model->index(rowIndex, COL_RUNS), QString::fromStdString(row["runs"]));
        m_model->setData(m_model->index(rowIndex, COL_ANGLE), QString::fromStdString(row["theta"]));
        m_model->setData(m_model->index(rowIndex, COL_SCALE), 1.0);
        m_model->setData(m_model->index(rowIndex, COL_GROUP), groups[row["group"]]);
      }
    }

    /** Plots any currently selected rows */
    void ReflMainViewPresenter::plotRow()
    {
      auto selectedRows = m_view->getSelectedRows();

      if(selectedRows.empty())
        return;

      std::set<std::string> workspaces, notFound;
      for(auto row = selectedRows.begin(); row != selectedRows.end(); ++row)
      {
        const std::string wsName = "IvsQ_" + getRunNumber(prepareRunWorkspace(m_model->data(m_model->index(*row, COL_RUNS)).toString().toStdString()));
        if(AnalysisDataService::Instance().doesExist(wsName))
          workspaces.insert(wsName);
        else
          notFound.insert(wsName);
      }

      if(!notFound.empty())
        m_view->giveUserWarning(
            "The following workspaces were not plotted because they were not found:\n"
            + boost::algorithm::join(notFound, "\n") +
            "\n\nPlease check that the rows you are trying to plot have been fully processed.",
            "Error plotting rows.");

      m_view->plotWorkspaces(workspaces);
    }

    /** Plots any currently selected groups */
    void ReflMainViewPresenter::plotGroup()
    {
      auto selectedRows = m_view->getSelectedRows();

      if(selectedRows.empty())
        return;

      std::set<int> selectedGroups;
      for(auto row = selectedRows.begin(); row != selectedRows.end(); ++row)
        selectedGroups.insert(m_model->data(m_model->index(*row, COL_GROUP)).toInt());

      //Now, get the names of the stitched workspace, one per group
      std::map<int,std::vector<std::string>> runsByGroup;
      const int numRows = m_model->rowCount();
      for(int row = 0; row < numRows; ++row)
      {
        int group = m_model->data(m_model->index(row, COL_GROUP)).toInt();

        //Skip groups we don't care about
        if(selectedGroups.find(group) == selectedGroups.end())
          continue;

        //Add this to the list of runs
        runsByGroup[group].push_back(getRunNumber(prepareRunWorkspace(m_model->data(m_model->index(row, COL_RUNS)).toString().toStdString())));
      }

      std::set<std::string> workspaces, notFound;
      for(auto runsMap = runsByGroup.begin(); runsMap != runsByGroup.end(); ++runsMap)
      {
        const std::string wsName = "IvsQ_" + boost::algorithm::join(runsMap->second, "_");
        if(AnalysisDataService::Instance().doesExist(wsName))
          workspaces.insert(wsName);
        else
          notFound.insert(wsName);
      }

      if(!notFound.empty())
        m_view->giveUserWarning(
            "The following workspaces were not plotted because they were not found:\n"
            + boost::algorithm::join(notFound, "\n") +
            "\n\nPlease check that the groups you are trying to plot have been fully processed.",
            "Error plotting groups.");

      m_view->plotWorkspaces(workspaces);
    }

    /** Shows the Refl Options dialog */
    void ReflMainViewPresenter::showOptionsDialog()
    {
      auto options = new QtReflOptionsDialog(m_view, m_view->getPresenter());
      //By default the dialog is only destroyed when ReflMainView is and so they'll stack up.
      //This way, they'll be deallocated as soon as they've been closed.
      options->setAttribute(Qt::WA_DeleteOnClose, true);
      options->exec();
    }

    /** Gets the options used by the presenter
        @returns The options used by the presenter
     */
    const std::map<std::string,QVariant>& ReflMainViewPresenter::options() const
    {
      return m_options;
    }

    /** Sets the options used by the presenter
        @param options : The new options for the presenter to use
     */
    void ReflMainViewPresenter::setOptions(const std::map<std::string,QVariant>& options)
    {
      //Overwrite the given options
      for(auto it = options.begin(); it != options.end(); ++it)
        m_options[it->first] = it->second;

      //Save any changes to disk
      QSettings settings;
      settings.beginGroup(ReflSettingsGroup);
      for(auto it = m_options.begin(); it != m_options.end(); ++it)
        settings.setValue(QString::fromStdString(it->first), it->second);
      settings.endGroup();
    }

    /** Load options from disk if possible, or set to defaults */
    void ReflMainViewPresenter::initOptions()
    {
      m_options.clear();

      //Set defaults
      m_options["WarnProcessAll"] = true;
      m_options["WarnDiscardChanges"] = true;
      m_options["WarnProcessPartialGroup"] = true;
      m_options["RoundAngle"] = false;
      m_options["RoundQMin"] = false;
      m_options["RoundQMax"] = false;
      m_options["RoundDQQ"] = false;
      m_options["RoundAnglePrecision"] = 3;
      m_options["RoundQMinPrecision"] = 3;
      m_options["RoundQMaxPrecision"] = 3;
      m_options["RoundDQQPrecision"] = 3;

      //Load saved values from disk
      QSettings settings;
      settings.beginGroup(ReflSettingsGroup);
      QStringList keys = settings.childKeys();
      for(auto it = keys.begin(); it != keys.end(); ++it)
        m_options[it->toStdString()] = settings.value(*it);
      settings.endGroup();
    }
  }
}
