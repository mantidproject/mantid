/*WIKI*

This algorithm loads a Sassena output file into a group workspace.
It will create a workspace for each scattering intensity and one workspace for the Q-values

*WIKI*/

#include "MantidDataHandling/LoadSassena.h"
#include "MantidAPI/LoadAlgorithmFactory.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include <hdf5.h>
#include <hdf5_hl.h>

namespace Mantid
{
namespace DataHandling
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadSassena)
//register the algorithm into LoadAlgorithmFactory
DECLARE_LOADALGORITHM(LoadSassena)

/// Sets documentation strings for this algorithm
void LoadSassena::initDocs()
{
  this->setWikiSummary("This algorithm loads a Sassena output file into a group workspace.");
  this->setOptionalMessage(" Algorithm to load an NXSPE file into a group workspace.");
  this->setWikiDescription("This algorithm loads a Sassena output file into a group workspace. It will create a workspace for each scattering intensity and one workspace for the Q-values");
}

/**
 * Do a quick file type check by looking at the first 100 bytes of the file
 *  @param filePath :: path of the file including name.
 *  @param nread :: no.of bytes read
 *  @param header :: The first 100 bytes of the file as a union
 *  @return true if the given file is of type which can be loaded by this algorithm
 */
bool LoadSassena::quickFileCheck(const std::string& filePath,size_t nread, const file_header& header)
{
  std::string ext = this->extension(filePath);
  // If the extension is h5 then give it a go
  if( ext.compare("h5") == 0 ) return true;

  // If not then let's see if it is a HDF file by checking for the magic cookie
  if ( nread >= sizeof(int32_t) && (ntohl(header.four_bytes) == g_hdf_cookie) ) return true;

  return false;
}

/**
 * Checks the file by opening it and reading few lines
 *  @param filePath :: name of the file inluding its path
 *  @return an integer value how much this algorithm can load the file
 */
int LoadSassena::fileCheck(const std::string &filePath)
{
  int confidence(0);
  hid_t h5file = H5Fopen(filePath.c_str(),H5F_ACC_RDONLY,H5P_DEFAULT);
  if (H5LTfind_attribute(h5file,"sassena_version")==1) confidence = 99;
  return confidence;
}

/**
 * Initialise the algorithm. Declare properties which can be set before execution (input) or
 * read from after the execution (output).
 */
void LoadSassena::init()
{
  std::vector<std::string> exts; // Specify file extensions which can be associated with an output Sassena file
  exts.push_back(".h5");
  exts.push_back(".hd5");

  // Declare the Filename algorithm property. Mandatory. Sets the path to the file to load.
  this->declareProperty(new API::FileProperty("Filename", "", API::FileProperty::Load, exts),"A Sassena file");
  // Declare the OutputWorkspace property
  this->declareProperty(new API::WorkspaceProperty<>("OutputWorkspace","",Kernel::Direction::Output), "The name of the group workspace to be created.");
}

/**
 * Execute the algorithm.
 */
void LoadSassena::exec()
{
  //this->GWS = API::WorkspaceFactory::Instance().create("WorkspaceGroup");
  // WorkspaceGroup_sptr wsgroup = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(inputs_orig[i]);
  // API::WorkspaceGroup_sptr gws = boost::dynamic_pointer_cast<API::WorkspaceGroup>(rws);
  // MatrixWorkspace_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceFactory::Instance().create("Workspace2D",numSpectra,energies.size(),numBins));

  //populate m_validSets
  int nvalidSets = 5;
  const char* validSets[] = { "fq", "fq0", "fq2", "fqt", "qvectors" };
  this->m_validSets(validSets, validSets+nvalidSets);

  this->m_filename = this->getPropertyValue("Filename");
  hid_t h5file = H5Fopen(this->m_filename.c_str(),H5F_ACC_RDONLY,H5P_DEFAULT);

  //find out the sassena version used
  char *cversion = new char[16];
  H5LTget_attribute_string( h5file, "/", "sassena_version", cversion );
  const std::string version(cversion);
  int iversion=boost::lexical_cast<int>(cversion);

  //determine which loader protocol to use based on the version
  //to be done at a later time, maybe implement a Version class

  //iterate over the valid sets
  for(std::vector<const char*>::const_iterator it=this->m_validSets.begin(); it!=this->m_validSets.end(); ++it){
    if (H5LTfind_dataset(h5file,*it)==1) {
    /*an implementation of the abstract base loadSet class
     *
     */

    }
  }

}



} // endof namespace DataHandling
} // endof namespace Mantid
