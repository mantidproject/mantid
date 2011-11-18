/*WIKI* 

Algorithm to load an NXSPE file into a workspace2D. It will create a new instrument, that can be overwritten later by the LoadInstrument algorithm.
*WIKI*/
#include "MantidDataHandling/LoadNXSPE.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/LoadAlgorithmFactory.h"
#include "nexus/NeXusFile.hpp"
#include "nexus/NeXusException.hpp"
#include "MantidNexus/NexusClasses.h"
#include <vector>
#include <map>

namespace Mantid
{
namespace DataHandling
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(LoadNXSPE)
  //register the algorithm into loadalgorithm factory
  DECLARE_LOADALGORITHM(LoadNXSPE)

  using namespace Mantid::Kernel;
  using namespace Mantid::API;


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  LoadNXSPE::LoadNXSPE()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  LoadNXSPE::~LoadNXSPE()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void LoadNXSPE::initDocs()
  {
    this->setWikiSummary("Algorithm to load an NXSPE file into a workspace2D.");
    this->setOptionalMessage(" Algorithm to load an NXSPE file into a workspace2D.");
    this->setWikiDescription("Algorithm to load an NXSPE file into a workspace2D. It will create a new instrument, that can be overwritten later by the LoadInstrument algorithm.");
  }

  //----------------------------------------------------------------------------------------------
  /**
   * Do a quick file type check by looking at the first 100 bytes of the file
   *  @param filePath :: path of the file including name.
   *  @param nread :: no.of bytes read
   *  @param header :: The first 100 bytes of the file as a union
   *  @return true if the given file is of type which can be loaded by this algorithm
   */
  bool LoadNXSPE::quickFileCheck(const std::string& filePath,size_t nread, const file_header& header)
  {
    std::string ext = this->extension(filePath);
    // If the extension is nxs then give it a go
    if( ext.compare("nxspe") == 0 ) return true;

    // If not then let's see if it is a HDF file by checking for the magic cookie
    if ( nread >= sizeof(int32_t) && (ntohl(header.four_bytes) == g_hdf_cookie) ) return true;
    return false;
  }


  /**
   * Checks the file by opening it and reading few lines
   *  @param filePath :: name of the file inluding its path
   *  @return an integer value how much this algorithm can load the file
   */
  int LoadNXSPE::fileCheck(const std::string& filePath)
  {
    int confidence(0);
    typedef std::map<std::string,std::string> string_map_t;
    try
    {
      string_map_t::const_iterator it;
      ::NeXus::File file = ::NeXus::File(filePath);
      string_map_t entries = file.getEntries();
      for(string_map_t::const_iterator it = entries.begin(); it != entries.end(); ++it)
      {
        if (it->second == "NXentry")
        {
          file.openGroup(it->first, it->second);
          file.openData("definition");
          if (file.getStrData().compare("NXSPE")==0) confidence =99;
        }
      }
    }
    catch(::NeXus::Exception&)
    {
    }
    return confidence;
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void LoadNXSPE::init()
  {
    std::vector<std::string> exts;
    exts.push_back(".nxspe");
    exts.push_back("");
    declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
                                       "An NXSPE file");
    declareProperty(new WorkspaceProperty<>("OutputWorkspace",
                                       "",Direction::Output), "The name of the workspace that will be created.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void LoadNXSPE::exec()
  {
    std::string filename = getProperty("Filename");
    //quicly check if it's really nxspe
    try
    {
      ::NeXus::File file(filename);
      std::string mainEntry=(*(file.getEntries().begin())).first;
      file.openGroup(mainEntry, "NXentry");
      file.openData("definition");
      if (file.getStrData().compare("NXSPE")) throw std::invalid_argument("Not NXSPE");
      file.close();
    }
    catch(...)
    {
      throw std::invalid_argument("Not NeXus or notNXSPE");
    }

    //Load the data
    ::NeXus::File file(filename);
    std::string mainEntry=(*(file.getEntries().begin())).first;
    file.openGroup(mainEntry, "NXentry");
    file.openGroup("data", "NXdata");
    ::NeXus::Info info;
    file.openData("data");
    info = file.getInfo();
    std::vector<double> data;
    int numSpectra=info.dims.at(0);
    int numBins=info.dims.at(1);
    file.getData(data);
    file.closeData();
    file.openData("error");
    std::vector<double> error;
    file.getData(error);
    file.closeData();
    file.openData("energy");
    std::vector<double> energies;
    file.getData(energies);
    file.closeData();
    file.openData("azimuthal");
    std::vector<double> azimuthal;
    file.getData(azimuthal);
    file.closeData();
    file.close();

    MatrixWorkspace_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>
        (WorkspaceFactory::Instance().create("Workspace2D",numSpectra,energies.size(),numBins));
    // Need to get hold of the parameter map
    Geometry::ParameterMap& pmap = outputWS->instrumentParameters();
    outputWS->getAxis(0)->unit() = UnitFactory::Instance().create("DeltaE");
    std::vector<double>::iterator itdata=data.begin(),iterror=error.begin(),itdataend,iterrorend;
    API::Progress prog = API::Progress(this, 0.0, 1.0, numSpectra);
    for (int i=0; i<numSpectra; i++)
    {
      itdataend=itdata+numBins;
      iterrorend=iterror+numBins;
      outputWS->dataX(i)=energies;
      if (((*itdata)==std::numeric_limits<double>::quiet_NaN())||(*itdata<=-1e10))//masked bin
      {
        outputWS->dataY(i)=std::vector<double>(numBins,0);
        outputWS->dataE(i)=std::vector<double>(numBins,0);
        pmap.addBool(outputWS->getDetector(i).get(),"masked",true);
      }
      else
      {
        outputWS->dataY(i)=std::vector<double>(itdata,itdataend);
        outputWS->dataE(i)=std::vector<double>(iterror,iterrorend);
      }
      itdata=(itdataend);
      iterror=(iterrorend);
      prog.report();
    }

    setProperty("OutputWorkspace", outputWS);
  }



} // namespace Mantid
} // namespace DataHandling

