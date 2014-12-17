#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidMDAlgorithms/SaveZODS.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::MDEvents;

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveZODS)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
SaveZODS::SaveZODS() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
SaveZODS::~SaveZODS() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string SaveZODS::name() const { return "SaveZODS"; };

/// Algorithm's version for identification. @see Algorithm::version
int SaveZODS::version() const { return 1; };

/// Algorithm's category for identification. @see Algorithm::category
const std::string SaveZODS::category() const { return "MDAlgorithms"; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SaveZODS::init() {
  declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("InputWorkspace", "",
                                                           Direction::Input),
                  "An input MDHistoWorkspace in HKL space.");

  std::vector<std::string> exts;
  exts.push_back(".h5");
  declareProperty(
      new FileProperty("Filename", "", FileProperty::Save, exts),
      "The name of the HDF5 file to write, as a full or relative path.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SaveZODS::exec() {
  IMDHistoWorkspace_sptr inWS = getProperty("InputWorkspace");
  std::string Filename = getPropertyValue("Filename");
  MDHistoWorkspace_sptr ws =
      boost::dynamic_pointer_cast<MDHistoWorkspace>(inWS);
  if (!ws)
    throw std::runtime_error("InputWorkspace is not a MDHistoWorkspace");
  if (ws->getNumDims() != 3)
    throw std::runtime_error("InputWorkspace must have 3 dimensions (having "
                             "one bin in the 3rd dimension is OK).");

  if (ws->getDimension(0)->getName() != "[H,0,0]")
    g_log.warning()
        << "SaveZODS expects the workspace to be in HKL space! Saving anyway..."
        << std::endl;

  // Create a HDF5 file
  ::NeXus::File *file = new ::NeXus::File(Filename, NXACC_CREATE5);

  // ----------- Coordinate system -----------
  uint32_t isLocal = 1;
  file->makeGroup("CoordinateSystem", "NXgroup", true);
  file->putAttr("isLocal", isLocal);

  if (ws->getNumExperimentInfo() > 0) {
    ExperimentInfo_const_sptr ei = ws->getExperimentInfo(0);
    if (ei) {
      if (ei->sample().hasOrientedLattice()) {
        std::vector<double> unitCell;
        const OrientedLattice &latt = ei->sample().getOrientedLattice();
        unitCell.push_back(latt.a());
        unitCell.push_back(latt.b());
        unitCell.push_back(latt.c());
        unitCell.push_back(latt.alpha());
        unitCell.push_back(latt.beta());
        unitCell.push_back(latt.gamma());

        // Now write out the 6D vector
        std::vector<int> unit_cell_size(1, 6);
        file->writeData("unit_cell", unitCell, unit_cell_size);
      }
    }
  }

  file->closeGroup();

  uint64_t numPoints = ws->getNPoints();

  file->makeGroup("Data", "NXgroup", true);
  file->makeGroup("Data_0", "NXgroup", true);

  // ----------- Attributes ------------------
  std::vector<double> origin(3, 0.0);

  // Size in each dimension (in the "C" style order, so z,y,x
  // That is, data[z][y][x] = etc.
  std::vector<int> size(3, 0);

  // And this will be the "size" field we save, in the usual XYZ order.
  std::vector<int> size_field(3, 0);

  // Dimension_X attributes give the step size for each dimension
  for (size_t d = 0; d < 3; d++) {
    IMDDimension_const_sptr dim = ws->getDimension(d);
    std::vector<double> direction(3, 0.0);
    direction[d] = dim->getBinWidth();
    // Origin of the CENTER of the first bin
    origin[d] = dim->getMinimum() + dim->getBinWidth() / 2;
    // Size in each dimension (reverse order for RANK)
    size[2 - d] = int(dim->getNBins());
    // Size in each dimension (in the XYZ order)
    size_field[d] = int(dim->getNBins());
    file->writeData("direction_" + Strings::toString(d + 1), direction);
  }
  file->writeData("origin", origin);
  file->writeData("size", size_field);

  // Copy data into a vector
  signal_t *signal = ws->getSignalArray();
  std::vector<double> data;

  for (int i = 0; i < size_field[0]; i++)
    for (int j = 0; j < size_field[1]; j++)
      for (int k = 0; k < size_field[2]; k++) {
        int l = i + size_field[0] * j + size_field[0] * size_field[1] * k;
        data.push_back(signal[l]);
      }

  file->writeData("Data", data, size);

  // Copy errors (not squared) into a vector called sigma
  signal_t *errorSquared = ws->getErrorSquaredArray();
  std::vector<double> sigma;
  sigma.reserve(numPoints);
  for (int i = 0; i < size_field[0]; i++)
    for (int j = 0; j < size_field[1]; j++)
      for (int k = 0; k < size_field[2]; k++) {
        int l = i + size_field[0] * j + size_field[0] * size_field[1] * k;
        sigma.push_back(sqrt(errorSquared[l]));
      }
  file->writeData("sigma", sigma, size);

  // Close Data_0 group
  file->closeGroup();
  // Close Data group
  file->closeGroup();

  file->close();
}

} // namespace Mantid
} // namespace MDAlgorithms
