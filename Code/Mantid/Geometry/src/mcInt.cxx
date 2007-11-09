#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#include <vector>
#include <complex>
#include <map>
#include <stack>
#include <algorithm>
#include <boost/regex.hpp>


#include "AuxException.h"
#include "FileReport.h"
#include "GTKreport.h"
#include "OutputLog.h"
#include "XMLattribute.h"
#include "XMLobject.h"
#include "XMLgroup.h"
#include "XMLread.h"
#include "XMLcollect.h"
#include "IndexIterator.h"
#include "LinkIterator.h"
#include "mathSupport.h"
#include "regexSupport.h"
#include "support.h"
#include "BaseVisit.h"
#include "Element.h"
#include "MapSupport.h"
#include "Material.h"
#include "Matrix.h"
#include "Vec3D.h"
#include "Triple.h"
#include "Beam.h"
#include "Track.h"
#include "Surface.h"
#include "Plane.h"
#include "Cylinder.h"
#include "Cone.h"
#include "Sphere.h"
#include "General.h"
#include "Rules.h"
#include "Object.h"
#include "SamGeometry.h"
#include "Flux.h"
#include "Detector.h"
#include "surfaceFactory.h"
#include "Simulation.h"

/*!
  \mainpage PING

  \section Introduction
 
  Main program to read all the data in an make the global structure 

  \section Useage

  The program is designed to be used by
*/

using namespace MonteCarlo;

namespace ELog 
{
  ELog::OutputLog<GTKreport> EMessages;
  ELog::OutputLog<FileReport> FMessages("Spectrum.log");
  ELog::OutputLog<StreamReport> CellMessage;
}

int 
main(int argc,char* argv[])
{
  const double DetectorDistance(300);
  const double Wavelength(0.94);
  Beam B;
  B.setWave(Wavelength);

  int Npts(30000);           // Numbero points to simulate
  double W(2.0);             // W beam width
  if (argc>1)
    StrFunc::convert(argv[1],Npts);
  if (argc>2)
    StrFunc::convert(argv[2],W);

  B.setWidth(W);
  Simulation Master;

  Material Vanadium(0.0725,0.02,5.19,5.08);
  Material NaCl(0.0973,6.5950,3.41,17.0150);
  Material TiZr(0.0541,2.9671,1.8794,4.2315);
  Material CuCl(0.0541,5.6886,0.0,2.2904);
  Material Vac(0.0,0.0,0.0,0.0);

  std::cerr<<"Atten Van = "<<Vanadium.getAtten(Wavelength)<<std::endl;;
  std::cerr<<"Atten NACL = "<<NaCl.getAtten(Wavelength)<<std::endl;;
  std::cerr<<"Atten TiZr = "<<TiZr.getAtten(Wavelength)<<std::endl;;

  Master.createSurface(1,"cz 0.3");
  Master.createSurface(2,"cz 0.31");
  Master.createSurface(11,"pz -2.5");
  Master.createSurface(12,"pz 2.5");
  // Objects
  Master.createObject<1>(1,"-1 2 11 12");
  // 
  Master.addMaterial("Vacuum",Vac);
  Master.addMaterial("NaCl",NaCl);
  Master.addMaterial("TiZr",TiZr);
  // Setting Object
  Master.setObjectMaterial<1>(1,"TiZr");
  Master.writeXML("Sim.xml");
  
/*
  VanRod.addLayer(W,Vanadium);
  VanRod.addDetector(DetectorDistance,-160.0,160.0,20.0);

  double theta;
  std::vector<double> Out;
  std::vector<double> Result;

  std::cout<<"Van"<<std::endl;
  for(int i=0;i<8;i++)
    {
      theta=i*20.0;
      Result=VanRod.getGeom().Integrate(B,theta);
      Out.push_back(Result[0]);
      std::cout<<i<<" "<<Result[0]<<std::endl;
//      std::cout<<i<<" "<<Out[i]<<" "<<1-exp(-Out[0])<<std::endl;
    }
  std::cout<<"--------------------"<+<std::endl;

  for(int i=0;i<Npts;i++)
    VanRod.runPts();
  
  std::vector<double> MCout=VanRod.getSingle();
  VanRod.print();
  for(int i=0;i<static_cast<int>(MCout.size());i++)
    std::cout<<"MC == "<<(1.0-MCout[i])/Out[i % 8]<<" "<<MCout[i]*Out[i % 8]<<std::endl;

*/
}
