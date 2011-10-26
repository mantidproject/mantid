/*WIKI* 


*WIKI*/
//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/LoadQKK.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/LoadAlgorithmFactory.h"
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

namespace Mantid
{
  namespace DataHandling
  {

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(LoadQKK)

    //register the algorithm into loadalgorithm factory
    DECLARE_LOADALGORITHM(LoadQKK)

    /// Sets documentation strings for this algorithm
    void LoadQKK::initDocs()
    {
      this->setWikiSummary(
          "Loads a ANSTO QKK file. ");
      this->setOptionalMessage(
          "Loads a ANSTO QKK file. ");
    }

    /**
     * Initialise the algorithm
     */
    void LoadQKK::init()
    {
      std::vector<std::string> exts;
      exts.push_back(".nx.hdf");
      declareProperty(new API::FileProperty("Filename", "", API::FileProperty::Load, exts),
          "The input filename of the stored data");
      declareProperty(new API::WorkspaceProperty<>("OutputWorkspace", "", Kernel::Direction::Output));
    }

    /**
     * Execute the algorithm
     */
    void LoadQKK::exec()
    {
      using namespace Mantid::API;
      std::string filename = getPropertyValue("Filename");

      NeXus::NXRoot root(filename);
      NeXus::NXEntry entry = root.openFirstEntry();
      NeXus::NXData data = entry.openNXData("data");
      double wavelength = static_cast<double>(data.getFloat("wavelength"));
      // open the data set with the counts 
      NeXus::NXInt hmm = data.openIntData(); 
      hmm.load();

      // hmm is a 3d array with axes: sample_x, y_pixel_offset, x_pixel_offset
      size_t n1 = hmm.dim1();
      size_t n2 = hmm.dim2();
      size_t nHist = n1 * n2;
      if (nHist == 0)
      {
        throw std::runtime_error("Error in data dimensions: " + boost::lexical_cast<std::string>(n1) + " X " + boost::lexical_cast<std::string>(n2));
      }
      size_t xWidth = 1, yWidth = 1;

      // 2. Create workspace & GSS Files data is always in TOF
      MatrixWorkspace_sptr outputWorkspace = boost::dynamic_pointer_cast<MatrixWorkspace>(
          WorkspaceFactory::Instance().create("Workspace2D", nHist, xWidth, yWidth));
      outputWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("Wavelength");
      outputWorkspace->setYUnitLabel("Counts");

      //  Put data into outputWorkspace
      size_t count = 0;
      for (size_t i = 0; i < n1; ++i)
      for (size_t j = 0; j < n2; ++j)
      {
        // Move data across
        double y = hmm(0,int(i),int(j));
        outputWorkspace->dataX(count)[0] = wavelength;
        outputWorkspace->dataY(count)[0] = y;
        outputWorkspace->dataE(count)[0] = sqrt(y);
        ++count;
      }

      // 2.3 Build instrument geometry

      std::string instrumentname = "QUOKKA";
      // 1. Create a new instrument and set its name
      Geometry::Instrument_sptr instrument(new Geometry::Instrument(instrumentname));
      outputWorkspace->setInstrument(instrument);

      // 3. Add dummy source and samplepos to instrument
      Geometry::ObjComponent *samplepos = new Geometry::ObjComponent("Sample",instrument.get());
      instrument->add(samplepos);
      instrument->markAsSamplePos(samplepos);
      samplepos->setPos(0.0,0.0,0.0);

      Geometry::ObjComponent *source = new Geometry::ObjComponent("Source",instrument.get());
      instrument->add(source);
      instrument->markAsSource(source);

      double l1 = entry.getFloat("instrument/parameters/L1");
      source->setPos(0.0,0.0,-1.0*l1);

      Geometry::RectangularDetector* bank = new Geometry::RectangularDetector("bank",instrument.get());
      std::string detXML =   "<cuboid id=\"s04\">"
        "<left-front-bottom-point   x=\"0.005\" y=\"-0.005\" z=\"0\"  />"
        "<left-front-top-point      x=\"0.005\" y=\"-0.005\" z=\"0.002\"  />"
        "<left-back-bottom-point    x=\"-0.005\" y=\"-0.005\" z=\"0\"  />"
        "<right-front-bottom-point  x=\"0.005\" y=\"0.005\" z=\"0\"  />"
        "</cuboid>";
      Geometry::Object_sptr shape = Geometry::ShapeFactory().createShape(detXML);
      bank->initialize(shape,192,0,0.005,192,0,0.005,1,true,192);
      outputWorkspace->setTitle(entry.getString("experiment/title"));

      setProperty("OutputWorkspace", outputWorkspace);
      return;
    }

