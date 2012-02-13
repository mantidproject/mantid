/*WIKI* 


*WIKI*/
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidCrystal/LoadHKL.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Utils.h"
#include "MantidKernel/V3D.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <fstream>

using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace Crystal
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(LoadHKL)


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  LoadHKL::LoadHKL()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  LoadHKL::~LoadHKL()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void LoadHKL::initDocs()
  {
    this->setWikiSummary("Save a PeaksWorkspace to a ASCII .hkl file.");
    this->setOptionalMessage("Save a PeaksWorkspace to a ASCII .hkl file.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void LoadHKL::init()
  {
    std::vector<std::string> exts;
    exts.push_back(".hkl");

    declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
        "Path to an hkl file to save.");

    declareProperty(new WorkspaceProperty<PeaksWorkspace>("OutputWorkspace","",Direction::Output), "Name of the output workspace.");

  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void LoadHKL::exec()
  {

    std::string filename = getPropertyValue("Filename");
    PeaksWorkspace_sptr ws(new PeaksWorkspace());
    ws->setName(getProperty("OutputWorkspace"));

    int run, seqNum;
    int bank;
    double h ; double k ;  double l ;  double tbar;
    double transmission ; double wl ;
    double scattering ; double Inti ;
    double SigI ; double dspace;
    std::fstream in;
    in.open( filename.c_str(), std::ios::in);


            // Anvred write from Art Schultz
            //hklFile.write('%4d%4d%4d%8.2f%8.2f%4d%8.4f%7.4f%7d%7d%7.4f%4d%9.5f%9.4f\n' 
            //    % (H, K, L, FSQ, SIGFSQ, hstnum, WL, TBAR, CURHST, SEQNUM, TRANSMISSION, DN, TWOTH, DSP))
            // HKL is flipped by -1 due to different q convention in ISAW vs mantid.
    Instrument_sptr inst(new Geometry::Instrument);
    Detector *detector = new Detector("det1",-1, 0);
    detector->setPos(0.0,0.0,0.0);
    inst->add(detector);  // This takes care of deletion
    inst->markAsDetector(detector);
    Mantid::Geometry::ObjComponent * sample = new Mantid::Geometry::ObjComponent("Sample");
    inst->add(sample);  // This takes care of deletion
    inst->markAsSamplePos(sample);
    Mantid::Geometry::ObjComponent * source = new Mantid::Geometry::ObjComponent("Source");
    source->setPos(0.0,0.0,-1.0);
    inst->add(source);  // This takes care of deletion
    inst->markAsSource(source);

    std::string line;
    do {   
      getline (in, line);
      h = atof(line.substr(0,3).c_str());
      k = atof(line.substr(4,7).c_str());
      l = atof(line.substr(8,11).c_str());
      if (h == 0.0 && k == 0 && l == 0) break;
      Inti = atof(line.substr(12,19).c_str());
      SigI = atof(line.substr(20,27).c_str());
      run = atoi(line.substr(28,31).c_str());
      wl = atof(line.substr(32,39).c_str());
      tbar = atof(line.substr(40,46).c_str());
      run = atoi(line.substr(47,53).c_str());
      seqNum = atoi(line.substr(54,60).c_str());
      transmission = atof(line.substr(61,67).c_str());
      bank = atoi(line.substr(68,71).c_str());
      scattering = atof(line.substr(72,80).c_str());
      dspace = atof(line.substr(81,89).c_str());
  
      Peak peak(inst, scattering, wl);
      peak.setHKL(-h,-k,-l);
      peak.setIntensity(Inti);
      peak.setSigmaIntensity(SigI);
      peak.setRunNumber(run);
      std::ostringstream oss;
      oss << "bank" << bank;
      std::string bankName = oss.str();

      peak.setBankName(bankName);
      ws->addPeak(peak);

    } while (!in.eof());

    in.close();
    setProperty("OutputWorkspace", boost::dynamic_pointer_cast<PeaksWorkspace>(ws));


  }

} // namespace Mantid
} // namespace Crystal

