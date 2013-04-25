/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidDataHandling/LoadMcStasNexus.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/LoadAlgorithmFactory.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/Unit.h"
#include "MantidNexusCPP/NexusFile.hpp"
#include "MantidNexusCPP/NexusException.hpp"

#include <boost/algorithm/string.hpp>

namespace Mantid
{
namespace DataHandling
{
  using namespace Kernel;
  using namespace API;

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(LoadMcStasNexus);
  DECLARE_LOADALGORITHM(LoadMcStasNexus);


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  LoadMcStasNexus::LoadMcStasNexus()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  LoadMcStasNexus::~LoadMcStasNexus()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string LoadMcStasNexus::name() const { return "LoadMcStasNexus";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int LoadMcStasNexus::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string LoadMcStasNexus::category() const { return "DataHandling";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void LoadMcStasNexus::initDocs()
  {
    this->setWikiSummary("TODO: Enter a quick description of your algorithm.");
    this->setOptionalMessage("TODO: Enter a quick description of your algorithm.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void LoadMcStasNexus::init()
  {
    std::vector<std::string> exts;
    exts.push_back(".h5");
    exts.push_back(".nxs");
    declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
        "The name of the Nexus file to load" );
        
    declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output), "An output workspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void LoadMcStasNexus::exec()
  {
    std::string filename = getPropertyValue("Filename");
    g_log.debug() << "Opening file " << filename << std::endl;
        
    ::NeXus::File nxFile(filename);
    auto entries = nxFile.getEntries();
    auto itend = entries.end();
    size_t workspaceCount(0);
    std::string prefix = getPropertyValue("OutputWorkspace");
    WorkspaceGroup_sptr outputGroup(new WorkspaceGroup);
    
    for(auto it = entries.begin(); it != itend; ++it)
    {
       std::string name = it->first;
       std::string type = it->second;
       nxFile.openGroup(name, type);
       auto dataEntries = nxFile.getEntries();

       for(auto eit = dataEntries.begin(); eit != dataEntries.end(); ++eit)
       {
          std::string dataName = eit->first;
          std::string dataType = eit->second;
          if( dataName == "content_nxs" || dataType != "NXdata" ) continue;
          g_log.debug() << "Opening " << dataName << "   " << dataType << std::endl;
          
          nxFile.openGroup(dataName, dataType);

          // Find the axis names
          auto nxdataEntries = nxFile.getEntries();
          std::string axis1Name,axis2Name;
          for(auto nit = nxdataEntries.begin(); nit != nxdataEntries.end(); ++nit)
          {
            if(nit->second == "NXparameters") continue;
            nxFile.openData(nit->first);
            if(nxFile.hasAttr("axis") )
            {
        	  int axisNo(0);
        	  nxFile.getAttr("axis", axisNo);
        	  if(axisNo == 1) axis1Name = nit->first;
        	  else if(axisNo==2) axis2Name = nit->first;
        	  else throw std::invalid_argument("Unknown axis number");
            }
            nxFile.closeData();
          }
          
          std::vector<double> axis1Values,axis2Values;
          nxFile.readData<double>(axis1Name,axis1Values);
          nxFile.readData<double>(axis2Name,axis2Values);

		  const size_t axis1Length = axis1Values.size();
		  const size_t axis2Length = axis2Values.size();
		  g_log.debug() << "Axis lengths=" << axis1Length << " " << axis2Length << std::endl;
          
          // Require "data" field
          std::vector<double> data;
          nxFile.readData<double>("data", data);
          
          // Optional errors field
          std::vector<double> errors;
          try
          {
            nxFile.readData<double>("errors", errors);
          }
          catch(::NeXus::Exception&)
          {
            g_log.information() << "Field " << dataName << " contains no error information." << std::endl;
          }
		  
		  MatrixWorkspace_sptr ws = 
		    WorkspaceFactory::Instance().create("Workspace2D", axis2Length, axis1Length, axis1Length);
	      Axis *axis1 = ws->getAxis(0);
	      axis1->title() = axis1Name;
	      // Set caption
	      boost::shared_ptr<Units::Label> lblUnit(new Units::Label);
	      lblUnit->setLabel(axis1Name,"");
	      axis1->unit() = lblUnit;
	      
		  	      
	      Axis *axis2 = new NumericAxis(axis2Length);
	      axis2->title() = axis2Name;
	      // Set caption
	      lblUnit = boost::shared_ptr<Units::Label>(new Units::Label);
	      lblUnit->setLabel(axis2Name,"");
	      axis2->unit() = lblUnit;
	      
	      ws->setYUnit(axis2Name);
	      ws->replaceAxis(1, axis2);
	      
	      for(size_t wsIndex=0; wsIndex < axis2Length; ++wsIndex)
	      {
	         auto &dataY = ws->dataY(wsIndex);
	         auto &dataE = ws->dataE(wsIndex);
	         auto &dataX = ws->dataX(wsIndex);
	      
		      for(size_t j=0; j < axis1Length; ++j)
		      {
		        // Data is stored in column-major order so we are translating to
		        // row major for Mantid
		        const size_t fileDataIndex = j*axis2Length + wsIndex;

	      		dataY[j] = data[fileDataIndex];
	            dataX[j] = axis1Values[j];
	            if(!errors.empty()) dataE[j] = errors[fileDataIndex];
	    	  }
	    	  axis2->setValue(wsIndex, axis2Values[wsIndex]);
	      }          
          // Make Mantid store the workspace in the group         
          outputGroup->addWorkspace(ws);
          
          nxFile.closeGroup();
       }
       nxFile.closeGroup();
    }    

    setProperty("OutputWorkspace", outputGroup);
  }
  
   /**
    * This method does a quick file type check by looking at the first 100 bytes of the file 
    *  @param filePath- path of the file including name.
    *  @param nread :: no.of bytes read
    *  @param header :: The first 100 bytes of the file as a union
    *  @return true if the given file is of type which can be loaded by this algorithm
    */
    bool LoadMcStasNexus::quickFileCheck(const std::string& filePath, size_t nread,const file_header& header)
    {
      // HDF files have magic cookie in the first 4 bytes
      if ((nread >= sizeof(unsigned)) && (ntohl(header.four_bytes) == g_hdf_cookie))
      {
        //hdf
        return true;
      }
      // HDF5 files have a different signature
      else if ( (nread >= sizeof(g_hdf5_signature)) && 
                (!memcmp(header.full_hdr, g_hdf5_signature, sizeof(g_hdf5_signature))) )
      { 
        //hdf5
        return true;
      }
      return false;
    }
    
    /**
     * Checks the file by opening it and reading few lines 
     *  @param filePath :: name of the file inluding its path
     *  @return an integer value how much this algorithm can load the file 
     */
    int LoadMcStasNexus::fileCheck(const std::string& filePath)
    {
      using namespace ::NeXus;
	  // We will look at the first entry and check for a 
	  // simulation class that contains a name attribute with the value=mcstas
	  int confidence(0);
      try
      {
        ::NeXus::File file = ::NeXus::File(filePath);
        auto entries = file.getEntries();
        if(!entries.empty())
        {
          auto firstIt = entries.begin();
          file.openGroup(firstIt->first,firstIt->second);
          file.openGroup("simulation", "NXsimulation");
          file.openData("information");
          
          std::string nameAttrValue;
          file.getAttr("name", nameAttrValue);
          if(boost::iequals(nameAttrValue, "mcstas")) confidence = 80;
          
          file.closeData();
          file.closeGroup();
          file.closeGroup();
        }
      }
      catch(::NeXus::Exception&)
      {
      }
      return confidence;
    }

} // namespace DataHandling
} // namespace Mantid