    /**This method does a quick file type check by checking the first 100 bytes of the file
     *  @param filePath- path of the file including name.
     *  @param nread :: no.of bytes read
     *  @param header :: The first 100 bytes of the file as a union
     *  @return true if the given file is of type which can be loaded by this algorithm
     */
    bool LoadQKK::quickFileCheck(const std::string& filePath, size_t nread, const file_header& header)
    {
      std::string extn=extension(filePath);
      bool bnexs(false);
      (!extn.compare("nxs")||!extn.compare("nx5")||!extn.compare("nx.hdf"))?bnexs=true:bnexs=false;
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
     *  @param filePath :: name of the file including its path
     *  @return an integer value how much this algorithm can load the file
     */
    int LoadQKK::fileCheck(const std::string& filePath)
    {
      NeXus::NXRoot root(filePath);

      try
      {
        // Open the raw data group 'raw_data_1'
        NeXus::NXEntry entry = root.openFirstEntry();
        if (entry.containsGroup("data"))
        {
          NeXus::NXData data = entry.openNXData("data");
          if (data.getDataSetInfo("hmm_xy").stat != NX_ERROR)
          {
            return 80;
          }
        }
      }
      catch(...)
      {
        return 0;
      }
      return 0;
    }

    /* Create the instrument geometry with Instrument
     *
     */
    //void LoadQKK::createInstrumentGeometry(MatrixWorkspace_sptr workspace, std::string instrumentname, double primaryflightpath,
    //    std::vector<int> detectorids, std::vector<double> totalflightpaths, std::vector<double> twothetas){

    //  // 0. Output information
    //  g_log.information() << "L1 = " << primaryflightpath << std::endl;
    //  if (detectorids.size() != totalflightpaths.size() || totalflightpaths.size() != twothetas.size()){
    //    g_log.debug() << "Input error!  cannot create geometry" << std::endl;
    //    g_log.information() << "Quit!" << std::endl;
    //    return;
    //  }
    //  for (size_t i = 0; i < detectorids.size(); i ++){
    //    g_log.information() << "Detector " << detectorids[i] << "  L1+L2 = " << totalflightpaths[i] << "  2Theta = " << twothetas[i] << std::endl;
    //  }

    //  // 1. Create a new instrument and set its name
    //  Geometry::Instrument_sptr instrument(new Geometry::Instrument(instrumentname));
    //  workspace->setInstrument(instrument);

    //  // 3. Add dummy source and samplepos to instrument
    //  Geometry::ObjComponent *samplepos = new Geometry::ObjComponent("Sample",instrument.get());
    //  instrument->add(samplepos);
    //  instrument->markAsSamplePos(samplepos);
    //  samplepos->setPos(0.0,0.0,0.0);

    //  Geometry::ObjComponent *source = new Geometry::ObjComponent("Source",instrument.get());
    //  instrument->add(source);
    //  instrument->markAsSource(source);

    //  double l1 = primaryflightpath;
    //  source->setPos(0.0,0.0,-1.0*l1);

    //  // 4. add detectors
    //  // The L2 and 2-theta values from Raw file assumed to be relative to sample position
    //  const int numDetector = (int)detectorids.size();    // number of detectors
    //  std::vector<int> detID = detectorids;    // detector IDs
    //  std::vector<double> angle = twothetas;  // angle between indicent beam and direction from sample to detector (two-theta)

    //  for (int i = 0; i < numDetector; ++i)
    //  {
    //    // Create a new detector. Instrument will take ownership of pointer so no need to delete.
    //    Geometry::Detector *detector = new Geometry::Detector("det",detID[i],samplepos);
    //    Kernel::V3D pos;

    //    // FIXME : need to confirm the definition
    //    double r = totalflightpaths[i] - l1;
    //    pos.spherical(r, angle[i], 0.0 );

    //    detector->setPos(pos);

    //    // add copy to instrument and mark it
    //    instrument->add(detector);
    //    instrument->markAsDetector(detector);
    //  }

    //}

  }//namespace
}//namespace
