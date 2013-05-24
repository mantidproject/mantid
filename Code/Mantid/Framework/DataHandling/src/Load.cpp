/*WIKI* 

The Load algorithm is a more intelligent algorithm than most other load algorithms. When passed a filename it attempts to search the existing load [[:Category:Algorithms|algorithms]] and find the most appropriate to load the given file. The specific load algorithm is then run as a child algorithm with the exception that it logs messages to the Mantid logger.

==== Specific Load Algorithm Properties ====

Each specific loader will have its own properties that are appropriate to it:  SpectrumMin and SpectrumMax for ISIS RAW/NeXuS, FilterByTof_Min and FilterByTof_Max for Event data. The Load algorithm cannot know about these properties until it has been told the filename and found the correct loader. Once this has happened the properties of the specific Load algorithm are redeclared on to that copy of Load.

*WIKI*/
/*WIKI_USAGE_NO_SIGNATURE*
==== Python ====
Given the variable number and types of possible arguments that Load can take, its simple Python function cannot just list the properties as arguments like the others do. Instead the Python function <code>Load</code> can handle any number of arguments. The OutputWorkspace and Filename arguments are the exceptions in that they are always checked for. A snippet regarding usage from the <code>help(Load)</code> is shown below
<div style="border:1pt dashed blue; background:#f9f9f9;padding: 1em 0;">
<source lang="python">
# Simple usage, ISIS NeXus file
Load('INSTR00001000.nxs', OutputWorkspace='run_ws')

# ISIS NeXus with SpectrumMin and SpectrumMax = 1
Load('INSTR00001000.nxs', OutputWorkspace='run_ws', SpectrumMin=1, SpectrumMax=1)

# SNS Event NeXus with precount on
Load('INSTR_1000_event.nxs', OutputWorkspace='event_ws', Precount=True)

# A mix of keyword and non-keyword is also possible
Load(OutputWorkspace='event_ws', Filename='INSTR_1000_event.nxs', Precount=True)
</source></div>
==== Loading Multiple Files ====

Loading multiple files is also possible with <code>Load</code>, as well as workspace addition.  For more information, see [[MultiFileLoading]].
*WIKI_USAGE_NO_SIGNATURE*/
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

#include <Poco/Path.h>

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
      std::vector<std::vector<std::string> >::const_iterator first = fileNames.begin();
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
   * @returns a string containing a suggested ws name based on the given run numbers.
   */
  std::string generateWsNameFromRuns(std::vector<unsigned int> runs)
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

  /**
   * Helper function that takes a vector of filenames, and generates a suggested workspace name.
   *
   * @param filenames :: a vector of filenames.
   *
   * @returns a string containing a suggested ws name based on the given file names.
   */
  std::string generateWsNameFromFileNames(std::vector<std::string> filenames)
  {
    std::string wsName("");

    for(size_t i = 0; i < filenames.size(); ++i)
    {
      if(!wsName.empty())
        wsName += "_";

      Poco::Path path(filenames[i]);
      wsName += path.getBaseName();
    }

    return wsName;
  }

  /**
   * Helper function that takes a vector of vectors of items and flattens it into
   * a single vector of items.
   */
  std::vector<std::string> flattenVecOfVec(std::vector<std::vector<std::string> > vecOfVec)
  {
    std::vector<std::string> flattenedVec;

    std::vector<std::vector<std::string> >::const_iterator it = vecOfVec.begin();

    for(; it != vecOfVec.end(); ++it)
    {
      flattenedVec.insert(
        flattenedVec.end(),
        it->begin(), it->end());
    }

    return flattenedVec;
  }
}

namespace Mantid
{
  namespace DataHandling
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(Load);
    
    // The mutex
    Poco::Mutex Load::m_mutex;

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
        std::vector<std::string> fileNames = flattenVecOfVec(getProperty("Filename"));
        // If it's a single file load, then it's fine to change loader.
        if(fileNames.size() == 1)
        {
          IDataFileChecker_sptr loader = getFileLoader(getPropertyValue(name));
          assert(loader); // (getFileLoader should throw if no loader is found.)
          declareLoaderProperties(loader);
        }

