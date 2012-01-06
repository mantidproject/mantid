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
        boost::regex pattern("(RAW)$", boost::regex_constants::icase); 

        if(!boost::regex_search(fileName, pattern))
        {
          std::string msg = "RawFileMemento:: Unknown File extension on: " + fileName;
          throw std::invalid_argument(msg);
        }

        if(!checkStillThere())
        {
          throw std::runtime_error("RawFileMemento:: File doesn't exist");
        }

        std::vector<std::string> strs;
        boost::split(strs, m_fileName, boost::is_any_of("/,\\"));
        m_adsID = strs.back();
        m_adsID = m_adsID.substr(0, m_adsID.find('.'));

        //Generate an initial report.
        MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(fetchIt(MinimalData));
        if(ws->mutableSample().hasOrientedLattice())
        {
          std::vector<double> ub = ws->mutableSample().getOrientedLattice().getUB().get_vector();
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

        IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("LoadRaw");
        alg->initialize();
        alg->setRethrows(true);
        alg->setProperty("Filename", m_fileName);
        alg->setPropertyValue("OutputWorkspace", m_adsID);
        if(protocol == MinimalData)
        {
          alg->setProperty("SpectrumMin", 1);
          alg->setProperty("SpectrumMax", 1);
        }
        alg->execute();

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
        
        if(m_ub.size() == 9)
        {
          Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("SetUB");
          alg->initialize();
          alg->setRethrows(true);
          alg->setPropertyValue("Workspace", this->m_adsID);
          alg->setProperty("UB", m_ub);
          alg->execute();
        }
        return AnalysisDataService::Instance().retrieve(m_adsID);
      }

  }
}