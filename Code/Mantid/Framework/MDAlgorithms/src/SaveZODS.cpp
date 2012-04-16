/*WIKI*

Saves a HDF5 file to the ZODS (Zurich Oak Ridge Disorder Simulation program) format.
This format consists of a slice of a [[MDHistoWorkspace]] and some information about its location.

'''You must be in HKL space for the output of this algorithm to make sense!'''

From http://www.crystal.mat.ethz.ch/research/DiffuseXrayScattering:
* In the frame of an international cooperation between our group, the University of Zurich and Oak Ridge National Laboratories, we are further developing Monte Carlo simulation techniques for modeling disordered crystal structures. A major goal of this project is to develop the Monte Carlo simulation computer program ZODS (Zurich Oak Ridge Disorder Simulation program), which is designed to be not only user friendly, but also fast and flexible. Special focus is on the efficient use of modern super-computer architectures and of multi-core desktop computers to take full advantage of current trends in computing technologies.

==== Summary of data format ====

In general it contains collection of grids with intensities and each grid is described by specifying origin, size of grid (in each direction) and grid base vectors (a1,a2,a3) so a point at node (i,j,k) of grid has coordinates r = origin+i*a1+j*a2+k*a3.

Please contact Michal Chodkiewicz (michal.chodkiewicz@gmail.com); Vickie Lynch (vel@ornl.gov) for more details.

==== Description of data fields ====

* The CoordinateSystem data object has the attribute "isLocal".
** If '''isLocal'''=1, then we are in HKL space:
*** The '''direction_1''' vector (0.04,0,0) represents a step of 0.04 in the (1,0,0) direction (a*) in reciprocal space.
*** '''This is currently the only mode in which SaveZODS saves'''.
** If '''isLocal'''=0, then we are in Q-space.
*** The '''direction_1''' vector (0.04,0,0) represents a step of 0.04 Angstrom^(-1) in X direction of Cartesian coordinate system (with X colinear with a and Z with c*)
*** In this case CoordinateSystem has additional attribute UnitCell, which is a vector with 6 components (a,b,c,alpha,beta,gamma) a,b,c in angstroms and angles in degrees.
* The '''origin''' field gives the origin of the center of the (0,0,0) cell; in HKL space for our case of isLocal=1.
* The '''size''' field gives the number of bins in each direction.
* The '''data''' field contains the actual intensity at each bin.
** The grid of points (r = origin+i*a1+j*a2+k*a3) specifies the centers of histogram, not the corners.

*WIKI*/

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
    this->setWikiSummary("Save a [[MDHistoWorkspace]] in HKL space to a HDF5 format for use with the ZODS analysis software.");
    this->setOptionalMessage("Save a MDHistoWorkspace in HKL space to a HDF5 format for use with the ZODS analysis software.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void SaveZODS::init()
  {
    declareProperty(new WorkspaceProperty<IMDHistoWorkspace>("InputWorkspace","",Direction::Input),
        "An input MDHistoWorkspace in HKL space.");

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

    if (ws->getDimension(0)->getName() != "H")
      g_log.warning() << "SaveZODS expects the workspace to be in HKL space! Saving anyway..." << std::endl;

    // Create a HDF5 file
    ::NeXus::File * file = new ::NeXus::File(Filename, NXACC_CREATE5);

    // ----------- Coordinate system -----------
    uint32_t isLocal = 1;
    file->makeGroup("CoordinateSystem", "NXgroup", true);
    file->putAttr("isLocal", isLocal);

    if (ws->getNumExperimentInfo() > 0)
    {
      ExperimentInfo_const_sptr ei = ws->getExperimentInfo(0);
      if (ei)
      {
        if (ei->sample().hasOrientedLattice())
        {
          std::vector<double> unitCell;
          const OrientedLattice & latt = ei->sample().getOrientedLattice();
          unitCell.push_back(latt.a());
          unitCell.push_back(latt.b());
          unitCell.push_back(latt.c());
          unitCell.push_back(latt.alpha());
          unitCell.push_back(latt.beta());
          unitCell.push_back(latt.gamma());

          // Now write out the 6D vector
          std::vector<int> unit_cell_size(1, 6);
          file->writeData("UnitCell", unitCell, unit_cell_size);
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
    for (size_t d=0; d<3; d++)
    {
      IMDDimension_const_sptr dim = ws->getDimension(d);
      std::vector<double> direction(3, 0.0);
      direction[d] = dim->getBinWidth();
      // Origin of the CENTER of the first bin
      origin[d] = dim->getMinimum() + dim->getBinWidth()/2;
      // Size in each dimension (reverse order for RANK)
      size[2-d] = int(dim->getNBins());
      // Size in each dimension (in the XYZ order)
      size_field[d] = int(dim->getNBins());
      file->writeData("direction_" + Strings::toString(d+1), direction);
    }
    file->writeData("origin", origin);
    file->writeData("size", size_field);

    // Copy data into a vector
    signal_t * signal = ws->getSignalArray();
    std::vector<double> data;
    data.insert(data.begin(), signal, signal+numPoints);
    file->writeData("Data", data, size);

    // Copy errors (not squared) into a vector called sigma
    signal_t * errorSquared = ws->getErrorSquaredArray();
    std::vector<double> sigma;
    sigma.reserve(numPoints);
    for (size_t i=0; i<numPoints; i++)
      sigma.push_back( sqrt(errorSquared[i]) );
    file->writeData("sigma", sigma, size);

    // Close Data_0 group
    file->closeGroup();
    // Close Data group
    file->closeGroup();

    file->close();
  }



} // namespace Mantid
} // namespace MDAlgorithms
