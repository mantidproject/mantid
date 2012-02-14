/*WIKI* 


The Load algorithm is a more intelligent algorithm than most other load algorithms. When passed a filename it attempts to search the existing load [[:Category:Algorithms|algorithms]] and find the most appropriate to load the given file. The specific load algorithm is then run as a child algorithm with the exception that it logs messages to the Mantid logger.

==== Specific Load Algorithm Properties ====

Each specific loader will have its own properties that are appropriate to it:  SpectrumMin and SpectrumMax for ISIS RAW/NeXuS, FilterByTof_Min and FilterByTof_Max for Event data. The Load algorithm cannot know about these properties until it has been told the filename and found the correct loader. Once this has happened the properties of the specific Load algorithm are redeclared on to that copy of Load.

== Usage ==

==== Python ====

Given the variable number and types of possible arguments that Load can take, its simple Python function cannot just list the properties as arguments like the others do. Instead the Python function <code>Load</code> can handle any number of arguments. The OutputWorkspace and Filename arguments are the exceptions in that they are always checked for. A snippet regarding usage from the <code>help(Load)</code> is shown below
<div style="border:1pt dashed blue; background:#f9f9f9;padding: 1em 0;">
<source lang="python">
# Simple usage, ISIS NeXus file
Load('INSTR00001000.nxs', 'run_ws')

# ISIS NeXus with SpectrumMin and SpectrumMax = 1
Load('INSTR00001000.nxs', 'run_ws', SpectrumMin=1,SpectrumMax=1)

# SNS Event NeXus with precount on
Load('INSTR_1000_event.nxs', 'event_ws', Precount=True)

# A mix of keyword and non-keyword is also possible
Load('event_ws', Filename='INSTR_1000_event.nxs',Precount=True)
</source></div>


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/Load.h"
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/LoadAlgorithmFactory.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/FacilityInfo.h"

#include <cctype>
#include <algorithm>
#include <functional>
#include <numeric>
#include <set>
#include "MantidAPI/IMDEventWorkspace.h"
#include <cstdio>


namespace
{
  /**
   * Convenience function that returns true if the passed vector of vector of strings
   * contains a single string, false if contains more than that (or zero).
   *
   * @param fileNames :: a vector of vectors of file name strings.
   *
   * @returns true if there is exactly one string, else false.
   */
  bool isSingleFile(const std::vector<std::vector<std::string> > & fileNames)
  {
    if(fileNames.size() == 1)
    {
      std::vector<std::vector<std::string>>::const_iterator first = fileNames.begin();
      if(first->size() == 1)
        return true;
    }
    return false;
  }

  /**
   * Helper function that takes a vector of runs, and generates a suggested workspace name.
   * This will likely need to be improved and may have to include instrument name, etc.
   *
   * @param runs :: a vector of run numbers.
   *
   * @returns a string containing a suggested file name based on the given run numbers.
   */
  std::string generateWsName(std::vector<unsigned int> runs)
  {
    std::string wsName("");

    for(size_t i = 0; i < runs.size(); ++i)
    {
      if(!wsName.empty())
        wsName += "_";

      wsName += boost::lexical_cast<std::string>(runs[i]);
    }

    return wsName;
  }
}

namespace Mantid
{
  namespace DataHandling
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(Load);
    
    /// Sets documentation strings for this algorithm
    void Load::initDocs()
    {
      this->setWikiSummary("Attempts to load a given file by finding an appropriate Load algorithm. ");
      this->setOptionalMessage("Attempts to load a given file by finding an appropriate Load algorithm.");
    }
    

    using namespace Kernel;
    using namespace API;

    //--------------------------------------------------------------------------
    // Public methods
    //--------------------------------------------------------------------------

    /// Default constructor
    Load::Load() : IDataFileChecker(), m_baseProps()
    {
    }

