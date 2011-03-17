//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/Load.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/LoadAlgorithmFactory.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/FacilityInfo.h"

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

    /**
    * Override setPropertyValue
    * @param name The name of the property
    * @param value The value of the property as a string
    */
    void Load::setPropertyValue(const std::string &name, const std::string &value)
    {
      IDataFileChecker_sptr loader;
      if( name == "Filename" )
      {
        // This call makes resolving the filename easier
        IDataFileChecker::setPropertyValue(name, value);
        loader = getFileLoader(getPropertyValue(name));
      }
      else
      {
        const std::string loaderName = getProperty("LoaderName");
        if( !loaderName.empty() )
        {
          loader = boost::static_pointer_cast<IDataFileChecker>(
            API::AlgorithmManager::Instance().createUnmanaged(loaderName));
          loader->initialize();
        }
      }
      if( loader ) declareLoaderProperties(loader);

      // Set the property after some may have been redeclared
      if( name != "Filename") IDataFileChecker::setPropertyValue(name, value);
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
      int nread = fread(&header,sizeof(unsigned char), g_hdr_bytes, fp);
      // Ensure the character string is null terminated.
      header.full_hdr[g_hdr_bytes] = '\0';
      if (nread == -1)
      {
        fclose(fp);
      }

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

      if( !winningLoader )
      {
        // Clear what may have been here previously
        setPropertyValue("LoaderName", "");
        throw std::runtime_error("Cannot find a loader for \"" + filePath + "\"");
      }
      setPropertyValue("LoaderName", winningLoader->name());
      winningLoader->initialize();
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
        if( m_baseProps.find(name) != m_baseProps.end() ||
            loader->existsProperty(name) )
        {
          continue;
        }
        this->removeProperty(name);
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
          declareProperty(loadProp->clone(), loadProp->documentation());
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
      const FacilityInfo & defaultFacility = Mantid::Kernel::ConfigService::Instance().Facility();
      std::vector<std::string> exts = defaultFacility.extensions();
      // Add in some other known extensions
      exts.push_back(".xml");
      exts.push_back(".dat");
      exts.push_back(".txt");
      exts.push_back(".csv");
      exts.push_back(".spe");

      declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
        "The name of the file to read, including its full or relative\n"
        "path. (N.B. case sensitive if running on Linux).");
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
    *   Executes the algorithm.
    */
    void Load::exec()
    {
      const std::string loaderName = getPropertyValue("LoaderName");
      if( loaderName.empty() )
      {
        throw std::invalid_argument("Cannot find loader, LoaderName property has not been set.");
      }     
      IDataFileChecker_sptr loader = createLoader(loaderName,0,1);
      g_log.information() << "Using " << loaderName << " version " << loader->version() << ".\n";
      ///get the list properties for the concrete loader load algorithm
      const std::vector<Kernel::Property*> & loader_props = loader->getProperties();

      // Loop through and set the properties on the sub algorithm
      std::vector<Kernel::Property*>::const_iterator itr;
      for (itr = loader_props.begin(); itr != loader_props.end(); ++itr)
      {
        const std::string propName = (*itr)->name();
        if( this->existsProperty(propName) )
        {
          loader->setPropertyValue(propName, getPropertyValue(propName));
        }
        else if( propName == loader->filePropertyName() )
        {
          loader->setPropertyValue(propName, getPropertyValue("Filename"));
        }
      }

      // Execute the concrete loader
      loader->execute();
      // Set the workspace. Deals with possible multiple periods
      setOutputWorkspace(loader);
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
      return loader;
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
      {
      }
      // Try a MatrixWorkspace
      try
      {
        MatrixWorkspace_sptr childWS = loader->getProperty(propName);
        return childWS;
      }
      catch(std::runtime_error&)
      {
      }
      // EventWorkspace
      try
      {
        IEventWorkspace_sptr childWS = loader->getProperty(propName);
        return childWS;
      }
      catch(std::runtime_error&)
      {
      }
      return Workspace_sptr();
    }

  } // namespace DataHandling
} // namespace Mantid
