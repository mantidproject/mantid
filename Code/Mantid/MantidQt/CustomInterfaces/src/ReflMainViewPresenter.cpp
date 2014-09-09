#include "MantidQtCustomInterfaces/ReflMainViewPresenter.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtCustomInterfaces/ReflMainView.h"
#include "MantidAPI/AlgorithmManager.h"
using namespace Mantid::API;

namespace MantidQt
{
  namespace CustomInterfaces
  {

    const int ReflMainViewPresenter::COL_RUNS(0);
    const int ReflMainViewPresenter::COL_ANGLE(1);
    const int ReflMainViewPresenter::COL_TRANSMISSION(2);
    const int ReflMainViewPresenter::COL_QMIN(3);
    const int ReflMainViewPresenter::COL_QMAX(4);
    const int ReflMainViewPresenter::COL_DQQ(5);
    const int ReflMainViewPresenter::COL_SCALE(6);
    const int ReflMainViewPresenter::COL_GROUP(7);

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

      try
      {
        //TODO: Handle groups and stitch them together accordingly
        std::string lastTrans = "";
        for (auto itr = rows.begin(); itr != rows.end(); ++itr)
        {
          lastTrans = processRow(*itr, lastTrans);
        }
        AnalysisDataService::Instance().remove("TransWS");
      }
      catch(std::exception& ex)
      {
        m_view->giveUserCritical("Error encountered while processing: \n" + std::string(ex.what()),"Error");
      }
    }

    /**
    Process a specific Row
    @param rowNo : The row in the model to process
    @param lastTrans : the last transmission runs for comparision to see if a new one needs made
    @returns a string contianing the contents of the transmission runs cell
    */
    std::string ReflMainViewPresenter::processRow(size_t rowNo, std::string lastTrans)
    {
      std::string   run = m_model->String(rowNo, COL_RUNS);
      std::string trans = m_model->String(rowNo, COL_TRANSMISSION);

      double   dqq = 0;
      double theta = 0;
      double  qmin = 0;
      double  qmax = 0;

      const bool   dqqGiven = !m_model->String(rowNo, COL_DQQ  ).empty();
      const bool thetaGiven = !m_model->String(rowNo, COL_ANGLE).empty();
      const bool  qminGiven = !m_model->String(rowNo, COL_QMIN ).empty();
      const bool  qmaxGiven = !m_model->String(rowNo, COL_QMAX ).empty();

      if(dqqGiven)
        Mantid::Kernel::Strings::convert<double>(m_model->String(rowNo, COL_DQQ), dqq);

      if(thetaGiven)
        Mantid::Kernel::Strings::convert<double>(m_model->String(rowNo, COL_ANGLE), theta);

      if(qminGiven)
        Mantid::Kernel::Strings::convert<double>(m_model->String(rowNo, COL_QMIN), qmin);

      if(qmaxGiven)
        Mantid::Kernel::Strings::convert<double>(m_model->String(rowNo, COL_QMAX), qmax);

      //Load the run
      IAlgorithm_sptr algLoadRun = AlgorithmManager::Instance().create("Load");
      algLoadRun->initialize();
      algLoadRun->setProperty("Filename", run);
      algLoadRun->setProperty("OutputWorkspace", run);
      algLoadRun->execute();

      if(algLoadRun->isExecuted())
      {
        if(trans != lastTrans)
          makeTransWS(trans);

        IAlgorithm_sptr algReflOne = AlgorithmManager::Instance().create("ReflectometryReductionOneAuto");
        algReflOne->initialize();
        algReflOne->setProperty("InputWorkspace", AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(run));
        algReflOne->setProperty("FirstTransmissionRun", AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("TransWS"));
        algReflOne->setProperty("OutputWorkspace", run + "_IvsQ");
        algReflOne->setProperty("OutputWorkspaceWaveLength", run + "_IvsLam");
        algReflOne->setProperty("ThetaIn", theta);
        algReflOne->execute();

        if(algReflOne->isExecuted())
        {
          std::vector<double> built_params;
          built_params.push_back(qmin);
          built_params.push_back(-dqq);
          built_params.push_back(qmax);

          IAlgorithm_sptr algRebinQ = AlgorithmManager::Instance().create("Rebin");
          algRebinQ->initialize();
          algRebinQ->setProperty("InputWorkspace", AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(run + "_IvsQ"));
          algRebinQ->setProperty("Params", built_params);
          algRebinQ->setProperty("OutputWorkspace", run + "_IvsQ_binned");
          algRebinQ->execute();

          IAlgorithm_sptr algRebinLam = AlgorithmManager::Instance().create("Rebin");
          algRebinLam->initialize();
          algRebinLam->setProperty("InputWorkspace", AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(run + "_IvsLam"));
          algRebinLam->setProperty("Params", built_params);
          algRebinLam->setProperty("OutputWorkspace", run + "_IvsLam_binned");
          algRebinLam->execute();
        }
      }
      AnalysisDataService::Instance().remove(run);
      return trans;
    }