    /** Override setPropertyValue to catch if filename is being set, as this may mean
     *  a change of concrete loader. If it's any other property, just forward the call.
     *  @param name The name of the property
     *  @param value The value of the property as a string
     */
    void Load::setPropertyValue(const std::string &name, const std::string &value)
    {
      // Call base class method in all cases.
      // For a filename property is deals with resolving the full path.
      IDataFileChecker::setPropertyValue(name, value);

      std::string NAME(name);
      std::transform(name.begin(),name.end(),NAME.begin(),toupper);
      if( NAME == "FILENAME" )
      {
        // Get back full path before passing to getFileLoader method, and also
        // find out whether this is a multi file load.
        std::vector<std::vector<std::string> > fileNames = getProperty("Filename");
        // If it's a single file load, then it's fine to change loader.
        if(isSingleFile(fileNames))
        {
          IDataFileChecker_sptr loader = getFileLoader(getPropertyValue(name));
          if( loader ) declareLoaderProperties(loader);
        }
      }
    }

    //--------------------------------------------------------------------------
    // Private methods
    //--------------------------------------------------------------------------

    /**
    * Quick file always returns false here
    * @param filePath :: File path
    * @param nread :: Number of bytes read
    * @param header :: A buffer containing the nread bytes
    */
    bool Load::quickFileCheck(const std::string& filePath,size_t nread,const file_header& header)
    {
      (void)filePath; (void)nread; (void)header;
      return false;
    }

    /**
    * File check by looking at the structure of the data file
    * @param filePath :: The full file path
    * @returns -1
    */
    int Load::fileCheck(const std::string& filePath)
    {
      (void)filePath;
      return -1;
    }

    /** 
    * Get a shared pointer to the load algorithm with highest preference for loading
    * @param filePath :: path of the file
    * @returns A shared pointer to the unmanaged algorithm
    */
    API::IDataFileChecker_sptr Load::getFileLoader(const std::string& filePath)
    {
      /* Open the file and read in the first bufferSize bytes - these will
      * be used to determine the type of the file
      */
      FILE* fp = fopen(filePath.c_str(), "rb");
      if (fp == NULL)
      {
        throw Kernel::Exception::FileError("Unable to open the file:", filePath);
      }
      file_header header;
      size_t nread = fread(&header,sizeof(unsigned char), g_hdr_bytes, fp);
      // Ensure the character string is null terminated.
      header.full_hdr[g_hdr_bytes] = '\0';

      if (fclose(fp) != 0)
      {
        throw std::runtime_error("Error while closing file \"" + filePath + "\"");
      } 

      // Iterate through all loaders and attempt to find the best qualified for the job.
      // Each algorithm has a quick and long file check. The long version returns an integer
      // giving its certainty about be able to load the file. The highest wins.
      std::vector<std::string> loaderNames = API::LoadAlgorithmFactory::Instance().getKeys();
      int highestPref(0);
      API::IDataFileChecker_sptr winningLoader;
      std::vector<std::string>::const_iterator cend = loaderNames.end();
      for( std::vector<std::string>::const_iterator citr = loaderNames.begin(); citr != cend;
        ++citr )
      {
        IDataFileChecker_sptr loader = API::LoadAlgorithmFactory::Instance().create(*citr);
        try
        {
          if( loader->quickFileCheck(filePath, nread, header) )
          {
            int pref = loader->fileCheck(filePath);
            // Can't just pick the first as there might be one later in the list with a higher
            // preference
            if( pref > highestPref )
            {
              highestPref = pref;
              winningLoader = loader;
            }
          }
        }
        catch (std::exception & e)
        {
          g_log.debug() << "Error running file check of " <<  loader->name() << std::endl;
          g_log.debug() << e.what() << std::endl;

        }

      }

      if( !winningLoader )
      {
        // Clear what may have been here previously
        setPropertyValue("LoaderName", "");
        throw std::runtime_error("Cannot find an algorithm that is able to load \"" + filePath + "\".\n"
                                 "Check that the file is a supported type.");
      }
      g_log.debug() << "Winning loader is " <<  winningLoader->name() << std::endl;
      setPropertyValue("LoaderName", winningLoader->name());
      winningLoader->initialize();
      setUpLoader(winningLoader);
      return winningLoader;
    }

