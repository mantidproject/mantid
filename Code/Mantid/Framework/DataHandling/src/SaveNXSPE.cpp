#include "MantidDataHandling/SaveNXSPE.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/ObjComponent.h"

#include <Poco/File.h>
#include <Poco/Path.h>

namespace Mantid
{
  namespace DataHandling
  {

    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM( SaveNXSPE)

    using namespace Kernel;
    using namespace API;

    const double SaveNXSPE::MASK_FLAG = -1e30;
    const double SaveNXSPE::MASK_ERROR = 0.0;

    SaveNXSPE::SaveNXSPE() :
      API::Algorithm()
    {
    }

    /**
     * Initialise the algorithm
     */
    void
    SaveNXSPE::init()
    {

      std::vector < std::string > exts;
      exts.push_back(".nxspe");

      declareProperty(new API::FileProperty("Filename", "", FileProperty::Save,
          exts),
          "The name of the NXSPE file to write, as a full or relative path");

      CompositeValidator<> * wsValidator = new CompositeValidator<> ;
      wsValidator->add(new API::WorkspaceUnitValidator<>("DeltaE"));
      wsValidator->add(new API::CommonBinsValidator<>);
      wsValidator->add(new API::HistogramValidator<>);

      declareProperty(new WorkspaceProperty<MatrixWorkspace> ("InputWorkspace",
          "", Direction::Input, wsValidator),
          "Name of the workspace to be saved.");

      BoundedValidator<double> *mustBePositive =
          new BoundedValidator<double> ();
      mustBePositive->setLower(0.0);
      declareProperty("Efixed", EMPTY_DBL(),
          "Value of the fixed energy to write into NXSPE file.");

      NullValidator<double> *numberValidator = new NullValidator<double> ();
      declareProperty("psi", EMPTY_DBL(), numberValidator,
          "Value of PSI to write into NXSPE file.");

      declareProperty("ki_over_kf_scaling", true,
          "Flags in the file whether Ki/Kf scaling has been done or not.");
    }

