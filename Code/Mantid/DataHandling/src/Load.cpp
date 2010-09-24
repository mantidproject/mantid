//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/Load.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidDataObjects/Workspace2D.h"

#include <algorithm>
#include <sstream>

namespace Mantid
{
  namespace DataHandling
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(Load)

    using namespace Kernel;
    using namespace API;

    /// Initialisation method.
    void Load::init()
    {
      std::vector<std::string> exts;
      exts.push_back(".raw");
      exts.push_back(".s*");
      exts.push_back(".add");

      exts.push_back(".nxs");
      exts.push_back(".nx5");
      exts.push_back(".xml");
      exts.push_back(".n*");

      exts.push_back(".dat");
      exts.push_back(".txt");
      exts.push_back(".csv");

      exts.push_back(".spe");

      declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
		      "The name of the file to read, including its full or relative\n"
		      "path. (N.B. case sensitive if running on Linux).");
      declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace",
        "",Direction::Output), "The name of the workspace that will be created, filled with the\n"
		      "read-in data and stored in the Analysis Data Service.");

      BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
      mustBePositive->setLower(1);
      declareProperty("SpectrumMin", 1, mustBePositive);
      declareProperty("SpectrumMax", EMPTY_INT(), mustBePositive->clone());
      declareProperty(new ArrayProperty<int>("SpectrumList"));
    }

    /** 
     *   Executes the algorithm.
     */
    void Load::exec()
    {
      std::string fileName = getPropertyValue("Filename");
      std::string::size_type i = fileName.find_last_of('.');
      std::string ext = fileName.substr(i+1);
      std::transform(ext.begin(),ext.end(),ext.begin(),tolower);

      API::IAlgorithm_sptr load;
      if (ext == "raw" || ext == "add")
      {
        runLoadRaw(load);
      }
      else if (ext == "nxs" || ext == "nx5")
      {
        runLoadNexus(load);
      }
      else if (ext == "dat" || ext == "txt" || ext == "csv")
      {
        runLoadAscii(load);
      }
      else if (ext == "spe")
      {
        runLoadSPE(load);
      }
      else if (ext[0] == 's')
      {
        runLoadRaw(load);
      }
      else if (ext[0] == 'n')
      {
        runLoadNexus(load);
      }
      else if (ext == "xml")
      {
        try
        {
          runLoadNexus(load);
        }
        catch(...)
        {
          runLoadSpice2D(load);
        }
      }
      else
      {
        throw std::runtime_error("Cannot load file " + fileName);
      }

    }

    /**
      * Run LoadRaw
      * @param load Shared pointer to the subalgorithm
      */ 
    void Load::runLoadRaw(API::IAlgorithm_sptr& load)
    {
      load = createSubAlgorithm("LoadRaw",0,1);
      load->initialize();
      load->setPropertyValue("Filename",getPropertyValue("Filename"));
      load->setPropertyValue("OutputWorkspace",getPropertyValue("OutputWorkspace"));
      load->setPropertyValue("SpectrumMin",getPropertyValue("SpectrumMin"));
      load->setPropertyValue("SpectrumMax",getPropertyValue("SpectrumMax"));
      load->setPropertyValue("SpectrumList",getPropertyValue("SpectrumList"));
      load->execute();
      setOutputWorkspace(load);
    }

    /**
      * Run LoadNexus
      * @param load Shared pointer to the subalgorithm
      */ 
    void Load::runLoadNexus(API::IAlgorithm_sptr& load)
    {
      load = createSubAlgorithm("LoadNexus",0,1);
      load->initialize();
      load->setPropertyValue("Filename",getPropertyValue("Filename"));
      load->setPropertyValue("OutputWorkspace",getPropertyValue("OutputWorkspace"));
      load->setPropertyValue("SpectrumMin",getPropertyValue("SpectrumMin"));
      load->setPropertyValue("SpectrumMax",getPropertyValue("SpectrumMax"));
      load->setPropertyValue("SpectrumList",getPropertyValue("SpectrumList"));
      load->execute();
      setOutputWorkspace(load);
    }

    /**
      * Run LoadAscii
      * @param load Shared pointer to the subalgorithm
      */ 
    void Load::runLoadAscii(API::IAlgorithm_sptr& load)
    {
      load = createSubAlgorithm("LoadAscii",0,1);
      load->initialize();
      load->setPropertyValue("Filename",getPropertyValue("Filename"));
      load->setPropertyValue("OutputWorkspace",getPropertyValue("OutputWorkspace"));
      load->execute();
      setOutputMatrixWorkspace(load);
    }

    /**
      * Run LoadSPE
      * @param load Shared pointer to the subalgorithm
      */ 
    void Load::runLoadSPE(API::IAlgorithm_sptr& load)
    {
      load = createSubAlgorithm("LoadSPE",0,1);
      load->initialize();
      load->setPropertyValue("Filename",getPropertyValue("Filename"));
      load->setPropertyValue("OutputWorkspace",getPropertyValue("OutputWorkspace"));
      load->execute();
      setOutputMatrixWorkspace(load);
    }

    void Load::runLoadSpice2D(API::IAlgorithm_sptr& load)
    {
      load = createSubAlgorithm("LoadSpice2D",0,1);
      load->initialize();
      load->setPropertyValue("Filename",getPropertyValue("Filename"));
      load->setPropertyValue("OutputWorkspace",getPropertyValue("OutputWorkspace"));
      load->execute();
      setOutputWorkspace(load);
    }

    /**
      * Set the output workspace(s) if the load's return workspace
      *  has type API::Workspace
      */
    void Load::setOutputWorkspace(API::IAlgorithm_sptr& load)
    {
      Workspace_sptr ws = load->getProperty("OutputWorkspace");
      setProperty("OutputWorkspace",ws);
      WorkspaceGroup_sptr wsg = boost::dynamic_pointer_cast<WorkspaceGroup>(ws);
      if (wsg)
      {
        std::vector<std::string> names = wsg->getNames();
        for(size_t i = 0; i < names.size(); ++i)
        {
          std::ostringstream propName;
          propName << "OutputWorkspace_" << (i+1);
          DataObjects::Workspace2D_sptr ws1 = load->getProperty(propName.str());
          std::string wsName = load->getPropertyValue(propName.str());
          declareProperty(new WorkspaceProperty<>(propName.str(),wsName,Direction::Output));
          setProperty(propName.str(),boost::dynamic_pointer_cast<MatrixWorkspace>(ws1));
        }
      }
    }

    /**
      * Set the output workspace(s) if the load's return workspace
      *  has type API::MatrixWorkspace
      */
    void Load::setOutputMatrixWorkspace(API::IAlgorithm_sptr& load)
    {
      MatrixWorkspace_sptr ws = load->getProperty("OutputWorkspace");
      setProperty("OutputWorkspace",boost::dynamic_pointer_cast<Workspace>(ws));
    }

  } // namespace DataHandling
} // namespace Mantid