    /**
    * Declare any additional properties of the concrete loader here
    * @param loader A pointer to the concrete loader
    */
    void Load::declareLoaderProperties(const IDataFileChecker_sptr loader)
    {
      // If we have switch loaders then the concrete loader will have different properties
      // so take care of ensuring Load has the correct ones
      const std::vector<Property*> existingProps = this->getProperties();      
      for( size_t i = 0; i < existingProps.size(); ++i )
      {
        const std::string name = existingProps[i]->name();
        // Wipe all properties except the Load native ones
        if( m_baseProps.find(name) == m_baseProps.end() )
        {
          this->removeProperty(name, false);
        }
      }

      const std::vector<Property*> &loaderProps = loader->getProperties();
      const std::string filePropName(loader->filePropertyName());
      size_t numProps(loaderProps.size());
      for (size_t i = 0; i < numProps; ++i)
      {
        Property* loadProp = loaderProps[i];
        if( loadProp->name() == filePropName ) continue;
        try
        {
          Property * propClone = loadProp->clone();
          propClone->deleteSettings(); //Get rid of special settings because it does not work in custom GUI.
          declareProperty(propClone, loadProp->documentation());
        }
        catch(Exception::ExistsError&)
        {
          // Already exists as a static property
          continue;
        }
      }
    }

    /// Initialisation method.
    void Load::init()
    {
      // Take extensions first from Facility object
      const FacilityInfo & defaultFacility = Mantid::Kernel::ConfigService::Instance().getFacility();
      std::vector<std::string> exts = defaultFacility.extensions();
      // Add in some other known extensions
      exts.push_back(".xml");
      exts.push_back(".dat");
      exts.push_back(".txt");
      exts.push_back(".csv");
      exts.push_back(".spe");
      exts.push_back(".grp");
      exts.push_back(".nxspe");

      declareProperty(new MultipleFileProperty("Filename", exts),
        "The name of the file(s) to read, including the full or relative\n"
        "path. (N.B. case sensitive if running on Linux). Multiple runs\n"
        "can be loaded and added together, e.g. INST10,11+12,13.ext");
      declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace", "",Direction::Output), 
        "The name of the workspace that will be created, filled with the\n"
        "read-in data and stored in the Analysis Data Service.");

