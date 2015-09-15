//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/LoadQKK.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Objects/ShapeFactory.h"

#include "MantidNexus/NexusClasses.h"

#include <boost/math/special_functions/fpclassify.hpp>
#include <Poco/File.h>
#include <iostream>
#include <fstream>
#include <iomanip>

using namespace Mantid::DataHandling;
using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadQKK)

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadQKK::confidence(Kernel::NexusDescriptor &descriptor) const {
  const auto &firstEntryName = descriptor.firstEntryNameType().first;
  if (descriptor.pathExists("/" + firstEntryName + "/data/hmm_xy"))
    return 80;
  else
    return 0;
}

/**
 * Initialise the algorithm. Declare properties which can be set before
 * execution (input) or
 * read from after the execution (output).
 */
void LoadQKK::init() {
  // Specify file extensions which can be associated with a QKK file.
  std::vector<std::string> exts;
  exts.push_back(".nx.hdf");
  // Declare the Filename algorithm property. Mandatory. Sets the path to the
  // file to load.
  declareProperty(
      new API::FileProperty("Filename", "", API::FileProperty::Load, exts),
      "The input filename of the stored data");
  // Declare the OutputWorkspace property. This sets the name of the workspace
  // to be filled with the data
  // from the file.
  declareProperty(new API::WorkspaceProperty<>("OutputWorkspace", "",
                                               Kernel::Direction::Output));
}

/**
 * Execute the algorithm.
 */
