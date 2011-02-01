//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/Load.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/LoadAlgorithmFactory.h"
#include<algorithm>


namespace Mantid
{
  namespace DataHandling
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(Load);

    using namespace Kernel;
    using namespace API;

    /// Default constructor
    Load::Load() : IDataFileChecker(), m_header_buffer(NULL) 
    {
    }

    /**
     * Quick file always returns false here
     * @param filePath File path
     * @param nread Number of bytes read
     * @param header_buffer A buffer containing the nread bytes
     */
    bool Load::quickFileCheck(const std::string& filePath,size_t nread,const file_header& header)
    {
      (void)filePath; (void)nread; (void)header;
      return false;
    }

    /**
     * File check by looking at the structure of the data file
     * @param filePath The full file path
     * @returns -1
     */
    int Load::fileCheck(const std::string& filePath)
    {
      (void)filePath;
      return -1;
    }

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
      declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace", "",Direction::Output, true), 
		      "The name of the workspace that will be created, filled with the\n"
		      "read-in data and stored in the Analysis Data Service.");
      declareProperty("FindLoader", false, "If true the algorithm will only be run as\n"
 		      "far as is necessary to discover the concrete Load algorithm to use");
      declareProperty("LoaderName", std::string(""), "A string containing the name of the concrete loader used", 
 		      Direction::Output);
    }

    namespace 
    {
      //@cond
      /** 
       * Checks this property exists in the list of algorithm properties.
       */
      struct hasProperty
      { 
	/** constructor which takes 1 arguement.
	 *@param name :: name of the property
	 */
	hasProperty(const std::string name):m_name(name){}
	
	/**This method comapres teh property name
	 *@param prop :: shared pointer to property
	 *@return true if the property exists in the list of properties.
	 */
	bool operator()(Mantid::Kernel::Property* prop)
	{
	  std::string name=prop->name();
	  return (!name.compare(m_name));
	}
	/// name of teh property
	std::string m_name;
      };

      //@endcond
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
      //get the shared pointer to the specialised load algorithm to execute  
      API::IAlgorithm_sptr loader = getFileLoader(fileName);
      if( !loader )
      {
        throw std::runtime_error("Cannot find a loader for \"" + fileName + "\"");
      }
      else
      {
	setPropertyValue("LoaderName", loader->name());
      }
      
      bool findOnly = getProperty("FindLoader");
      if( findOnly ) return;

      g_log.information()<<"The sub load algorithm  created to execute is "
			 << loader->name() << "  and it version is  " 
			 << loader->version() <<std::endl;
      double startProgress(0.0), endProgress(1.0);
      initialiseLoadSubAlgorithm(loader,startProgress,endProgress,true,-1);
  
      //get the list of properties for this algorithm
      std::vector<Kernel::Property*> props=getProperties();
      ///get the list properties for the sub load algorithm
      std::vector<Kernel::Property*>loader_props=loader->getProperties();
      
      //loop through the properties of this algorithm 
      std::vector<Kernel::Property*>::iterator itr;
      for (itr=props.begin();itr!=props.end();++itr)
      {        
	std::vector<Mantid::Kernel::Property*>::iterator prop;
	//if  the load sub algorithm has the same property then set it.
	prop=std::find_if(loader_props.begin(),loader_props.end(),hasProperty((*itr)->name()));
	if(prop!=loader_props.end())
	{ 
          
	  loader->setPropertyValue((*prop)->name(),getPropertyValue((*prop)->name()));
	}
        
      }
      // Execute the concrete loader
      loader->execute();
      // Set the workspace. Deals with possible multiple periods
      setOutputWorkspace(loader);          
    }

    /** Get a shared pointer to the load algorithm with highest preference for loading
     *@param filePath :: path of the file
     *@return filePath - path of the file
     */
    API::IAlgorithm_sptr Load::getFileLoader(const std::string& filePath)
    {
      /* Open the file and read in the first bufferSize bytes - these will
       * be used to determine the type of the file
       */
      FILE* fp = fopen(filePath.c_str(), "rb");
      if (fp == NULL)
      {
	throw Kernel::Exception::FileError("Unable to open the file:", filePath);
      }
      //unsigned char* header_buffer = header_buffer_union.c;
      //unsigned char * header_buffer = IDataFileChecker::g_hdr_buf.get();
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
      API::IAlgorithm_sptr winningLoader;
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
	throw std::runtime_error("Cannot find a loader for \"" + filePath + "\"");
      }
      return winningLoader;
    }

    /** This method set the algorithm as a child algorithm.
     *  @param alg :: The shared pointer to a  algorithm
     *  @param startProgress :: The percentage progress value of the overall 
     *  algorithm where this child algorithm starts
     *  @param endProgress :: The percentage progress value of the overall 
     *  algorithm where this child algorithm ends
     *  @param enableLogging :: Set to false to disable logging from the child algorithm
     *  @param version ::  The version of the child algorithm to create. 
     *  By default gives the latest version.
     */
    void Load::initialiseLoadSubAlgorithm(API::IAlgorithm_sptr alg, 
					  const double startProgress, const double endProgress, 
					  const bool enableLogging, const int& version)
    {
      (void) version; // Avoid compiler warning.
      //Set as a child so that we are in control of output storage
      alg->initialize();
      alg->setChild(true);
      alg->setLogging(enableLogging);

      // If output workspaces are nameless, give them a temporary name to satisfy validator
      const std::vector< Property*> &props = alg->getProperties();
      for (unsigned int i = 0; i < props.size(); ++i)
      {
	if (props[i]->direction() == 1 && dynamic_cast<IWorkspaceProperty*>(props[i]) )
	{
	  if ( props[i]->value().empty() ) props[i]->setValue("LoadChildWorkspace");
	}
      }

      if (startProgress >= 0. && endProgress > startProgress && endProgress <= 1.)
      {
	alg->addObserver(m_progressObserver);
	alg->setChildStartProgress(startProgress);
	alg->setChildEndProgress(endProgress);
      }

    }
        
    /**
     * Set the output workspace(s) if the load's return workspace
     *  has type API::Workspace
     *@param load :: shared pointer to load algorithm
     */
    void Load::setOutputWorkspace(API::IAlgorithm_sptr& load)
    {
      try
      {
	Workspace_sptr ws = load->getProperty("OutputWorkspace"); 
	WorkspaceGroup_sptr wsg = boost::dynamic_pointer_cast<WorkspaceGroup>(ws);
	if (wsg)
	{
	  setProperty("OutputWorkspace",ws);
	  std::vector<std::string> names = wsg->getNames();
	  for(size_t i = 0; i < names.size(); ++i)
	  {
	    std::ostringstream propName;
	    propName << "OutputWorkspace_" << (i+1);
	    DataObjects::Workspace2D_sptr memberwsws1 = load->getProperty(propName.str());

	    std::string memberwsName = load->getPropertyValue(propName.str());
	    declareProperty(new WorkspaceProperty<>(propName.str(),memberwsName,Direction::Output));
	    setProperty(propName.str(),boost::dynamic_pointer_cast<MatrixWorkspace>(memberwsws1));
	  }
	}
	else
	{ 
	  setProperty("OutputWorkspace",ws);
	}
      }
      catch(std::runtime_error&)
      {
	MatrixWorkspace_sptr mws=load->getProperty("OutputWorkspace");
	setProperty("OutputWorkspace",boost::dynamic_pointer_cast<Workspace>(mws));
      }
         
     
    }

  
  } // namespace DataHandling
} // namespace Mantid