      declareProperty("LoaderName", std::string(""), "A string containing the name of the concrete loader used", 
        Direction::Output);
      // Save for later what the base Load properties are
      const std::vector<Property*> & props = this->getProperties();
      for( size_t i = 0; i < this->propertyCount(); ++i )
      {
        m_baseProps.insert(props[i]->name());
      }

    }

    /** 
     * Executes the algorithm.
     */
    void Load::exec()
    {
      std::vector<std::vector<std::string> > fileNames = getProperty("Filename");
      
      if(isSingleFile(fileNames))
      {
        // This is essentially just the same code that was called before multiple files were supported.
        loadSingleFile();
      }
      else
      {
        // Code that supports multiple file loading.
        loadMultipleFiles();
      }
    }

    void Load::loadSingleFile()
    {
      std::string loaderName = getPropertyValue("LoaderName");
      if( loaderName.empty() )
      {
        m_loader = getFileLoader(getPropertyValue("Filename"));
        loaderName = m_loader->name();
        setPropertyValue("LoaderName",loaderName);
      }
      else
      {
        m_loader = createLoader(loaderName,0,1);
      }
      g_log.information() << "Using " << loaderName << " version " << m_loader->version() << ".\n";
      ///get the list properties for the concrete loader load algorithm
      const std::vector<Kernel::Property*> & loader_props = m_loader->getProperties();

      // Loop through and set the properties on the sub algorithm
      std::vector<Kernel::Property*>::const_iterator itr;
      for (itr = loader_props.begin(); itr != loader_props.end(); ++itr)
      {
        const std::string propName = (*itr)->name();
        if( this->existsProperty(propName) )
        {
          m_loader->setPropertyValue(propName, getPropertyValue(propName));
        }
        else if( propName == m_loader->filePropertyName() )
        {
          m_loader->setPropertyValue(propName, getPropertyValue("Filename"));
        }
      }

      // Execute the concrete loader
      m_loader->execute();
      // Set the workspace. Deals with possible multiple periods
      setOutputWorkspace(m_loader);
    }


    void Load::loadMultipleFiles()
    {
      MultipleFileProperty * multiFileProp = dynamic_cast<MultipleFileProperty*>(getPointerToProperty("Filename"));
      
      const std::vector<std::vector<std::string> > values = getProperty("Filename");
      const std::vector<std::vector<unsigned int> > runs = multiFileProp->getRuns();
      std::string outputWsName = getProperty("OutputWorkspace");

      std::vector<std::string> loadedWsNames;

      std::vector<std::vector<std::string> >::const_iterator values_it = values.begin();
      std::vector<std::vector<unsigned int> >::const_iterator runs_it = runs.begin();

      // Cycle through the fileNames and wsNames.
      for(; values_it != values.end(); ++values_it, ++runs_it)
      {
        std::vector<std::string> fileNames = *values_it;
        std::string wsName = generateWsName(*runs_it);

        // If there is only one filename, then just load it to the given wsName.
        if(fileNames.size() == 1)
        {
          loadFileToHiddenWs(fileNames.at(0), wsName);
          loadedWsNames.push_back("__" + wsName);
        }
        // Else there is more than one filename.  Load them all, sum them, and rename the
        // result to the given wsName.
        else
        {
          // Load all files and place the resulting workspaces in a vector.
          std::vector<Workspace_sptr> loadedWs;
          std::vector<std::string>::const_iterator vIt = fileNames.begin();

          for(; vIt != fileNames.end(); ++vIt)
          {
            Workspace_sptr ws = loadFileToHiddenWs(*vIt, (*vIt) + "_temp");
            loadedWs.push_back(ws);
          }

          // Add all workspaces together, sticking the result in sum.
          Workspace_sptr sum;
          for(
            size_t i = 1; // Start at second workspace in list.
            i < loadedWs.size(); 
            i++)
          {
            Workspace_sptr firstWsToAdd;
            // If there have been no workspaces added yet, then the first workspace to add
            // is the first workspace in the list.
            if(sum == Workspace_sptr())
              firstWsToAdd = loadedWs.at(i-1);
            // Else the first workspace to add is "sum" itself.
            else
              firstWsToAdd = sum;
            
            Workspace_sptr secondWsToAdd = loadedWs.at(i);
            sum = plusWs(firstWsToAdd, secondWsToAdd);
          }

          // Delete all of the temporarily loaded workspaces except the first one, so that we are left only
          // with sum at this point.
          for(size_t i = 1; i < fileNames.size(); i++)
          {
            deleteWs("__" + fileNames.at(i) + "_temp");
          }

          // Rename the sum and add to the list of loaded workspace names.
          renameWs(sum->name(), "__" + wsName);
          loadedWsNames.push_back("__" + wsName);
        }
      }

      // If we only have one loaded ws, set it as the output.
      if(loadedWsNames.size() == 1)
      {
        renameWs(loadedWsNames.at(0), outputWsName);
        setProperty("OutputWorkspace", AnalysisDataService::Instance().retrieve(outputWsName.c_str()));
      }
      // Else we have multiple loaded workspaces - group them and set the group as output.
      else
      {
        Mantid::API::IAlgorithm_sptr groupingAlg = 
          Mantid::API::AlgorithmManager::Instance().create("GroupWorkspaces", 1);

        groupingAlg->setProperty("InputWorkspaces",loadedWsNames);
        groupingAlg->setProperty("OutputWorkspace",outputWsName.c_str());
        groupingAlg->execute();

        unhideWs(outputWsName);

        setProperty("OutputWorkspace", AnalysisDataService::Instance().retrieve(outputWsName.c_str()));
      }
    }

    /** 
    * Create the concrete instance use for the actual loading.
    * @param name :: The name of the loader to instantiate
    * @param startProgress :: The percentage progress value of the overall 
    * algorithm where this child algorithm starts
    * @param endProgress :: The percentage progress value of the overall 
    * algorithm where this child algorithm ends
    * @param logging :: Set to false to disable logging from the child algorithm
    */
    API::IDataFileChecker_sptr Load::createLoader(const std::string & name, const double startProgress, 
      const double endProgress, const bool logging) const
    {
      IDataFileChecker_sptr loader = boost::static_pointer_cast<IDataFileChecker>(
        API::AlgorithmManager::Instance().createUnmanaged(name));
      loader->initialize();
      if( !loader )
      {
        throw std::runtime_error("Cannot create loader for \"" + getPropertyValue("Filename") + "\"");
      }
      setUpLoader(loader,startProgress,endProgress, logging);
      return loader;
    }

    /**
     * Set the loader option for use as a sub algorithm.
     * @param loader :: Concrete loader
     * @param startProgress :: The start progress fraction
     * @param endProgress :: The end progress fraction
     * @param logging:: If true, enable logging
     */
    void Load::setUpLoader(API::IDataFileChecker_sptr loader, const double startProgress, 
			   const double endProgress, const bool logging) const
    {
      //Set as a child so that we are in control of output storage
      loader->setChild(true);
      loader->setLogging(logging);
      // If output workspaces are nameless, give them a temporary name to satisfy validator
      const std::vector< Property*> &props = loader->getProperties();
      for (unsigned int i = 0; i < props.size(); ++i)
      {
        if (props[i]->direction() == Direction::Output && 
          dynamic_cast<IWorkspaceProperty*>(props[i]) )
        {
          if ( props[i]->value().empty() ) props[i]->setValue("LoadChildWorkspace");
        }
      }
      if (startProgress >= 0. && endProgress > startProgress && endProgress <= 1.)
      {
        loader->addObserver(m_progressObserver);
        setChildStartProgress(startProgress);
        setChildEndProgress(endProgress);
      }
    }


    /**
    * Set the output workspace(s) if the load's return workspace has type API::Workspace
    * @param loader :: Shared pointer to load algorithm
    */
    void Load::setOutputWorkspace(const API::IDataFileChecker_sptr loader)
    {
      // Go through each OutputWorkspace property and check whether we need to make a counterpart here
      const std::vector<Property*> loaderProps = loader->getProperties();
      const size_t count = loader->propertyCount();
      for( size_t i = 0; i < count; ++i )
      {
        Property *prop = loaderProps[i];
        if( dynamic_cast<IWorkspaceProperty*>(prop) && prop->direction() == Direction::Output )
        {
          const std::string & name = prop->name();
          if( !this->existsProperty(name) )
          {
            declareProperty(new WorkspaceProperty<Workspace>(name, loader->getPropertyValue(name),
              Direction::Output));
          }
          Workspace_sptr wkspace = getOutputWorkspace(name, loader);
          setProperty(name, wkspace);
        }
      }
    }

    /**
    * Return an output workspace property dealing with the lack of connection between of 
    * WorkspaceProperty types
    * @param propName :: The name of the property
    * @param loader :: The loader algorithm
    * @returns A pointer to the OutputWorkspace property of the sub algorithm
    */
    API::Workspace_sptr Load::getOutputWorkspace(const std::string & propName,
      const API::IDataFileChecker_sptr loader) const
    {
      // @todo Need to try and find a better way using the getValue methods
      try
      {
        return loader->getProperty(propName);
      }
      catch(std::runtime_error&)
      { }

      // Try a MatrixWorkspace
      try
      {
        MatrixWorkspace_sptr childWS = loader->getProperty(propName);
        return childWS;
      }
      catch(std::runtime_error&)
      { }

      // EventWorkspace
      try
      {
        IEventWorkspace_sptr childWS = loader->getProperty(propName);
        return childWS;
      }
      catch(std::runtime_error&)
      { }

      // IMDEventWorkspace
      try
      {
        IMDEventWorkspace_sptr childWS = loader->getProperty(propName);
        return childWS;
      }
      catch(std::runtime_error&)
      { }

      // Just workspace?
      try
      {
        Workspace_sptr childWS = loader->getProperty(propName);
        return childWS;
      }
      catch(std::runtime_error&)
      { }

      g_log.debug() << "Workspace property " << propName << " did not return to MatrixWorkspace, EventWorkspace, or IMDEventWorkspace." << std::endl;
      return Workspace_sptr();
    }

    /*
    * Overrides the default cancel() method. Calls cancel() on the actual loader.
    */
    void Load::cancel()const
    {
      if (m_loader)
      {
        m_loader->cancel();
      }
    }

    /**
     * Loads a file into a *hidden* workspace.
     *
     * @param fileName :: file name to load.
     * @param wsName   :: workspace name, which will be prefixed by a "__"
     *
     * @returns a pointer to the loaded workspace
     */
    API::Workspace_sptr Load::loadFileToHiddenWs(
      const std::string & fileName, 
      const std::string & wsName)
    {
      Mantid::API::IAlgorithm_sptr loadAlg = createSubAlgorithm("Load", 1);
      // Here, as a workaround for groupworkspaces who's members have names but no
      // accompanying entries in the ADS, we set the sub algo to not be a child ...
      loadAlg->setChild(false);
      // ... and now, so that the the workspaces dont appear in the window, we prefix
      // all workspace names with "__".
      loadAlg->setPropertyValue("Filename",fileName);
      loadAlg->setPropertyValue("OutputWorkspace","__" + wsName);
      loadAlg->executeAsSubAlg();

      return AnalysisDataService::Instance().retrieve("__" + wsName);
    }

    /**
     * Plus two workspaces together, "in place".
     *
     * @param ws1 :: The first workspace.
     * @param ws2 :: The second workspace.
     *
     * @returns a pointer to the result (the first workspace).
     */
   API::Workspace_sptr Load::plusWs(
      Workspace_sptr ws1,
      Workspace_sptr ws2)
    {
      Mantid::API::IAlgorithm_sptr plusAlg = createSubAlgorithm("Plus", 1);
      plusAlg->setPropertyValue("LHSWorkspace", ws1->name());
      plusAlg->setPropertyValue("RHSWorkspace", ws2->name());
      plusAlg->setPropertyValue("OutputWorkspace", ws1->name());
      plusAlg->executeAsSubAlg();

      return ws1;
    }

    /**
     * Renames a workspace.
     *
     * @param oldName :: the old workspace name.
     * @param newName :: the new workspace name.
     */
    void Load::renameWs(
      const std::string & oldName, 
      const std::string & newName)
    {
      if(oldName == newName)
        return;

      Mantid::API::IAlgorithm_sptr renameAlg = createSubAlgorithm("RenameWorkspace", 1);
      renameAlg->setChild(false);
      renameAlg->setPropertyValue("InputWorkspace", oldName);
      renameAlg->setPropertyValue("OutputWorkspace", newName);
      renameAlg->executeAsSubAlg();
    }

    /**
     * Deletes a given workspace.  If the given workspace is a group workspace,
     * then this function calls itself recursively for each workspace in the group.
     *
     * @param wsName :: the name of the workspace to delete.
     */
    void Load::deleteWs(const std::string & wsName)
    {
      Workspace_sptr ws = AnalysisDataService::Instance().retrieve(wsName);
      if(WorkspaceGroup_sptr wsGrpSptr =
        boost::dynamic_pointer_cast<WorkspaceGroup>(ws))
      {
        std::vector<std::string> childWsNames = wsGrpSptr->getNames();
        std::vector<std::string>::iterator vIt = childWsNames.begin();

        for(; vIt != childWsNames.end(); ++vIt)
        {
          // Call this function recursively, to delete each child workspace.
          deleteWs(*vIt);
        }
      }
      else
      {
        Mantid::API::IAlgorithm_sptr deleteAlg = createSubAlgorithm("DeleteWorkspace", 1);

        deleteAlg->setPropertyValue("Workspace", wsName);
        deleteAlg->execute();
      }
    }

    /**
     * Unhides a given workspace (by removing the "__" prefix from its name if present).
     * If the given workspace is a group workspace, then this function calls itself 
     * recursively for each workspace in the group.
     *
     * @param wsName :: the name of the workspace to unhide.
     */
    void Load::unhideWs(const std::string & wsName)
    {
      std::set<std::string> adsContents1 = AnalysisDataService::Instance().getObjectNames();

      Workspace_sptr ws = AnalysisDataService::Instance().retrieve(wsName);
      if(WorkspaceGroup_sptr wsGrpSptr =
        boost::dynamic_pointer_cast<WorkspaceGroup>(ws))
      {
        std::vector<std::string> childWsNames = wsGrpSptr->getNames();
        std::vector<std::string>::iterator vIt = childWsNames.begin();

        for(; vIt != childWsNames.end(); ++vIt)
        {
          // Call this function recursively, to unhide each child workspace.
          unhideWs(*vIt);
        }
      }
      else
      {
        if(boost::starts_with(wsName, "__"))
        {
          std::string newName = wsName.substr(2, (wsName.size() - 2));
          renameWs(wsName, newName);
        }
      }
    }

  } // namespace DataHandling
} // namespace Mantid
