#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidCrystal/LoadHKL.h"
#include "MantidCrystal/AnvredCorrection.h"
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
using namespace Mantid::PhysicalConstants;

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
    bool first = true;
    double mu1 = 0.0, mu2 = 0.0, wl1 = 0.0, wl2 = 0.0, sc1 = 0.0, astar1 = 0.0;
    do {   
      getline (in, line);
      double h = atof(line.substr(0,4).c_str());
      double k = atof(line.substr(4,4).c_str());
      double l = atof(line.substr(8,4).c_str());
      if (h == 0.0 && k == 0 && l == 0) break;
      double Inti = atof(line.substr(12,8).c_str());
      double SigI = atof(line.substr(20,8).c_str());
      double wl = atof(line.substr(32,8).c_str());
      double tbar = atof(line.substr(40,7).c_str()); //tbar
      int run = atoi(line.substr(47,7).c_str());
      atoi(line.substr(54,7).c_str()); //seqNum
      double trans = atof(line.substr(61,7).c_str()); //transmission
      int bank = atoi(line.substr(68,4).c_str());
      double scattering = atof(line.substr(72,9).c_str());
      atof(line.substr(81,9).c_str()); //dspace
      if (first)
      {
        mu1 = -(double)std::log(trans)/tbar;
        wl1 = wl/1.8;
        sc1 = scattering;
        astar1 = 1.0/trans;
        first = false;
      }
      else
      {
        mu2 = -(double)std::log(trans)/tbar;
        wl2 = wl/1.8;
      }
  
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
    // solve 2 linear equations to find amu and smu
    double amu = (mu2 - 1.0 * mu1) / (-1.0 * wl1 + wl2);
    double smu = mu1 - wl1 * amu;
    double theta = sc1*radtodeg_half;
    int i = (int)(theta/5.);
    double x0,x1,x2;
    gsl_poly_solve_cubic(pc[2][i]/pc[3][i], pc[1][i]/pc[3][i],(pc[0][i]-astar1)/pc[3][i],&x0,&x1,&x2);
    double radius = 0.0;
    if (x0 > 0) radius = x0;
    else if (x1 > 0) radius = x1;
    else if (x2 > 0) radius = x2;
    gsl_poly_solve_cubic(pc[2][i+1]/pc[3][i+1], pc[1][i+1]/pc[3][i+1],(pc[0][i+1]-astar1)/pc[3][i+1],&x0,&x1,&x2);
    double radius1 = 0.0;
    if (x0 > 0) radius1 = x0;
    else if (x1 > 0) radius1 = x1;
    else if (x2 > 0) radius1 = x2;
    double frac = theta - static_cast<double>(static_cast<int>( theta / 5. )) * 5.;//theta%5.
    frac = frac/5.;
    radius = radius*(1-frac) + radius1*frac;
    radius /= mu1;
    g_log.notice() << "LinearScatteringCoef = " << smu << " LinearAbsorptionCoef = " << amu << " Radius = " << radius << " calculated from tbar and transmission of 2 peaks\n";
    API::Run & mrun = ws->mutableRun();
    mrun.addProperty<double>("Radius", radius, true);
    NeutronAtom neutron(static_cast<uint16_t>(EMPTY_DBL()), static_cast<uint16_t>(0),
                        0.0, 0.0, smu, 0.0, smu, amu);
    Object shape = ws->sample().getShape(); // copy
    shape.setMaterial(Material("SetInLoadHKL", neutron, 1.0));
    ws->mutableSample().setShape(shape);

    setProperty("OutputWorkspace", boost::dynamic_pointer_cast<PeaksWorkspace>(ws));


  }

} // namespace Mantid
} // namespace Crystal