        // Else we've got multiple files, and must enforce the rule that only one type of loader is allowed.
        // Allowing more than one would mean that "extra" properties defined by the class user (for example
        // "LoadLogFiles") are potentially ambiguous.
        else if(fileNames.size() > 1)
        {
          IDataFileChecker_sptr loader = getFileLoader(fileNames[0]);
          
          // If the first file has a loader ...
          if( loader )
          {
            // ... store it's name and version and check that all other files have loaders with the same name and version.
            std::string name = loader->name();
            int version = loader->version();
            for(size_t i = 1; i < fileNames.size(); ++i)
            {
              loader = getFileLoader(fileNames[i]);

              if( name != loader->name() || version != loader->version() )
                throw std::runtime_error("Cannot load multiple files when more than one Loader is needed.");
            }
          }

          assert(loader); // (getFileLoader should throw if no loader is found.)
          declareLoaderProperties(loader);
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

      Poco::Mutex::ScopedLock lock( m_mutex );

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
      // THIS IS A COPY as the properties are mutated as we move through them
      const std::vector<Property*> existingProps = this->getProperties();
      for( size_t i = 0; i < existingProps.size(); ++i )
      {
        const std::string name = existingProps[i]->name();
        // Wipe all properties except the Load native ones
        if( m_baseProps.find(name) == m_baseProps.end() )
        {
          this->removeProperty(name);
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
      exts.push_back(".h5");
      exts.push_back(".hd5");

      declareProperty(new MultipleFileProperty("Filename", exts),
        "The name of the file(s) to read, including the full or relative "
        "path. (N.B. case sensitive if running on Linux). Multiple runs "
        "can be loaded and added together, e.g. INST10,11+12,13.ext");
      declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace", "",Direction::Output), 
        "The name of the workspace that will be created, filled with the "
        "read-in data and stored in the Analysis Data Service. Some algorithms "
        "can created additional OutputWorkspace properties on the fly, e.g. multi-period data.");

      declareProperty("LoaderName", std::string(""), "When an algorithm has been found that will load the given file, its name is set here.", 
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

      // Loop through and set the properties on the Child Algorithm
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
      // allFilenames contains "rows" of filenames. If the row has more than 1 file in it
      // then that row is to be summed across each file in the row
      const std::vector<std::vector<std::string> > allFilenames = getProperty("Filename");
      std::string outputWsName = getProperty("OutputWorkspace");

      std::vector<std::string> wsNames(allFilenames.size());
      std::transform(allFilenames.begin(), allFilenames.end(),
                     wsNames.begin(), generateWsNameFromFileNames);


      std::vector<std::vector<std::string> >::const_iterator filenames = allFilenames.begin();
      std::vector<std::string >::const_iterator wsName = wsNames.begin();
      assert( allFilenames.size() == wsNames.size() );

      std::vector<API::Workspace_sptr> loadedWsList;
      loadedWsList.reserve(allFilenames.size());

      // Cycle through the filenames and wsNames.
      for(; filenames != allFilenames.end(); ++filenames, ++wsName)
      {
        std::vector<std::string>::const_iterator filename = filenames->begin();
        Workspace_sptr sumWS = loadFileToWs(*filename, *wsName);

        ++filename;
        for(; filename != filenames->end(); ++filename)
        {
          Workspace_sptr secondWS = loadFileToWs(*filename,  "__@loadsum_temp@");
          sumWS = plusWs(sumWS, secondWS);
        }

        API::WorkspaceGroup_sptr group = boost::dynamic_pointer_cast<WorkspaceGroup>(sumWS);
        if(group)
        {
          for(size_t i = 0; i < group->size(); ++i )
          {
            Workspace_sptr childWs = group->getItem( i );
            // adding to ADS is the only way to set a name
            AnalysisDataService::Instance().addOrReplace(*wsName + "_" + boost::lexical_cast<std::string>(i + 1), childWs);
          }
        }
        // Add the sum to the list of loaded workspace names.
        loadedWsList.push_back(sumWS);
      }

      // If we only have one loaded ws, set it as the output.
      if(loadedWsList.size() == 1)
      {
        setProperty("OutputWorkspace", loadedWsList[0]);
      }
      // Else we have multiple loaded workspaces - group them and set the group as output.
      else
      {
        API::WorkspaceGroup_sptr group = groupWsList(loadedWsList, wsNames);

        for(size_t i = 0; i < group->size(); ++i)
        {
          Workspace_sptr childWs = group->getItem( i );
          std::string outWsPropName = "OutputWorkspace_" + boost::lexical_cast<std::string>(i + 1);
          std::string childWsName = childWs->name();
          if ( childWsName.empty() )
          {
              childWsName = outputWsName + "_" + boost::lexical_cast<std::string>(i + 1);
          }
          declareProperty(new WorkspaceProperty<Workspace>(outWsPropName, childWsName, Direction::Output));
          setProperty(outWsPropName, childWs);
        }
        setProperty("OutputWorkspace", group);
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
     * Set the loader option for use as a Child Algorithm.
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
    void Load::setOutputWorkspace(const API::IDataFileChecker_sptr & loader)
    {
      // Go through each OutputWorkspace property and check whether we need to make a counterpart here
      const std::vector<Property*> & loaderProps = loader->getProperties();
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
    * @returns A pointer to the OutputWorkspace property of the Child Algorithm
    */
    API::Workspace_sptr Load::getOutputWorkspace(const std::string & propName,
      const API::IDataFileChecker_sptr & loader) const
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

      // General IMDWorkspace
      try
      {
        IMDWorkspace_sptr childWS = loader->getProperty(propName);
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

      g_log.debug() << "Workspace property " << propName << " did not return to MatrixWorkspace, EventWorkspace, IMDEventWorkspace, IMDWorkspace" << std::endl;
      return Workspace_sptr();
    }

    /*
    * Overrides the default cancel() method. Calls cancel() on the actual loader.
    */
    void Load::cancel()
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
    API::Workspace_sptr Load::loadFileToWs(const std::string & fileName, const std::string & wsName)
    {
      Mantid::API::IAlgorithm_sptr loadAlg = createChildAlgorithm("Load", 1);

      // Get the list properties for the concrete loader load algorithm
      const std::vector<Kernel::Property*> & props = getProperties();

      // Loop through and set the properties on the Child Algorithm
      std::vector<Kernel::Property*>::const_iterator prop = props.begin();
      for (; prop != props.end(); ++prop)
      {
        const std::string & propName = (*prop)->name();

        if( this->existsProperty(propName) )
        {
          if(propName == "Filename")
          {
            loadAlg->setPropertyValue("Filename",fileName);
          }
          else if(propName == "OutputWorkspace")
          {
            loadAlg->setPropertyValue("OutputWorkspace", wsName);
          }
          else
          {
            loadAlg->setPropertyValue(propName, getPropertyValue(propName));
          }
        }
      }

      loadAlg->executeAsChildAlg();

      Workspace_sptr ws = loadAlg->getProperty("OutputWorkspace");
      //ws->setName(wsName);
      return ws;
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
      WorkspaceGroup_sptr group1 = boost::dynamic_pointer_cast<WorkspaceGroup>(ws1);
      WorkspaceGroup_sptr group2 = boost::dynamic_pointer_cast<WorkspaceGroup>(ws2);

      if( group1 && group2 )
      {
        // If we're dealing with groups, then the child workspaces must be added separately - setProperty
        // wont work otherwise.
        std::vector<std::string> group1ChildWsNames = group1->getNames();
        std::vector<std::string> group2ChildWsNames = group2->getNames();

        if( group1ChildWsNames.size() != group2ChildWsNames.size() )
          throw std::runtime_error("Unable to add group workspaces with different number of child workspaces.");

        auto group1ChildWsName = group1ChildWsNames.begin();
        auto group2ChildWsName = group2ChildWsNames.begin();

        for( ; group1ChildWsName != group1ChildWsNames.end(); ++group1ChildWsName, ++group2ChildWsName )
        {
          Workspace_sptr group1ChildWs = group1->getItem(*group1ChildWsName);
          Workspace_sptr group2ChildWs = group2->getItem(*group2ChildWsName);

          Mantid::API::IAlgorithm_sptr plusAlg = createChildAlgorithm("Plus", 1);
          plusAlg->setProperty<Workspace_sptr>("LHSWorkspace", group1ChildWs);
          plusAlg->setProperty<Workspace_sptr>("RHSWorkspace", group2ChildWs);
          plusAlg->setProperty<Workspace_sptr>("OutputWorkspace", group1ChildWs);
          plusAlg->executeAsChildAlg();
        }
      }
      else if( ! group1 && ! group2 )
      {
        Mantid::API::IAlgorithm_sptr plusAlg = createChildAlgorithm("Plus", 1);
        plusAlg->setProperty<Workspace_sptr>("LHSWorkspace", ws1);
        plusAlg->setProperty<Workspace_sptr>("RHSWorkspace", ws2);
        plusAlg->setProperty<Workspace_sptr>("OutputWorkspace", ws1);
        plusAlg->executeAsChildAlg();
      }
      else
      {
        throw std::runtime_error("Unable to add a group workspace to a non-group workspace");
      }

      return ws1;
    }

    /**
     * Groups together a vector of workspaces.  This is done "manually", since the
     * workspaces being passed will be outside of the ADS and so the GroupWorkspaces
     * alg is not an option here.
     *
     * @param wsList :: the list of workspaces to group
     */
    API::WorkspaceGroup_sptr Load::groupWsList(const std::vector<API::Workspace_sptr> & wsList, const std::vector<std::string> &wsNames)
    {
      assert( wsList.size() == wsNames.size() );
      WorkspaceGroup_sptr group = WorkspaceGroup_sptr(new WorkspaceGroup);

      auto ws = wsList.begin();
      auto wsName = wsNames.begin();
      for( ; ws != wsList.end(); ++ws, ++wsName )
      {
        WorkspaceGroup_sptr isGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(*ws);
        // If the ws to add is already a group, then add its children individually.
        if(isGroup)
        {
            for(size_t i = 0; i < isGroup->size(); ++i)
            {
                auto item = isGroup->getItem(i);
                // have to add item to ADS if we want to give it a specific name
                AnalysisDataService::Instance().addOrReplace(*wsName + "_" + boost::lexical_cast<std::string>(i+1), item);
                group->addWorkspace( item );
            }
        }
        else
        {
          group->addWorkspace(*ws);
        }
      }

      return group;
    }

  } // namespace DataHandling
} // namespace Mantid
