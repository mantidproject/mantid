#include "MantidQtCustomInterfaces/RawFileMemento.h"
#include "MantidKernel/Matrix.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include <iostream>
#include <fstream>
#include <boost/regex.hpp>

using namespace Mantid::API;

namespace MantidQt
{
  namespace CustomInterfaces
  {
      /**
      Constructor
      @param fileName : path + name of the file to load
      */
      RawFileMemento::RawFileMemento(std::string fileName) : m_fileName(fileName)
      {
        boost::regex pattern("(NXS)$", boost::regex_constants::icase); 

        //Fail if the file extension is wrong.
        if(!boost::regex_search(fileName, pattern))
        {
          std::string msg = "NexusFileMemento:: Unknown File extension on: " + fileName;
          throw std::invalid_argument(msg);
        }

        //Fail if there is no file at the given location
        if(!checkStillThere())
        {
          throw std::invalid_argument("NexusFileMemento:: File doesn't exist");
        }

        std::vector<std::string> strs;
        boost::split(strs, m_fileName, boost::is_any_of("/,\\"));
        m_adsID = strs.back();
        m_adsID = m_adsID.substr(0, m_adsID.find('.'));

        //Generate an initial report.
        MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(fetchIt(MinimalData));
        if(ws->mutableSample().hasOrientedLattice())
        {
          std::vector<double> ub = ws->mutableSample().getOrientedLattice().getUB().getVector();
          this->setUB(ub[0], ub[1], ub[2], ub[3], ub[4], ub[5], ub[6], ub[7], ub[8]);
        }
        cleanUp();
      }

      /**
      Getter for the id of the workspace
      @return the id of the workspace
      */
      std::string RawFileMemento::getId() const
      {
        return m_adsID;
      }

      /**
      Getter for the type of location where the workspace is stored
      @ return the location type
      */
      std::string RawFileMemento::locationType() const
      {
        return locType();
      }

      /**
      Check that the workspace has not been deleted since instantiating this memento
      @return true if still in specified location
      */
      bool RawFileMemento::checkStillThere() const
      {
        std::ifstream ifile;
        ifile.open(m_fileName.c_str(), std::ifstream::in);
        return !ifile.fail();
      }

      /**
      Getter for the workspace itself
      @returns the matrix workspace
      @param protocol : Follow the protocol to fetch all spectrum or just the first.
      @throw if workspace has been moved since instantiation.
      */
      Mantid::API::Workspace_sptr RawFileMemento::fetchIt(FetchProtocol protocol) const
      {
        checkStillThere();

        IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("LoadNexus");
        alg->initialize();
        alg->setRethrows(true);
        alg->setProperty("Filename", m_fileName);
        alg->setPropertyValue("OutputWorkspace", m_adsID);
        if(protocol == MinimalData)
        {
          alg->setProperty("SpectrumMin", 0);
          alg->setProperty("SpectrumMax", 1);
        }
        alg->execute();

        // Overwrite add log values. These are commonly needed by algorithms such as SetGoniometer.
        for(size_t i = 0 ; i < m_logEntries.size(); i++)
        {
          Mantid::API::IAlgorithm_sptr logAlg = Mantid::API::AlgorithmManager::Instance().create("AddSampleLog");
          logAlg->initialize();
          logAlg->setRethrows(true);
          logAlg->setPropertyValue("Workspace", this->m_adsID);
          logAlg->setPropertyValue("LogName", m_logEntries[i].name);
          logAlg->setPropertyValue("LogText", m_logEntries[i].value);
          logAlg->setPropertyValue("LogType", m_logEntries[i].type);
          logAlg->execute();
        }

        Workspace_sptr ws = AnalysisDataService::Instance().retrieve(m_adsID);

        Mantid::API::WorkspaceGroup_sptr gws = boost::dynamic_pointer_cast<WorkspaceGroup>(ws);
        if(gws != NULL)
        {
          throw std::invalid_argument("This raw file corresponds to a WorkspaceGroup. Cannot process groups like this. Import via MantidPlot instead.");
        }
        return ws;
      }

      /**
      Dump the workspace out of memory:
      @name : name of the workspace to clean-out.
      */
      void RawFileMemento::dumpIt(const std::string& name)
      {
        if(AnalysisDataService::Instance().doesExist(name))
        {
          AnalysisDataService::Instance().remove(name);
        }
      }

      /// Destructor
      RawFileMemento::~RawFileMemento()
      {
      }

      /// Clean up.
      void RawFileMemento::cleanUp()
      {
          dumpIt(m_adsID);
      }

      /*
      Apply actions. Load workspace and apply all actions to it.
      */
      Mantid::API::Workspace_sptr RawFileMemento::applyActions()
      {
        Mantid::API::Workspace_sptr ws = fetchIt(Everything);
        
        // Overwrite ub matrix
        if(m_ub.size() == 9)
        {
          Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("SetUB");
          alg->initialize();
          alg->setRethrows(true);
          alg->setPropertyValue("Workspace", this->m_adsID);
          alg->setProperty("UB", m_ub);
          alg->execute();
        }
        // Overwrite goniometer settings
        if(m_axes.size() == 6)
        {
          Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("SetGoniometer");
          alg->initialize();
          alg->setRethrows(true);
          alg->setPropertyValue("Workspace", this->m_adsID);
          if(!m_axes[0].empty())
          {
            alg->setProperty("Axis0", m_axes[0]);
          }
          if(!m_axes[1].empty())
          {
            alg->setProperty("Axis1", m_axes[1]);
          }
          if(!m_axes[2].empty())
          {
            alg->setProperty("Axis2", m_axes[2]);
          }
          if(!m_axes[3].empty())
          {
            alg->setProperty("Axis3", m_axes[3]);
          }
          if(!m_axes[4].empty())
          {
            alg->setProperty("Axis4", m_axes[4]);
          }
          if(!m_axes[5].empty())
          {
            alg->setProperty("Axis5", m_axes[5]);
          }
          alg->execute();
        }
        return AnalysisDataService::Instance().retrieve(m_adsID);
      }

  }
}