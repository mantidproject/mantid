#include "MantidQtCustomInterfaces/ReflMainViewPresenter.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtCustomInterfaces/ReflMainView.h"
#include "MantidAPI/AlgorithmManager.h"
using namespace Mantid::API;

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
    Process selected rows
    */
    void ReflMainViewPresenter::process()
    {
      if (m_model->rowCount() == 0)
      {
        m_view->giveUserWarning("Cannot process an empty Table","Warning");
        return;
      }
      bool willprocess = true;
      std::vector<size_t> rows = m_view->getSelectedRowIndexes();
      if (rows.size() == 0)
      {
        if (m_view->askUserYesNo("This will process all rows in the table. Continue?","Process all rows?"))
        {
          //if so populate rows with every index in the model
          for (size_t idx = 0; idx < m_model->rowCount(); ++idx)
          {
            rows.push_back(idx);
          }
        }
        else
        {
          willprocess = false;
        }
      }
      if (willprocess)
      {
        try
        {
          if (rows.size() == 0)
          {
            //if we're here and rows is empty then somehting is wrong as an empty table should have been checked for but jsut to be safe
            m_view->giveUserWarning("Recieved nothing to process.","Warning");
            return;
          }
          else if (rows.size() == 1)
          {
            //a single row makes things simple, we don't need to look for stitch groups
            size_t rowNo = rows.at(0);
            processRow(rowNo);
          }
          else
          {
            //Not looking fro stick groups in this iteration of the interace, that will come later.
            std::string lastTrans = "";
            for (auto itr = rows.begin(); itr != rows.end(); ++itr)
            {
              lastTrans= processRow(*itr,lastTrans);
            }
          }
          AnalysisDataService::Instance().remove("TransWS");
        }
        catch(std::exception & ex)
        {
          //Contain the exception and notify the user
          m_view->giveUserCritical("Error encountered while processing: \n" + std::string(ex.what()),"Error");
          return;
        }
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
      std::string run = m_model->String(rowNo,0);
      const double theta = boost::lexical_cast<double>(m_model->String(rowNo,1));
      std::string trans = m_model->String(rowNo,2);
      const double qmin = boost::lexical_cast<double>(m_model->String(rowNo,3));
      const double qmax = boost::lexical_cast<double>(m_model->String(rowNo,4));
      const double dqq = boost::lexical_cast<double>(m_model->String(rowNo,5));

      size_t commacheck = run.find_first_of(',');
      if (commacheck != std::string::npos)
      {
        //If there are multiple runs, just grab the first
        run = run.substr(0, commacheck);
      }
      //Load the run
      IAlgorithm_sptr algLoadRun = AlgorithmManager::Instance().create("Load");
      //algLoadRun->setChild(true);
      algLoadRun->initialize();
      algLoadRun->setProperty("Filename", run);
      algLoadRun->setProperty("OutputWorkspace", run);
      if (algLoadRun->isInitialized())
      {
        algLoadRun->execute();
      }
      if (algLoadRun->isExecuted())
      {
        if (trans != lastTrans)
        {
          makeTransWS(trans);
        }

        IAlgorithm_sptr algReflOne = AlgorithmManager::Instance().create("ReflectometryReductionOneAuto");
        //algReflOne->setChild(true);
        algReflOne->initialize();
        algReflOne->setProperty("InputWorkspace", AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(run));
        algReflOne->setProperty("FirstTransmissionRun", AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("TransWS"));
        algReflOne->setProperty("OutputWorkspace", run + "_IvsQ");
        algReflOne->setProperty("OutputWorkspaceWaveLength", run + "_IvsLam");
        algReflOne->setProperty("ThetaIn", theta);

        if (algReflOne->isInitialized())
        {
          algReflOne->execute();
        }
        if (algReflOne->isExecuted())
        {
          std::vector<double> built_params;
          built_params.push_back(qmin);
          built_params.push_back(-dqq);
          built_params.push_back(qmax);

          IAlgorithm_sptr algRebinQ = AlgorithmManager::Instance().create("Rebin");
          //algRebinQ->setChild(true);
          algRebinQ->initialize();
          algRebinQ->setProperty("InputWorkspace", AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(run + "_IvsQ"));
          algRebinQ->setProperty("Params", built_params);
          algRebinQ->setProperty("OutputWorkspace", run + "_IvsQ_binned");

          IAlgorithm_sptr algRebinLam = AlgorithmManager::Instance().create("Rebin");
          //algRebinLam->setChild(true);
          algRebinLam->initialize();
          algRebinLam->setProperty("InputWorkspace", AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(run + "_IvsLam"));
          algRebinLam->setProperty("Params", built_params);
          algRebinLam->setProperty("OutputWorkspace", run + "_IvsLam_binned");

          if (algRebinLam->isInitialized())
          {
            algRebinLam->execute();
          }
          if (algRebinQ->isInitialized())
          {
            algRebinQ->execute();
          }
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
      {
        std::vector<double> HACKYPARAMS;
        HACKYPARAMS.push_back(1.5);
        HACKYPARAMS.push_back(0.02);
        HACKYPARAMS.push_back(17);

        algCreateTrans->setProperty("SecondTransmissionRun", transVec[1]);
        algCreateTrans->setProperty("Params",HACKYPARAMS); //HACK FOR NOW
        algCreateTrans->setProperty("StartOverlap",10.0); //HACK FOR NOW
        algCreateTrans->setProperty("EndOverlap",12.0); //HACK FOR NOW
      }

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
      if(m_view->getSaveAsFlag())
        saveAs();

      if(m_view->getSaveFlag())
        save();

      if(m_view->getAddRowFlag())
        addRow();

      if(m_view->getProcessFlag())
        process();

      if(m_view->getDeleteRowFlag())
        deleteRow();

      m_view->clearNotifyFlags();
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
