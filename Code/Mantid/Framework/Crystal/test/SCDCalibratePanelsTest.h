/*
 * SCDCalibratePanelsTest.h
 *
 *  Created on: Mar 12, 2012
 *      Author: ruth
 */

#ifndef SCDCALIBRATEPANELSTEST_H_
#define SCDCALIBRATEPANELSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "../../Geometry/inc/MantidGeometry/Instrument/RectangularDetector.h"
#include "../../Geometry/inc/MantidGeometry/IComponent.h"
#include "../../Kernel/inc/MantidKernel/Quat.h"
#include "../../Geometry/inc/MantidGeometry/Instrument.h"
#include "../../Kernel/inc/MantidKernel/V3D.h"
#include "../../Geometry/inc/MantidGeometry/Instrument/ParameterMap.h"
#include "MantidCrystal/SCDCalibratePanels.h"
#include "../../DataObjects/inc/MantidDataObjects/Workspace2D.h"
//----------------------------------- For subtesting part of Calibrate Saving code------------------------
#include <iostream>
#include <fstream>
#include "../../Kernel/inc/MantidKernel/Property.h"
#include "../../API/inc/MantidAPI/MatrixWorkspace.h"
#include "../../API/inc/MantidAPI/AnalysisDataService.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace std;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using Mantid::Crystal::SCDCalibratePanels;
//------------------------------------------------


class SCDCalibratePanelsTest : public CxxTest::TestSuite
{

public:

  SCDCalibratePanelsTest(){}

