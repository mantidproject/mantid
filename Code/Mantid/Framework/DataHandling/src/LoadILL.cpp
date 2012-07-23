/*WIKI* 

Loads the file given into a [[Workspace2D]] with the given name. The file should be in the SPE format, which is described [[Media:Spe_file_format.pdf|here]].
The workspace will have X units of [[Unit_Factory|Energy transfer]]. The other axis will be binned and have units of either [[Unit_Factory|Momentum transfer / Q]] or degrees, depending on the label in the input file. The workspace will be flagged as a distribution.


*WIKI*/
//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/LoadILL.h"
#include "MantidAPI/FileProperty.h"
//#include "MantidKernel/UnitFactory.h"
//#include "MantidAPI/NumericAxis.h"
//#include "MantidDataObjects/Histogram1D.h"
#include "MantidAPI/LoadAlgorithmFactory.h"

namespace Mantid
{
namespace DataHandling
{

using namespace Kernel;
using namespace API;
using namespace NeXus;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadILL)

//register the algorithm into loadalgorithm factory
DECLARE_LOADALGORITHM(LoadILL)

/// Sets documentation strings for this algorithm
void LoadILL::initDocs()
{
  this->setWikiSummary("Loads a ILL nexus file. ");
  this->setOptionalMessage("Loads a ILL nexus file.");
}


//---------------------------------------------------
// Private member functions
//---------------------------------------------------

/**
 * Initialise the algorithm
 */
void LoadILL::init()
{
  declareProperty(new FileProperty("Filename","", FileProperty::Load, ".nxs"),
                  "Name of the SPE file to load" );
  declareProperty(new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
    "The name to use for the output workspace" );
}

/**
 * Execute the algorithm
 */
void LoadILL::exec()
{
  // Retrieve filename and try to open the file
  m_filename = getPropertyValue("Filename");

  // open the root node
  NXRoot root(m_filename);
  // find the first entry
  NXEntry entry = root.openFirstEntry();
  std::cerr << "Entry " << entry.name() << std::endl;
  // find out the instrument name. If other instruments at ILL have this format
  // I assume their instrument group will have the name of the instrument
  const std::string instrName = "IN5";

  NXData dataGroup = entry.openNXData("data");
  MatrixWorkspace_sptr workspace;
  loadData( dataGroup, workspace );

  // Set the output workspace property
  setProperty("OutputWorkspace", workspace);
}

/**
 * Load the counts.
 * @param data :: The dataset with the counts (signal=1).
 * @param workspace :: The workspace to write the data in.
 */
void LoadILL::loadData(NeXus::NXData& dataGroup, API::MatrixWorkspace_sptr& workspace)
{
  NXInt data = dataGroup.openIntData();
  int rank = data.rank();
  std::cerr << "Data: " << rank << ' ' << data.dim0() << ' ' << data.dim1() << ' ' << data.dim2() << std::endl;

  size_t dim0 = static_cast<size_t>( data.dim0() );
  size_t dim1 = static_cast<size_t>( data.dim1() );
  // dim0 * dim1 is the total number of detectors
  size_t nhist = dim0 * dim1;
  // dim2 is the number of time bins
  size_t nbins = data.dim2();

  // Now create the output workspace
  workspace = WorkspaceFactory::Instance().create("Workspace2D",nhist,nbins+1,nbins);
  //workspace->getAxis(0)->unit() = UnitFactory::Instance().create("DeltaE");
  workspace->setYUnitLabel("Counts");

  // put the time of flight data into the x vector
  std::vector<double> tof(nbins + 1);
  // I am making them up because I cannot find them in the file.
  for(auto t = tof.begin(); t != tof.end(); ++t)
  {
    *t = static_cast<double>( t - tof.begin() );
  }
  workspace->dataX(0).assign(tof.begin(), tof.end());

  // load the counts from the file into memory
  data.load();
  size_t spec = 0;
  for(size_t i = 0; i < dim0; ++i)
  for(size_t j = 0; j < dim1; ++j)
  {
    if ( spec > 0 )
    {
      workspace->dataX(spec) = workspace->readX(0);
    }
    int* data_p = &data(static_cast<int>(i), static_cast<int>(j), 0);
    workspace->dataY(spec).assign( data_p, data_p + nbins );
    ++spec;
  }

}


/**This method does a quick file check by checking the no.of bytes read nread params and header buffer
 *  @param filePath- path of the file including name.
 *  @param nread :: no.of bytes read
 *  @param header :: The first 100 bytes of the file as a union
 *  @return true if the given file is of type which can be loaded by this algorithm
 */
bool LoadILL::quickFileCheck(const std::string& filePath,size_t nread,const file_header& header)
{
  std::string extn=extension(filePath);
  bool bnexs(false);
  (!extn.compare("nxs")||!extn.compare("nx5"))?bnexs=true:bnexs=false;
  /*
  * HDF files have magic cookie in the first 4 bytes
  */
  if ( ((nread >= sizeof(unsigned)) && (ntohl(header.four_bytes) == g_hdf_cookie)) || bnexs )
  {
    //hdf
    return true;
  }
  else if ( (nread >= sizeof(g_hdf5_signature)) && 
    (!memcmp(header.full_hdr, g_hdf5_signature, sizeof(g_hdf5_signature))) )
  { 
    //hdf5
    return true;
  }
  return false;
}

/**checks the file by opening it and reading few lines 
 * @param filePath :: name of the file inluding its path
 * @return an integer value how much this algorithm can load the file 
 */
int LoadILL::fileCheck(const std::string& filePath)
{  
  // Create the root Nexus class
  NXRoot root(filePath);
  NXEntry entry = root.openFirstEntry();
  if ( entry.containsGroup("IN5") ) return 80;

  return 0;
}


} // namespace DataHandling
} // namespace Mantid