    /**
    Create a transmission workspace
    @param transString : the numbers of the transmission runs to use
    */
    void ReflMainViewPresenter::makeTransWS(const std::string& transString)
    {
      const size_t maxTransWS = 2;

      std::vector<std::string> transVec;

      //Take the first two run numbers
      boost::split(transVec, transString, boost::is_any_of(","));
      if(transVec.size() > maxTransWS)
        transVec.resize(maxTransWS);

      if(transVec.size() == 0)
        throw std::runtime_error("Failed to parse the transmission run list.");

      size_t numLoaded = 0;
      for(auto it = transVec.begin(); it != transVec.end(); ++it)
      {
        IAlgorithm_sptr algLoadTrans = AlgorithmManager::Instance().create("Load");
        algLoadTrans->initialize();
        algLoadTrans->setProperty("Filename", *it);
        algLoadTrans->setProperty("OutputWorkspace", *it);

        if(!algLoadTrans->isInitialized())
          break;

        algLoadTrans->execute();

        if(!algLoadTrans->isExecuted())
          break;

        numLoaded++;
      }

      if(numLoaded != transVec.size())
        throw std::runtime_error("Failed to load one or more transmission runs. Check the run number and Mantid's data directories are correct.");

      //We have the runs, so we can create a TransWS
      IAlgorithm_sptr algCreateTrans = AlgorithmManager::Instance().create("CreateTransmissionWorkspaceAuto");
      algCreateTrans->initialize();
      algCreateTrans->setProperty("OutputWorkspace", "TransWS");
      algCreateTrans->setProperty("FirstTransmissionRun", transVec[0]);
      if(numLoaded > 1)
        algCreateTrans->setProperty("SecondTransmissionRun", transVec[1]);

      if(!algCreateTrans->isInitialized())
        throw std::runtime_error("Could not initialize CreateTransmissionWorkspaceAuto");

      algCreateTrans->execute();

      if(!algCreateTrans->isExecuted())
        throw std::runtime_error("CreateTransmissionWorkspaceAuto failed to execute");

      //Remove the transmission workspaces we loaded as we no longer need them.
      for(size_t i = 0; i < numLoaded; ++i)
        AnalysisDataService::Instance().remove(transVec[i]);
    }

    /**
    Add row(s) to the model
    */
    void ReflMainViewPresenter::addRow()
    {
      std::vector<size_t> rows = m_view->getSelectedRowIndexes();
      if (rows.size() == 0)
      {
        m_model->appendRow();
      }
      else
      {
        //as selections have to be contigous, then all that needs to be done is add
        //a number of rows at the highest index equal to the size of the returned vector
        std::sort (rows.begin(), rows.end());
        for (size_t idx = rows.size(); 0 < idx; --idx)
        {
          m_model->insertRow(rows.at(0));
        }
      }

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
    Used by the view to tell the presenter something has changed
    */
    void ReflMainViewPresenter::notify()
    {
      //Fetch all the flags in turn, processing them.
      while(m_view->flagSet())
      {
        ReflMainView::Flag flag = m_view->getFlag();
        switch(flag)
        {
        case ReflMainView::SaveAsFlag:    saveAs();     break;
        case ReflMainView::SaveFlag:      save();       break;
        case ReflMainView::AddRowFlag:    addRow();     break;
        case ReflMainView::DeleteRowFlag: deleteRow();  break;
        case ReflMainView::ProcessFlag:   process();    break;

        case ReflMainView::NoFlags:       return;
        }
        //Not having a 'default' case is deliberate. gcc issues a warning if there's a flag we aren't handling.
      }
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