  void test_data()
   {
     FrameworkManager::Instance();
     boost::shared_ptr< Algorithm > alg= AlgorithmFactory::Instance().create("LoadIsawPeaks", 1);

     alg->initialize() ;
     alg->setPropertyValue("Filename", "TOPAZ_3007.peaks");
     alg->setPropertyValue("OutputWorkspace", "TOPAZ_3007");

     alg->execute();

     PeaksWorkspace_sptr Peakws =boost::dynamic_pointer_cast<PeaksWorkspace>(
         AnalysisDataService::Instance().retrieve("TOPAZ_3007") );
     AnalysisDataService::Instance().remove("TOPAZ_3007");

     alg= AlgorithmFactory::Instance().create("SCDCalibratePanels", 1);

     alg->initialize();

     alg->setProperty("PeakWorkspace", Peakws );
     alg->setProperty("a",14.0);
     alg->setProperty("b",19.3);
     alg->setProperty("c",  8.6);
     alg->setProperty("alpha",90.0);
     alg->setProperty("beta",105.0);
     alg->setProperty("gamma",90.0);
    // alg->setProperty("use_L0", false);
    // alg->setProperty("use_timeOffset", false );
     alg->setProperty("XmlFilename","abc.xml");
     alg->setProperty("DetCalFilename","abc.DetCal");
     alg->setProperty("PanelGroups","SpecifyGroups");
     alg->setProperty("Grouping","26");
     alg->execute();



   }
// ------ Test code for outputs ---------------------------
  void trest_data()
  {
    FrameworkManager::Instance();
    boost::shared_ptr< Algorithm > alg= AlgorithmFactory::Instance().create("LoadIsawPeaks", 1);

    alg->initialize() ;
    alg->setPropertyValue("Filename", "TOPAZ_3007.peaks");
    alg->setPropertyValue("OutputWorkspace", "TOPAZ_3007");

    alg->execute();
    PeaksWorkspace_sptr Peakws =boost::dynamic_pointer_cast<PeaksWorkspace>(
              AnalysisDataService::Instance().retrieve("TOPAZ_3007") );
         AnalysisDataService::Instance().remove("TOPAZ_3007");
    set<string> SetOfBankNames;
    for( int i=0; i< Peakws->getNumberPeaks();i++)
       SetOfBankNames.insert( Peakws->getPeak(i).getBankName());

    set<string>::iterator BankSetIterator;
    vector<string> VectorofBankNames;
    for( BankSetIterator = SetOfBankNames.begin(); BankSetIterator != SetOfBankNames.end(); BankSetIterator++)
       VectorofBankNames.push_back(*BankSetIterator);

    boost::shared_ptr<const Instrument> NewInstrument =Peakws->getPeak(0).getInstrument();

    boost::shared_ptr<ParameterMap> pmap =NewInstrument->getParameterMap();
    map<string,double> result;
    for( int i=0; i< (int)VectorofBankNames.size();i++)
     { char ixchar[3] ;
       sprintf(ixchar,"%d",i);
       string ichar(ixchar);
       result["f"+ichar+".l0"]=17.5;
       result["f"+ichar+".t0"]=.2;
       result["f"+ichar+".detWidthScale"]=1;
       result["f"+ichar+".detHeightScale"]=1;
       result["f"+ichar+".Xoffset"]=0.000;
       result["f"+ichar+".Yoffset"]=0.000;
       result["f"+ichar+".Zoffset"]=0.000;
       result["f"+ichar+".Xrot"]=0;
       result["f"+ichar+".Yrot"]=0;
       result["f"+ichar+".Zrot"]=0;
     }
    ostringstream oss3 (ostringstream::out);
    oss3<<"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"<<std::endl;
     oss3<<" <parameter-file instrument=\""<<"TOPAZ"<<"\" date=\"2011-01-21T00:00:00\">"<<std::endl;
        boost::shared_ptr<const RectangularDetector> bank_rect;
       for( int i=0; i< (int)VectorofBankNames.size(); i++)

      {

        string bankName = VectorofBankNames[i];;


         oss3<<"<component-link name=\""<<bankName<<"\">"<<std::endl;

          boost::shared_ptr<const IComponent> bank =NewInstrument->getComponentByName( bankName);
        // boost::shared_ptr<const Geometry::IComponent> bank = NewInstrument->getComponentByName( bankName);
         Quat RelRot = bank->getRelativeRot();

         double rotx,roty,rotz;
         char ichar[3] ;
         sprintf(ichar,"%d",i);

         string istring(ichar);
          rotx=result["f"+istring+".Xrot"];

          roty= result["f"+istring+".Yrot"];
          rotz= result["f"+istring+".Zrot"];

         Quat newRelRot = Quat(rotx,V3D(1,0,0))*Quat(roty,V3D(0,1,0))*Quat(rotz,V3D(0,0,1))*RelRot;
         SCDCalibratePanels pp;
         pp.Quat2RotxRotyRotz( newRelRot, rotx,roty,rotz);

         pmap->addRotationParam( bank.get(),"string(rotx)", rotx);
         pmap->addRotationParam( bank.get(),"string(roty)", roty);
         pmap->addRotationParam( bank.get(),"string(rotz)", rotz);
         pmap->addQuat( bank.get(),"rot",newRelRot);//Should not have had to do this???

         oss3<<"  <parameter name =\"rotx\"><value val=\""<<rotx<<"\" /> </parameter>"<<std::endl;
         oss3<<"  <parameter name =\"roty\"><value val=\""<<roty<<"\" /> </parameter>"<<std::endl;

         oss3<<"  <parameter name =\"rotz\"><value val=\""<<rotz<<"\" /> </parameter>"<<std::endl;


         V3D pos1 = bank->getRelativePos();

         pmap->addPositionCoordinate(bank.get(), string("x"), result["f"+istring+".Xoffset"] + pos1.X());
         pmap->addPositionCoordinate(bank.get(), string("y"),result["f"+istring+".Yoffset"] + pos1.Y());
         pmap->addPositionCoordinate(bank.get(), string("z"),result["f"+istring+".Zoffset"]+ pos1.Z());

         oss3<<"  <parameter name =\"x\"><value val=\""<<result["f"+istring+".Xoffset"] + pos1.X()<<"\" /> </parameter>"<<std::endl;

         oss3<<"  <parameter name =\"y\"><value val=\""<<result["f"+istring+".Yoffset"] + pos1.Y()<<"\" /> </parameter>"<<std::endl;

         oss3<<"  <parameter name =\"z\"><value val=\""<<result["f"+istring+".Zoffset"]+ pos1.Z()<<"\" /> </parameter>"<<std::endl;


         vector<double>oldScalex = pmap->getDouble( bank->getName(),string("scalex"));
         vector<double>oldScaley = pmap->getDouble( bank->getName(),string("scaley"));

         double scalex,scaley;
         if( oldScalex.size()>0)
           scalex= oldScalex[0]+result["f"+istring+".detWidthScale"];
         else
           scalex=result["f"+istring+".detWidthScale"];

         if( oldScaley.size()>0)
           scaley= oldScaley[0]+result["f"+istring+".detHeightScale"];
         else
           scaley= result["f"+istring+".detHeightScale"];

         pmap->addDouble( bank.get(),string("scalex"), scalex);
         pmap->addDouble( bank.get(),string("scaley"), scaley);
         oss3<<"  <parameter name =\"scalex\"><value val=\""<<scalex<<"\" /> </parameter>"<<std::endl;
         oss3<<"  <parameter name =\"scaley\"><value val=\""<<scaley<<"\" /> </parameter>"<<std::endl;
         oss3<<"</component-link>"<<std::endl;


      }//For @ bank

      //-------------------Set moderator position for L0---------------------------------
      IObjComponent_const_sptr source = NewInstrument->getSource();
      oss3<<"<component-link name=\""<<source->getName()<<"\">"<<std::endl;
      IObjComponent_const_sptr sample = NewInstrument->getSample();
      V3D sourceRelPos = source->getRelativePos();
      V3D sourcePos = source->getPos();
      V3D parentSourcePos = sourcePos-sourceRelPos;
      V3D source2sampleDir = sample->getPos()-source->getPos();
      double scalee = result["f0.l0"]/source2sampleDir.norm();
      V3D newsourcePos = sample->getPos()- source2sampleDir*scalee;
      V3D newsourceRelPos = newsourcePos-parentSourcePos;

      pmap->addPositionCoordinate(source.get(), string("x"), newsourceRelPos.X());
      pmap->addPositionCoordinate(source.get(), string("y"), newsourceRelPos.Y());
      pmap->addPositionCoordinate(source.get(), string("z"),newsourceRelPos.Z());
      oss3<<"  <parameter name =\"x\"><value val=\""<< newsourceRelPos.X()<<"\" /> </parameter>"<<std::endl;

      oss3<<"  <parameter name =\"y\"><value val=\""<< newsourceRelPos.Y()<<"\" /> </parameter>"<<std::endl;

      oss3<<"  <parameter name =\"z\"><value val=\""<<newsourceRelPos.Z()<<"\" /> </parameter>"<<std::endl;


      oss3<<"</component-link>"<<std::endl;
      std::cout<<"after setting instrument"<<std::endl;
      oss3<<"</parameter-file>"<< std::endl;
      //peaksWs->setInstrument( NewInstrument);
      string DetCalFileName ="abc1.DetCal";
      string XmlFileName ="abc1.xml";

      if( DetCalFileName.size()>0)
      {

       boost::shared_ptr<Algorithm>fit_alg = AlgorithmFactory::Instance().create("SaveIsawDetCal",1);
       fit_alg->initialize();


       Peakws->setInstrument( NewInstrument);

       fit_alg->setProperty("Filename", DetCalFileName);


       fit_alg->setProperty("InputWorkspace", Peakws);
       fit_alg->setProperty("TimeOffset", 1.2);

       fit_alg->execute();

      }
      std::cout<<"At the End"<<std::endl;
      std::cout<< oss3.str();
      if( XmlFileName.size() > 0)
      {
        cout<<"XmlFileName="<<XmlFileName<<std::endl;
        filebuf fb;
        fb.open (XmlFileName,ios::out);
        ostream os(&fb);
        os << oss3.str();
        fb.close();

      }

  }

};

#endif /* SCDCALIBRATEPANELSTEST_H_ */