void LoadQKK::exec() {
  using namespace Mantid::API;
  // Get the name of the file.
  std::string filename = getPropertyValue("Filename");

  // Open the root.
  NeXus::NXRoot root(filename);
  // Open the first NXentry found in the file.
  NeXus::NXEntry entry = root.openFirstEntry();
  // Open NXdata group with name "data"
  NeXus::NXData data = entry.openNXData("data");
  // Read in wavelength value
  double wavelength = static_cast<double>(data.getFloat("wavelength"));
  // open the data set with the counts. It is identified by the signal=1
  // attribute
  NeXus::NXInt hmm = data.openIntData();
  // Read the data into memory
  hmm.load();

  // Get the wavelength spread
  double wavelength_spread = static_cast<double>(
      entry.getFloat("instrument/velocity_selector/wavelength_spread"));
  double wavelength0 = wavelength - wavelength_spread / 2;
  double wavelength1 = wavelength + wavelength_spread / 2;

  // hmm is a 3d array with axes: sample_x, y_pixel_offset, x_pixel_offset
  size_t ny = hmm.dim1(); // second dimension
  size_t nx = hmm.dim2(); // third dimension
  size_t nHist = ny * nx; // number of spectra in the dataset
  if (nHist == 0) {
    throw std::runtime_error("Error in data dimensions: " +
                             boost::lexical_cast<std::string>(ny) + " X " +
                             boost::lexical_cast<std::string>(nx));
  }

  // Set the workspace structure. The workspace will contain nHist spectra each
  // having a single wavelength bin.
  const size_t xWidth = 2; // number of wavelength bin boundaries
  const size_t yWidth = 1; // number of bins

  // Create a workspace with nHist spectra and a single y bin.
  MatrixWorkspace_sptr outputWorkspace =
      boost::dynamic_pointer_cast<MatrixWorkspace>(
          WorkspaceFactory::Instance().create("Workspace2D", nHist, xWidth,
                                              yWidth));
  // Set the units of the x axis as Wavelength
  outputWorkspace->getAxis(0)->unit() =
      UnitFactory::Instance().create("Wavelength");
  // Set the units of the data as Counts
  outputWorkspace->setYUnitLabel("Counts");

  //  Put the data into outputWorkspace
  size_t count = 0;
  for (size_t i = 0; i < ny; ++i)
    for (size_t j = 0; j < nx; ++j) {
      // Move data across
      double c = hmm(0, int(i), int(j));
      outputWorkspace->dataX(count)[0] = wavelength0;
      outputWorkspace->dataX(count)[1] = wavelength1;
      outputWorkspace->dataY(count)[0] = c;
      outputWorkspace->dataE(count)[0] = sqrt(c);
      ++count;
    }

  // Build instrument geometry

  // Create a new instrument and set its name
  std::string instrumentname = "QUOKKA";
  Geometry::Instrument_sptr instrument(
      new Geometry::Instrument(instrumentname));
  outputWorkspace->setInstrument(instrument);

  // Add dummy source and samplepos to instrument

  // Create an instrument component wich will represent the sample position.
  Geometry::ObjComponent *samplepos =
      new Geometry::ObjComponent("Sample", instrument.get());
  instrument->add(samplepos);
  instrument->markAsSamplePos(samplepos);
  // Put the sample in the centre of the coordinate system
  samplepos->setPos(0.0, 0.0, 0.0);

  // Create a component to represent the source
  Geometry::ObjComponent *source =
      new Geometry::ObjComponent("Source", instrument.get());
  instrument->add(source);
  instrument->markAsSource(source);

  // Read in the L1 value and place the source at (0,0,-L1)
  double l1 = static_cast<double>(entry.getFloat("instrument/parameters/L1"));
  source->setPos(0.0, 0.0, -1.0 * l1);

  // Create a component for the detector.

  // We assumed that these are the dimensions of the detector, and height is in
  // y direction and width is in x direction
  double height =
      static_cast<double>(entry.getFloat("instrument/detector/active_height"));
  double width =
      static_cast<double>(entry.getFloat("instrument/detector/active_width"));
  // Convert them to metres
  height /= 1000;
  width /= 1000;

  // We assumed that individual pixels have the same size and shape of a cuboid
  // with dimensions:
  double pixel_height = height / static_cast<double>(ny);
  double pixel_width = width / static_cast<double>(nx);
  // Create size strings for shape creation
  std::string pixel_height_str =
      boost::lexical_cast<std::string>(pixel_height / 2);
  std::string pixel_width_str =
      boost::lexical_cast<std::string>(pixel_width / 2);
  // Set the depth of a pixel to a very small number
  std::string pixel_depth_str = "0.00001";

  // Create a RectangularDetector which represents a rectangular array of pixels
  Geometry::RectangularDetector *bank =
      new Geometry::RectangularDetector("bank", instrument.get());
  // Define shape of a pixel as an XML string. See
  // http://www.mantidproject.org/HowToDefineGeometricShape for details
  // on shapes in Mantid.
  std::string detXML =
      "<cuboid id=\"pixel\">"
      "<left-front-bottom-point   x= \"" +
      pixel_width_str + "\" y=\"-" + pixel_height_str +
      "\" z=\"0\"  />"
      "<left-front-top-point      x= \"" +
      pixel_width_str + "\" y=\"-" + pixel_height_str + "\" z=\"" +
      pixel_depth_str + "\"  />"
                        "<left-back-bottom-point    x=\"-" +
      pixel_width_str + "\" y=\"-" + pixel_height_str +
      "\" z=\"0\"  />"
      "<right-front-bottom-point  x= \"" +
      pixel_width_str + "\" y= \"" + pixel_height_str + "\" z=\"0\"  />"
                                                        "</cuboid>";
  // Create a shape object which will be shared by all pixels.
  Geometry::Object_sptr shape = Geometry::ShapeFactory().createShape(detXML);
  // Initialise the detector specifying the sizes.
  bank->initialize(shape, int(nx), 0, pixel_width, int(ny), 0, pixel_height, 1,
                   true, int(nx));
  for (int i = 0; i < static_cast<int>(ny); ++i)
    for (int j = 0; j < static_cast<int>(nx); ++j) {
      instrument->markAsDetector(bank->getAtXY(j, i).get());
    }
  // Position the detector so the z axis goes through its centre
  bank->setPos(-width / 2, -height / 2, 0);

  // Set the workspace title
  outputWorkspace->setTitle(entry.getString("experiment/title"));
  // Attach the created workspace to the OutputWorkspace property. The workspace
  // will also be saved in AnalysisDataService
  // and can be retrieved by its name.
  setProperty("OutputWorkspace", outputWorkspace);
}

} // namespace
} // namespace
