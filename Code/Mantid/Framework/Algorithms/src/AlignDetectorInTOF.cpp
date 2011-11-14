/*WIKI*


The offsets are a correction to the TOF values and are applied to each TOF event as follows:

:<math> d = \frac{h}{2m_N} \frac{t.o.f.}{L_{tot} sin \theta} (1+ \rm{offset})</math>

The detector offsets can be obtained from either: an [[OffsetsWorkspace]] where each pixel has one value, the offset; or a .cal file (in the form created by the ARIEL software).

'''Note:''' the workspace that this algorithms outputs is a [[Ragged Workspace]].

==== Restrictions on the input workspace ====
The input workspace must contain histogram or event data where the X unit is time-of-flight and the Y data is raw counts. The [[instrument]] associated with the workspace must be fully defined because detector, source & sample position are needed.


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/AlignDetectorInTOF.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"
#include <fstream>
#include <sstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using Mantid::DataObjects::OffsetsWorkspace;

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace Algorithms
{

  // Register the algorithm into the algorithm factory
  DECLARE_ALGORITHM(AlignDetectorInTOF)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  AlignDetectorInTOF::AlignDetectorInTOF()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  AlignDetectorInTOF::~AlignDetectorInTOF()
  {
  }
  

  /// Sets documentation strings for this algorithm
  void AlignDetectorInTOF::initDocs()
  {
    this->setWikiSummary("Performs correction on TOF value on each TOF. ");
    this->setOptionalMessage("Performs correction on TOF value on each TOF.");
  }

  void AlignDetectorInTOF::init()
  {
    /*
    CompositeWorkspaceValidator<> *wsValidator = new CompositeWorkspaceValidator<>;
    //Workspace unit must be TOF.
    wsValidator->add(new WorkspaceUnitValidator<>("TOF"));
    wsValidator->add(new RawCountValidator<>);
    wsValidator->add(new InstrumentValidator<>);

    declareProperty( new WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace","",Direction::Input,wsValidator),
      "A workspace with units of TOF" );
    */

    //Input workspace must be in dSpacing and be an inputWorkspace
    API::CompositeWorkspaceValidator<EventWorkspace> *wsValidator2 = new API::CompositeWorkspaceValidator<EventWorkspace>;
    wsValidator2->add(new API::WorkspaceUnitValidator<EventWorkspace>("TOF"));

    declareProperty( new WorkspaceProperty<DataObjects::EventWorkspace>("InputWorkspace","",Direction::Input,wsValidator2),
         "An EventWorkspace with units of TOF" );


    declareProperty( new WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace","",Direction::Output),
      "The name to use for the output workspace" );

    std::vector<std::string> exts;
    exts.push_back(".cal");
    exts.push_back(".dat");
    declareProperty(new FileProperty("CalibrationFile", "", FileProperty::OptionalLoad, exts),
       "Optional: The .cal file containing the position correction factors. Either this or OffsetsWorkspace needs to be specified.");

  }

  //-----------------------------------------------------------------------
  /** Executes the algorithm
   *  @throw Exception::FileError If the calibration file cannot be opened and read successfully
   *  @throw Exception::InstrumentDefinitionError If unable to obtain the source-sample distance
   */
  void AlignDetectorInTOF::exec()
  {
    // Get the input workspace
    MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");

    // Read in the calibration data
    const std::string calFileName = getProperty("CalibrationFile");
    progress(0.0,"Reading calibration file");
    if (calFileName.empty())
        throw std::invalid_argument("You must specify CalibrationFile.");

    if (calFileName.empty()){
      throw std::invalid_argument("Must use Ke's calibration file in TOF");
    }

    this->execTOFEvent(calFileName, inputWS);

    return;
  }

  /*
   * Compute TOF with Offset
   * @params
   *
   */
  void AlignDetectorInTOF::execTOFEvent(std::string calfilename, Mantid::API::MatrixWorkspace_const_sptr inputWS){

    g_log.debug() << "Processing in TOF only!" << std::endl;

    // 1. Read spectral - offset file
    std::map<detid_t, double> specmap;

    std::ifstream calfile(calfilename.c_str());
    if (!calfile){
      g_log.error() << "File " << calfilename << " is not readable" << std::endl;
    }
    std::string line;
    detid_t specid;
    double offset;
    while(getline(calfile, line)){
      std::istringstream ss(line);
      ss >> specid >> offset;
      specmap.insert(std::make_pair(specid, offset));
    }

    // 2. Convert to Eventworkspace and generate a new workspace for output
    EventWorkspace_const_sptr eventWS = boost::dynamic_pointer_cast<const EventWorkspace>(inputWS);
    if (!eventWS){
      g_log.error() << "Input Worskspace is not EventWorkspace!" << std::endl;
      throw std::invalid_argument("Input Workspace is not EventWorkspace");
    }

    API::MatrixWorkspace_sptr outputMatrixWS = this->getProperty("OutputWorkspace");
    EventWorkspace_sptr outputWS;
    if (outputMatrixWS == inputWS){
      outputWS = boost::dynamic_pointer_cast<EventWorkspace>(outputMatrixWS);
    } else {
      outputWS = boost::dynamic_pointer_cast<EventWorkspace>(
          API::WorkspaceFactory::Instance().create("EventWorkspace", eventWS->getNumberHistograms(), 2, 1));
      API::WorkspaceFactory::Instance().initializeFromParent(eventWS, outputWS, false);
      outputWS->copyDataFrom((*eventWS));

      outputMatrixWS = boost::dynamic_pointer_cast<MatrixWorkspace>(outputWS);
      this->setProperty("OutputWorkspace", outputMatrixWS);
    }

    // 3. Convert!
    for (int ispec = 0; ispec < static_cast<int>(outputWS->getNumberHistograms()); ispec ++){
      // For each spectrum

      EventList events = outputWS->getEventList(ispec);
      std::set<detid_t> detectorids = eventWS->getSpectrum(size_t(ispec))->getDetectorIDs();

      // a) Check! There is only one possible detector ID in the set
      if (detectorids.size() != 1){
        g_log.error() << "Spectrum " << ispec << " Detectors = " << detectorids.size() << std::endl;
      }

      std::set<detid_t>::iterator setiter;
      double shiftfactor = 1.0;
      detid_t detid = 0;
      for (setiter=detectorids.begin(); setiter != detectorids.end(); ++setiter){

        detid = *setiter;
        std::map<detid_t, double>::iterator mapiter = specmap.find(detid);
        if (mapiter == specmap.end()){
          // No match
          g_log.error() << "Detector (ID) = " << detid << "  Has No Entry In Calibration File" << std::endl;
        } else {
          // Matched
          // i) Inner-module offset
          double offset1 = mapiter->second;

          // ii) Inter-module offset
          detid_t index2 = detid_t(detid/1250)*1250+1250-2;
          std::map<detid_t, double>::iterator itermodule = specmap.find(index2);
          if (itermodule == specmap.end()){
            throw std::invalid_argument("Inter-module offset cannot be found");
          }
          double offset2 = itermodule->second;

          // iii) Inter-stack offset
          detid_t index3 = index2 + 1;
          std::map<detid_t, double>::iterator iterstack = specmap.find(index3);
          if (iterstack == specmap.end()){
            throw std::invalid_argument("Inter-stack offset cannot be found");
          }
          double offset3 = iterstack->second;

          // iv) overall factor
          shiftfactor = pow(10.0, -(offset1+offset2+offset3));

          /*
          if (ispec < 30){
            g_log.notice() << "Detector " << detid << "  Shift Factor = " << shiftfactor << "  Inner-module = " << offset1 << std::endl;
          }
          */
        }

      } // for one and only one detector
      if (ispec < 30){
        g_log.notice() << "Detector " << detid << "  Shift Factor = " << shiftfactor << "  Number of events = " << events.getNumberEvents() << std::endl;
      }

      /*
      if (ispec == 11){
        std::vector<double> tofs;
        outputWS->getEventList(ispec).getTofs(tofs);
        // events.getTofs(tofs);
        g_log.notice() << "Before: TOF[0] = " << tofs[0] << std::endl;
      }
      */
      outputWS->getEventList(ispec).convertTof(shiftfactor, 0.0);
      /*
      if (ispec == 11){
        std::vector<double> tofs;
        events.getTofs(tofs);
        g_log.notice() << "After: TOF[0] = " << tofs[0] << std::endl;
      }
      */

    } // for spec

    // Another check
    /*
    EventList checklist = outputWS->getEventList(11);
    std::vector<double> tofs;
    checklist.getTofs(tofs);
    g_log.notice() << "Final Check: TOF[11][0]" << tofs[0] << std::endl;
    */

    return;
  }

} // namespace Mantid
} // namespace Algorithms
