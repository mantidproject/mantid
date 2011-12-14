#include "MantidQtCustomInterfaces/WorkspaceOnDisk.h"
#include "MantidAPI/AlgorithmManager.h"
#include <iostream>
#include <fstream>
#include <boost/regex.hpp>

namespace MantidQt
{
  namespace CustomInterfaces
  {
      /**
      Constructor
      @param fileName : path + name of the file to load
      */
      WorkspaceOnDisk::WorkspaceOnDisk(std::string fileName) : m_fileName(fileName)
      {
        boost::regex pattern("(RAW)$", boost::regex_constants::icase); 

        if(!boost::regex_search(fileName, pattern))
        {
          std::string msg = "WorkspaceOnDisk:: Unknown File extension on: " + fileName;
          throw std::invalid_argument(msg);
        }
        if(!checkStillThere())
        {
          throw std::runtime_error("WorkspaceOnDisk:: File doesn't exist");
        }
      }

      /**
      Getter for the id of the workspace
      @return the id of the workspace
      */
      std::string WorkspaceOnDisk::getId() const
      {
        return m_fileName;
      }

      /**
      Getter for the type of location where the workspace is stored
      @ return the location type
      */
      std::string WorkspaceOnDisk::locationType() const
      {
        return "On Disk";
      }

      /**
      Check that the workspace has not been deleted since instantiating this memento
      @return true if still in specified location
      */
      bool WorkspaceOnDisk::checkStillThere() const
      {
        std::ifstream ifile(m_fileName);
        return ifile;
      }

      /**
      Getter for the workspace itself
      @returns the matrix workspace
      @throw if workspace has been moved since instantiation.
      */
      Mantid::API::MatrixWorkspace_sptr WorkspaceOnDisk::fetchIt() const
      {
        using namespace Mantid::API;

        IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("LoadRaw");
        alg->initialize();
        alg->setProperty("Filename", m_fileName);
        alg->setProperty("OutputWorkspace", "_tmp");
        alg->execute();

        return boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("_tmp"));
      }

      /// Destructor
      WorkspaceOnDisk::~WorkspaceOnDisk()
      {
      }

  }
}