/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidMDAlgorithms/SaveZODS.h"
#include "MantidMDEvents/MDHistoWorkspace.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::MDEvents;

namespace Mantid
{
namespace MDAlgorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(SaveZODS)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SaveZODS::SaveZODS()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SaveZODS::~SaveZODS()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string SaveZODS::name() const { return "SaveZODS";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int SaveZODS::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string SaveZODS::category() const { return "MDAlgorithms";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void SaveZODS::initDocs()
  {
    this->setWikiSummary("Save a [[MDHistoWorkspace]] to a HDF5 format for use with the ZODS analysis software.");
    this->setOptionalMessage("Save a MDHistoWorkspace to a HDF5 format for use with the ZODS analysis software.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void SaveZODS::init()
  {
    declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("InputWorkspace","",Direction::Input),
        "An input MDHistoWorkspace.");

    std::vector<std::string> exts;
    exts.push_back(".h5");
    declareProperty(new FileProperty("Filename", "", FileProperty::Save, exts),
        "The name of the HDF5 file to write, as a full or relative path.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SaveZODS::exec()
  {
    IMDHistoWorkspace_sptr inWS = getProperty("InputWorkspace");
    std::string Filename = getPropertyValue("Filename");
    MDHistoWorkspace_sptr ws = boost::dynamic_pointer_cast<MDHistoWorkspace>(inWS);
    if (!ws)
      throw std::runtime_error("InputWorkspace is not a MDHistoWorkspace");
    if (ws->getNumDims() != 3)
      throw std::runtime_error("InputWorkspace must have 3 dimensions (having one bin in the 3rd dimension is OK).");

    // Create a HDF5 file
    ::NeXus::File * file = new ::NeXus::File(Filename, NXACC_CREATE5);

    // ----------- Coordinate system -----------
    uint32_t isLocal = 0;
    file->makeGroup("CoordinateSystem", "NXgroup", true);
    file->putAttr("isLocal", isLocal);
    file->closeGroup();

    uint64_t numPoints = ws->getNPoints();
    signal_t * signal = ws->getSignalArray();

    file->makeGroup("Data", "NXgroup", true);
    file->makeGroup("Data_0", "NXgroup", true);

    // ----------- Attributes ------------------
    std::vector<double> origin(3, 0.0);

    // Size in each dimension (in the "C" style order, so z,y,x
    // That is, data[z][y][x] = etc.
    std::vector<int> size(3, 0);

    // Dimension_X attributes give the step size for each dimension
    for (size_t d=0; d<3; d++)
    {
      IMDDimension_const_sptr dim = ws->getDimension(d);
      std::vector<double> direction(3, 0.0);
      direction[d] = dim->getBinWidth();
      // Origin of the CENTER of the first bin
      origin[d] = dim->getMinimum() + dim->getBinWidth()/2;
      // Size in each dimension (reverse order)
      size[2-d] = int(dim->getNBins());
      file->writeData("direction_" + Strings::toString(d+1), direction);
    }
    file->writeData("origin", origin);
    file->writeData("size", size);

    // Copy data into a vector
    std::vector<double> data;
    data.insert(data.begin(), signal, signal+numPoints);
    file->writeData("Data", data, size);

    // Close Data_0 group
    file->closeGroup();
    // Close Data group
    file->closeGroup();




  }



} // namespace Mantid
} // namespace MDAlgorithms
