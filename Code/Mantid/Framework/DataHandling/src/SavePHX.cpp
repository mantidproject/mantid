/*WIKI* 


Saves the geometry information of the detectors in a workspace into a PHX format ASCII file. The angular positions and angular sizes of the detectors are calculated using [[FindDetectorsPar]] algorithm. 

Mantid generated PHX file is an ASCII file consisting of the header and 7 text columns. Header contains the number of the rows in the phx file excluding the header. (number of detectors). The column has the following information about a detector:

  *         1st column      secondary flightpath,e.g. sample to detector distance (m) -- Mantid specific
  *         2nt  "          0
  *         3rd  "          scattering angle (deg)
  *         4th  "          azimuthal angle (deg)
  *                        (west bank = 0 deg, north bank = 90 deg etc.)
  *                        (Note the reversed sign convention wrt [[SavePAR|.par]] files)
  *         5th  "          angular width e.g. delta scattered angle (deg) 
  *         6th  "          angular height e.g. delta azimuthal angle (deg)
  *         7th  "          detector ID    -- Mantid specific. 
  *---


In standard phx file only the columns  3,4,5 and 6 contain useful information. You can expect to find column 1 to be the secondary flightpath and the column 7 -- the detector ID in Mantid-generated phx files only. 




*WIKI*/
#include "MantidDataHandling/SavePHX.h"
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
DECLARE_ALGORITHM( SavePHX);

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

// A reference to the logger is provided by the base class, it is called g_log.
// It is used to print out information, warning and error messages

void SavePHX::init() {
  declareProperty(new WorkspaceProperty<> ("InputWorkspace", "",
      Direction::Input, boost::make_shared<InstrumentValidator>()), "The input workspace");
  declareProperty(new FileProperty("Filename", "", FileProperty::Save),
      "The filename to use for the saved data");

}

void SavePHX::exec() {
 
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

  std::ofstream outPHX_file(filename.c_str());

  if (!outPHX_file) {
    g_log.error("Failed to open (PHX) file:" + filename);
    throw Kernel::Exception::FileError("Failed to open (PHX) file:",
        filename);
  }

   // execute the subalgorithm to calculate the detector's parameters;
       IAlgorithm_sptr   spCalcDetPar = this->createSubAlgorithm("FindDetectorsPar", 0, 1, true, 1);
       spCalcDetPar->initialize();
       spCalcDetPar->setPropertyValue("InputWorkspace", inputWorkspace->getName());
       spCalcDetPar->setPropertyValue("ReturnLinearRanges", "0");
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
     outPHX_file <<" "<< nDetectors << std::endl;

   for (size_t i = 0; i < nDetectors; ++i) {
    // verify if no detector defined;
    volatile double NanID = azimuthal[i];
    if(NanID !=azimuthal[i] )continue; // skip NaN -s

      // Now write all the detector info.
      outPHX_file << std::fixed << std::setprecision(3);
      outPHX_file <<" "<<secondary_flightpath[i]<<"\t 0 \t\t" << polar[i] << " \t" << azimuthal[i] << " \t"
          << polar_width[i] << " \t" << azimuthal_width[i] << " \t\t"
          << det_ID[i] << std::endl;

  }

  // Close the file
  outPHX_file.close();
}

} // namespace DataHandling
} // namespace Mantid
