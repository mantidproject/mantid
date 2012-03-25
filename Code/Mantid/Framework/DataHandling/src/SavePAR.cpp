/*WIKI* 


Saves the geometry information of the detectors in a workspace into a PAR format ASCII file. The angular positions and linear sizes of the detectors are calculated using [[FindDetectorsPar]] algorithm. 

Tobyfit PAR file is an ASCII file consisting of the header and 5 or 6 text columns. Mantid generates 6-column files. Header contains the number of the rows in the phx file excluding the header. (number of detectors). The column has the following information about a detector:

  *
  *         1st column      sample-detector distance (secondary flight path)
  *         2nd  "          scattering angle (deg)
  *         3rd  "          azimuthal angle (deg)
  *                         (west bank = 0 deg, north bank = -90 deg etc.)
  *                         (Note the reversed sign convention cf [[SavePHX|.phx]] files)
  *         4th  "          width (m)
  *         5th  "          height (m)
  *         6th  "          detector ID    -- Mantid specific. 
  *---


You should expect to find column 6 to be the detector ID in Mantid-generated par files only. 




*WIKI*/
#include "MantidDataHandling/SavePAR.h"
#include "MantidDataHandling/FindDetectorsPar.h"
#include "MantidAPI/FileProperty.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidAPI/WorkspaceValidators.h"

#include <cstdio>
#include <fstream>
#include <iomanip>

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SavePAR);

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

// A reference to the logger is provided by the base class, it is called g_log.
// It is used to print out information, warning and error messages

void SavePAR::init() {
  declareProperty(new WorkspaceProperty<> ("InputWorkspace", "",
                                           Direction::Input, boost::make_shared<InstrumentValidator>()), "The input workspace");
  declareProperty(new FileProperty("Filename", "", FileProperty::Save),
      "The filename to use for the saved data");

}

void SavePAR::exec() {
 
  // Get the input workspace
  MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");

  // Get the sample position
  const Kernel::V3D samplePos =
      inputWorkspace->getInstrument()->getSample()->getPos();

  // Retrieve the filename from the properties
  const std::string filename = getProperty("Filename");

  // Get a pointer to the sample
  IObjComponent_const_sptr sample =
      inputWorkspace->getInstrument()->getSample();

  std::ofstream outPAR_file(filename.c_str());

  if (!outPAR_file) {
    g_log.error("Failed to open (PAR) file:" + filename);
    throw Kernel::Exception::FileError("Failed to open (PAR) file:",
        filename);
  }

   // execute the subalgorithm to calculate the detector's parameters;
       IAlgorithm_sptr   spCalcDetPar = this->createSubAlgorithm("FindDetectorsPar", 0, 1, true, 1);
       spCalcDetPar->initialize();
       spCalcDetPar->setPropertyValue("InputWorkspace", inputWorkspace->getName());
       // calculate linear rather then angular detector's sizes;
       spCalcDetPar->setPropertyValue("ReturnLinearRanges", "1");
       // in test mode, request the subalgortithm to create output workspace and add it to dataservice
       if(!det_par_ws_name.empty()){
           spCalcDetPar->setPropertyValue("OutputParTable",det_par_ws_name);
       }
  
        // let's not do this for the time being
 /* std::string parFileName = this->getPropertyValue("ParFile");
     if(!(parFileName.empty()||parFileName=="not_used.par")){
               spCalcDetPar->setPropertyValue("ParFile",parFileName);
                   }*/
      spCalcDetPar->execute();
      //
     FindDetectorsPar * pCalcDetPar = dynamic_cast<FindDetectorsPar *>(spCalcDetPar.get());
     if(!pCalcDetPar){	  // "can not get pointer to FindDetectorsPar algorithm"
          throw(std::bad_cast());
      }
      const std::vector<double> & azimuthal           = pCalcDetPar->getAzimuthal();
      const std::vector<double> & polar               = pCalcDetPar->getPolar();
      const std::vector<double> & azimuthal_width     = pCalcDetPar->getAzimWidth();
      const std::vector<double> & polar_width         = pCalcDetPar->getPolarWidth();
      const std::vector<double> & secondary_flightpath= pCalcDetPar->getFlightPath();
      const std::vector<size_t> & det_ID              = pCalcDetPar->getDetID();


      size_t nDetectors = pCalcDetPar->getNDetectors();

  
   // Write the number of detectors to the file.
     outPAR_file <<" "<< nDetectors << std::endl;

   for (size_t i = 0; i < nDetectors; ++i) {
    // verify if no detector defined;
    volatile double NanID = azimuthal[i];
    if(NanID !=azimuthal[i] )continue; // skip NaN -s

      // Now write all the detector info.
      outPAR_file << std::fixed << std::setprecision(3);
      outPAR_file.width(10);
      outPAR_file <<secondary_flightpath[i];
      outPAR_file.width(10);
      outPAR_file<< polar[i];
      outPAR_file.width(10);
      outPAR_file << (-azimuthal[i]);
      outPAR_file.width(10);
      outPAR_file << polar_width[i];
      outPAR_file.width(10);
      outPAR_file << azimuthal_width[i];
      outPAR_file.width(10);
      outPAR_file << det_ID[i] << std::endl;

  }

  // Close the file
  outPAR_file.close();
}

} // namespace DataHandling
} // namespace Mantid