    /**
     * Execute the algorithm
     */
    void
    SaveNXSPE::exec()
    {
      using namespace Mantid::API;

      // Constant for converting Radians to Degrees
      const double rad2deg = 180.0 / M_PI;
      double efixed = 0.0;

      // Retrieve the input workspace
      const MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

      // Do the full check for common binning
      if (!WorkspaceHelpers::commonBoundaries(inputWS))
        {
          g_log.error("The input workspace must have common bins");
          throw std::invalid_argument(
              "The input workspace must have common bins");
        }

      // Number of spectra
      const int nHist = inputWS->getNumberHistograms();
      // Number of energy bins
      this->nBins = inputWS->blocksize();

      // Get a pointer to the sample
      Geometry::IObjComponent_const_sptr sample =
          inputWS->getInstrument()->getSample();

      // Retrieve the filename from the properties
      this->filename = getPropertyValue("Filename");

      // Create the file.
      ::NeXus::File nxFile(this->filename, NXACC_CREATE5);

      // Make the top level entry (and open it)
      nxFile.makeGroup(inputWS->getName(), "NXentry", true);

      // Definition name and version
      nxFile.writeData("definition", "NXSPE");
      nxFile.openData("definition");
      // Make Version 1.0 as we don't have all the metadata yet!
      nxFile.putAttr("version", "1.0");
      nxFile.closeData();

      // Program name and version
      nxFile.writeData("program_name", "mantid");
      nxFile.openData("program_name");
      // TODO: Get the correct version number
      nxFile.putAttr("version", "1.1");
      nxFile.closeData();

      // Create NXSPE_info
      nxFile.makeGroup("NXSPE_info", "NXcollection", true);

      // Get the value out of the property first
      efixed = getProperty("Efixed");
      // Now lets check to see if the workspace nows better.
      // TODO: Check that this is the way round we want to do it.
      const API::Run & run = inputWS->run();
      if (run.hasProperty("Ei"))
        {
          Kernel::Property* propEi = run.getProperty("Ei");
          efixed = boost::lexical_cast<double, std::string>(propEi->value());
        }
      nxFile.writeData("fixed_energy", efixed);
      nxFile.openData("fixed_energy");
      nxFile.putAttr("units", "meV");
      nxFile.closeData();

      double psi = getProperty("psi");
      if (psi != EMPTY_DBL())
        {
          nxFile.writeData("psi", psi);
          nxFile.openData("psi");
          nxFile.putAttr("units", "degrees");
          nxFile.closeData();
        }

      bool kikfScaling = getProperty("ki_over_kf_scaling");
      if (kikfScaling)
        {
      nxFile.writeData("ki_over_kf_scaling", 1);
        }
      else
        {
          nxFile.writeData("ki_over_kf_scaling", 0);
        }

      nxFile.closeGroup(); // NXSPE_info

      // NXinstrument
      nxFile.makeGroup("instrument", "NXinstrument", true);
      // Write the instrument name
      nxFile.writeData("name", inputWS->getInstrument()->getName());
      // and the short name
      nxFile.openData("name");
      // TODO: Get the instrument short name
      nxFile.putAttr("short_name", inputWS->getInstrument()->getName());
      nxFile.closeData();

      // NXfermi_chopper
      nxFile.makeGroup("fermi", "NXfermi_chopper", true);

      nxFile.writeData("energy", efixed);
      nxFile.closeGroup(); // NXfermi_chopper

      nxFile.closeGroup(); // NXinstrument

      // NXsample
      nxFile.makeGroup("sample", "NXsample", true);
      // TODO: Write sample info
//      nxFile.writeData("rotation_angle", 0.0);
//      nxFile.writeData("seblock", "NONE");
//      nxFile.writeData("temperature", 300.0);

      nxFile.closeGroup(); // NXsample

      // Make the NXdata group
      nxFile.makeGroup("data", "NXdata", true);

      // Energy bins
      // Get the Energy Axis (X) of the first spectra (they are all the same - checked above)
      const MantidVec& X = inputWS->readX(0);
      nxFile.writeData("energy", X);
      nxFile.openData("energy");
      nxFile.putAttr("units", "meV");
      nxFile.closeData();

      // let's create some blank arrays in the nexus file

      std::vector<double> azimuthal;
      std::vector<double> polar;
      std::vector<double> azimuthal_width;
      std::vector<double> polar_width;
      std::vector<double> secondary_flightpath;

      double delta_polar = 0.0;
      double delta_azimuthal = 0.0;
      double distance = 0.0;

      std::vector<int> array_dims;
      array_dims.push_back(nHist);
      array_dims.push_back(nBins);

      nxFile.makeData("data", ::NeXus::FLOAT64, array_dims, false);
      nxFile.makeData("error", ::NeXus::FLOAT64, array_dims, false);

      std::vector<int> slab_start;
      std::vector<int> slab_size;

      // What size slabs are we going to write...
      slab_size.push_back(1);
      slab_size.push_back(nBins);

      // And let's start at the beginning
      slab_start.push_back(0);
      slab_start.push_back(0);

      // define the data and error vectors for masked detectors
      std::vector<double> masked_data (nBins, MASK_FLAG);
      std::vector<double> masked_error (nBins, MASK_ERROR);
       
      // Create a progress reporting object
      Progress progress(this,0,1,100);
      const int progStep = static_cast<int>(ceil(nHist/100.0));
      
      // Loop over spectra
      for (size_t i = 0; i < static_cast<size_t> (nHist); i++)
        {
          // Check that we aren't writing a monitor...
          if (!inputWS->getDetector(i)->isMonitor())
            {
              Geometry::IDetector_sptr det = inputWS->getDetector(i);
              polar.push_back(inputWS->detectorTwoTheta(det) * rad2deg);
              azimuthal.push_back(det->getPhi() * rad2deg);

              // Get Sample->Detector distance
              distance = det->getDistance(*sample);
              // ... and store it
              secondary_flightpath.push_back(distance);

              // Now let's work out the detector widths
              // TODO: This is the historically wrong method...update it!

              // Initialise to large values
              double xmin = -1000.0;
              double xmax = 1000.0;
              double ymin = -1000.0;
              double ymax = 1000.0;
              double zmin = -1000.0;
              double zmax = 1000.0;
              // Get the bounding box
              det->getBoundingBox(xmax, ymax, zmax, xmin, ymin, zmin);
              double xsize = xmax - xmin;
              double ysize = ymax - ymin;
              delta_polar = atan2((ysize / 2.0), distance) * rad2deg;
              delta_azimuthal = atan2((xsize / 2.0), distance) * rad2deg;

              // Now store the widths...
              polar_width.push_back(delta_polar);
              azimuthal_width.push_back(delta_azimuthal);

              if (!inputWS->getDetector(i)->isMasked())
                {
                  // no masking...
                  // Open the data
                  nxFile.openData("data");
                  slab_start[0] = i;
                  nxFile.putSlab(const_cast<MantidVec&> (inputWS->readY(i)),
                      slab_start, slab_size);
                  // Close the data
                  nxFile.closeData();

                  // Open the error
                  nxFile.openData("error");
                  //MantidVec& tmparr = const_cast<MantidVec&>(inputWS->dataE(i));
                  //nxFile.putSlab((void*)(&(tmparr[0])), slab_start, slab_size);
                  nxFile.putSlab(const_cast<MantidVec&> (inputWS->readE(i)),
                      slab_start, slab_size);
                  // Close the error
                  nxFile.closeData();
                }
              else
                {
                  // Write a masked value...
                  // Open the data
                  nxFile.openData("data");
                  slab_start[0] = i;
                  nxFile.putSlab(masked_data, slab_start, slab_size);
                  // Close the data
                  nxFile.closeData();

                  // Open the error
                  nxFile.openData("error");
                  nxFile.putSlab(masked_error, slab_start, slab_size);
                  // Close the error
                  nxFile.closeData();
                }
            }
          // make regular progress reports and check for canceling the algorithm
          if ( i % progStep == 0 )
          {
            progress.report();
          }
        }

      // Write the Polar (2Theta) angles
      nxFile.writeData("polar", polar);

      // Write the Azimuthal (phi) angles
      nxFile.writeData("azimuthal", azimuthal);

      // Now the widths...
      nxFile.writeData("polar_width", polar_width);
      nxFile.writeData("azimuthal_width", azimuthal_width);

      // Secondary flight path
      nxFile.writeData("distance", secondary_flightpath);

      nxFile.closeGroup(); // NXdata

      nxFile.closeGroup(); // Top level NXentry
    }

  } // namespace DataHandling
} // namespace Mantid
