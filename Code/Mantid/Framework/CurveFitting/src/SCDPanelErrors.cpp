/*
 * SCDPanelErrors.cpp
 *
 *  Created on: Feb 27, 2012
 *      Author: ruth
 */

#include "MantidCurveFitting/SCDPanelErrors.h"
#include <stdio.h>
#include <math.h>
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/IComponent.h"
#include "MantidKernel/Quat.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidKernel/V3D.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/Unit.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Crystal/UnitCell.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/ObjComponent.h"

using namespace Mantid::API;
using namespace std;
using namespace Mantid::DataObjects;
using namespace  Mantid::Geometry;
using namespace  Mantid::Kernel;
using namespace  Mantid::Kernel::Units;

namespace Mantid
{
namespace CurveFitting
{
// Include when setup for the Fit interface public API::ParamFunction,
  //DECLARE_FUNCTION(SCDPanelErrors)


  Kernel::Logger& SCDPanelErrors::g_log= Kernel::Logger::get("SCDPanelErrors");

  SCDPanelErrors::SCDPanelErrors():IFitFunction(),API::ParamFunction(),CommonDataSetUp(false),pwks(),
                                          tolerance(),B0(),ComponentName()
  {


    g_log.error("Not implemented yet");
    throw invalid_argument("Not implemented yet");
  }

  SCDPanelErrors::~SCDPanelErrors()
  {
    // TODO Auto-generated destructor stub
  }



  SCDPanelErrors::SCDPanelErrors( DataObjects::PeaksWorkspace_sptr &pwk, std::string& Component_name ,
                                        double a, double b, double c, double alpha, double beta,
                                        double gamma, double tolerance1 ):IFitFunction()
                                                                        ,API::ParamFunction()
  {

    CommonDataSetUp = true;
    pwks=pwk;
    ComponentName= Component_name;

    tolerance = tolerance1;
    Geometry::UnitCell lat(a,b,c,alpha,beta,gamma);
    B0= lat.getB();
    init();
    g_log.setLevel(7);//debug mode
  }



  void SCDPanelErrors::init()
  {
      declareParameter("l0",0.0,"Initial Flight Path");
      declareParameter("t0", 0.0, "Time offset");
      declareParameter("detWidth", 0.0,"panel Width");
      declareParameter("detHeight", 0.0,"panel Height");
      declareParameter("Xoffset", 0.0, "Panel Center x offset");
      declareParameter("Yoffset", 0.0, "Panel Center y offset");
      declareParameter("Zoffset", 0.0, "Panel Center z offset");
      declareParameter("Xrot",0.0,"Rotation(degrees) Panel Center in x axis direction");
      declareParameter("Yrot",0.0,"Rotation(degrees) Panel Center in y axis direction");
      declareParameter("Zrot",0.0,"Rotation(degrees) Panel Center in z axis direction");

  }

  void updateParams(  boost::shared_ptr<Geometry::ParameterMap> const &  pmapSv,
                      boost::shared_ptr<Geometry::ParameterMap> &pmap,
                      boost::shared_ptr<const IComponent>  component)
  {
    std::set<std::string> pnamesSv = component->getParameterNames( false );

    set<string>::iterator setIt = pnamesSv.begin();
    int N=0;

    for(  ;setIt !=pnamesSv.end();setIt++)
    {
      string name = (*setIt);
      vector<V3D> posParams = component->getPositionParameter(name , false);

      if( posParams.size() > 0)
      {N++;
        pmap->addV3D( component.get(), name, posParams[0]);
      }

      vector<Quat> rotParams = component->getRotationParameter(name , false);

      if( rotParams.size() > 0)
      {N++;
        pmap->addQuat( component.get(), name, rotParams[0]);
      }

      vector<string> strParams = component->getStringParameter(name,false);

      if( strParams.size() > 0)
      {N++;
        pmap->addString( component.get(), name, strParams[0]);
      }

    }


     boost::shared_ptr<const ICompAssembly>pAssm= boost::dynamic_pointer_cast<const ICompAssembly>(component);

    if( pAssm )
    {

      for( int i=0; i< pAssm->nelements(); i++)
      {
        boost::shared_ptr<IComponent> child = pAssm->getChild(i);
        boost::shared_ptr<const IComponent>childComp = boost::const_pointer_cast<const IComponent>(child);

        updateParams( pmapSv, pmap,childComp);
      }
    }
  }
  void SCDPanelErrors::functionMW  (double *out, const double *xValues, const size_t nData)const
  {
    //----------------- Get Instrument --------------------------
    std::cout<<"Starting functioMW"<<std::endl;
     if( nData <= (size_t)0)
       return;

     if( xValues[0] != floor(xValues[0]))
     {
       g_log.error("Improper workspace set xVals must be integer");
       throw invalid_argument("Improper workspace. xVals must be integer");
     }
     if( xValues[0] <0 || xValues[0] >= pwks->rowCount())
     {

       g_log.error("Improper workspace set");
       throw invalid_argument("Improper workspace. xVals correspond to an index in the PeaksWorkspace");
     }


     Geometry::Instrument_const_sptr instSave = pwks->getPeak( (int)xValues[0]).getInstrument();
     boost::shared_ptr<Geometry::ParameterMap> pmap( new ParameterMap()) ;
     boost::shared_ptr<Geometry::ParameterMap> pmapSv; //original pmap
     if( !instSave)
     {
       g_log.error(" Not all peaks have an instrument");
       throw invalid_argument( " Not all peaks have an instrument");
     }



     boost::shared_ptr<Geometry::Instrument>Pinst( new Geometry::Instrument());
     bool isParameterized = false;

   try
     {
        boost::shared_ptr<Geometry::Instrument> instClone ( instSave->clone());
        boost::shared_ptr<Geometry::Instrument>
                  Pinsta( new Geometry::Instrument(instSave, pmap));
        Pinst=Pinsta;
        isParameterized = true;

     }catch(...)
     {
       isParameterized = true;
       pmapSv= instSave->getParameterMap();

       std::cout<<"updating params"<<std::endl;
       boost::shared_ptr<const IComponent> inst3 = boost::dynamic_pointer_cast<const IComponent>(instSave);

       //set<string> xc=instSave->getParameterNames(false);
       updateParams(   pmapSv,pmap, inst3);

       std::cout<<std::endl;
       boost::shared_ptr<const Geometry::IComponent>baseC ( instSave->base());
       boost::shared_ptr<const Geometry::Instrument> baseI= boost::dynamic_pointer_cast<const Geometry::Instrument>(baseC);

       boost::shared_ptr<Geometry::Instrument> P1(new Geometry::Instrument( baseI , pmap));
       Pinst= P1;
       baseC.reset();
       baseI.reset();

     }


     boost::shared_ptr<Geometry::Instrument> instChange(Pinst);

     if( !instChange)
     {
       g_log.error("Cannot 'clone' instrument");
       throw logic_error("Cannot clone instrument");
     }


    //---------------------Now get Panel-----------------------------

     boost::shared_ptr<const Geometry::IComponent> bank_const =instChange->getComponentByName( ComponentName);
     if( !bank_const)
     {
       g_log.error()<<"No component named "<<ComponentName<<std::endl;
       throw invalid_argument("No component named "+ComponentName);
     }
     boost::shared_ptr< Geometry::IComponent> bankx =boost::const_pointer_cast< Geometry::IComponent>(bank_const);
     boost::shared_ptr< Geometry::Component> bank=boost::dynamic_pointer_cast< Geometry::Component>(bankx);
     if( !bank)
     {
       g_log.error()<<"No non const component named "<<ComponentName<<std::endl;
       throw invalid_argument("No nonconst component named "+ComponentName);
     }

     V3D panelCenter_old = bank->getPos();
     std::cout<<"bank's detector and base's relative position="<<bank->getRelativePos()<<
         bank->base()->getRelativePos()<<std::endl;
     std::cout<<"bank's detector and base's position="<<bank->getPos()<<
         bank->base()->getPos()<<std::endl;

     //------------------------ Rotate Panel------------------------

     Quat Rot0 = bank_const->getRelativeRot();

     Quat Rot =  Quat(getParameter("Xrot"),Kernel::V3D(1.0,0.0,0.0))*Rot0;


     Rot = Quat(getParameter("Yrot"),Kernel::V3D(0.0,1.0,0.0))*Rot;

     Rot = Quat(getParameter("Zrot"),Kernel::V3D(0.0,0.0,1.0))*Rot;

     std::string name = ComponentName;

     try
     {

       bank->setRot( Rot);
       if( isParameterized)
       {
         g_log.error(" instrument parameterization state != bank parmeterization state");
         throw invalid_argument(" instrument parameterization state != bank parmeterization state");
       }
     }catch(...)
     {
       if( !isParameterized)
       {
         g_log.error(" instrument parameterization state != bank parmeterization state");
         throw invalid_argument(" instrument parameterization state != bank parmeterization state");

       }

      pmap->addQuat(bank.get(),"rot", Rot);
     }


     //-------------------------Move Panel ------------------------

     boost::shared_ptr<Geometry::RectangularDetector> Rect= boost::dynamic_pointer_cast<Geometry::RectangularDetector>(bank);
     if( !isParameterized)
     {
       bank->translate( getParameter("Xoffset"), getParameter("Yoffset"), getParameter("Zoffset"));

     }
     else
     {
       V3D pos1 = bank->getRelativePos();
       std::cout<<"Rect detector and base's relative position="<<pos1<<
           bank->base()->getRelativePos()<<std::endl;

       std::cout<<"Rect detector and base's  position="<<bank->getPos()<<
           bank->base()->getPos()<<std::endl;
       pmap->addPositionCoordinate( bank.get(),string("x"),getParameter("Xoffset")+pos1.X());
       pmap->addPositionCoordinate( bank.get(),string("y"),getParameter("Yoffset")+pos1.Y());
       pmap->addPositionCoordinate( bank.get(),string("z"),getParameter("Zoffset")+pos1.Z());


       //--------------------------- Scale Panel------------------------------------

     Kernel::V3D scale = bank->getScaleFactor();
       if( scale != Kernel::V3D(1.0,1.0,1.0))
         pmap->addV3D(bank.get(),"sca", scale);

       //detWidth and detHeight so far only for Rectangular Detectors.
       //TODO::Rotate back to base base and then get width height from bounding box.

       if( Rect)
       {
             const Geometry::IComponent *baseC= Rect->base();
             const Geometry::RectangularDetector *baseR = dynamic_cast< const Geometry::RectangularDetector *>(baseC);
             std::cout<<"rel pos ere"<<Rect->getRelativePosAtXY(3,5)<<baseR->getRelativePosAtXY(3,5)<<std::endl;
             double scalex = getParameter("detWidth")/baseR->xsize();
             double scaley = getParameter("detHeight")/baseR->ysize();
             double scalez = 1;
             if( pmapSv->getDouble( baseR->getName(),string("scalex")).size()>0)
               scalex *=pmapSv->getDouble(  baseR->getName(),string("scalex"))[0];

             if( pmapSv->getDouble(  baseR->getName(),string("scaley")).size()>0)
               scaley *=pmapSv->getDouble(  baseR->getName(),string("scaley"))[0];

             if( pmapSv->getDouble(  baseR->getName(),string("scalez")).size()>0)
               scalez *=pmapSv->getDouble(  baseR->getName(),string("scalez"))[0];

             V3D RectScaleFactor = Rect->getScaleFactor();
             std::cout<<RectScaleFactor;
             if( RectScaleFactor.X()==0 || RectScaleFactor.Y()==0 ||RectScaleFactor.Z()==0)
               RectScaleFactor=V3D(1,1,1);
             V3D scale( scalex,scaley,1.0);
             pmap->addDouble( bank.get(),"scalex",scalex*RectScaleFactor.X());
             pmap->addDouble( bank.get(),"scaley",scaley*RectScaleFactor.Y());
             pmap->addDouble( bank.get(),"scalez",scalez*RectScaleFactor.Z());
             pmap->addV3D( bank.get(),"scale",scale*RectScaleFactor);
             std::cout<<" rel pos aft"<<Rect->getRelativePosAtXY(3,5)
                 <<baseR->getRelativePosAtXY(3,5)<<std::endl;

     }//if Rect
     }//if is parameterized
     //-------------------- Move source(L0)-------------------------------
     boost::shared_ptr<const Geometry::IObjComponent>  source = instChange->getSource();
     boost::shared_ptr<const Geometry::IObjComponent>  sample = instChange->getSample();

      //For L0 change the source position
      Kernel::V3D sourcePos1 = source->getPos();
      Kernel::V3D samplePos = sample->getPos();
      Kernel::V3D direction = (sourcePos1-samplePos);
      sourcePos1 = samplePos +direction*getParameter("l0")/direction.norm();


      pmap->addPositionCoordinate( source.get(),string("x"), sourcePos1.X());
      pmap->addPositionCoordinate( source.get(),string("y"), sourcePos1.Y());
      pmap->addPositionCoordinate( source.get(),string("z"), sourcePos1.Z());

      V3D panelCenter = bank->getPos();//for debug only

      std::cout<<"panel centers="<<panelCenter_old<<panelCenter<<std::endl;
      boost::shared_ptr<const IComponent>RectParent = Rect->getParent();
      std::cout<<"Rect parent pos,name"<<RectParent->getPos()<<RectParent->getName()<<std::endl;
      vector<V3D> m1=pmapSv->getV3D(RectParent->getName(),"pos");

      vector<V3D> m2=pmap->getV3D(RectParent->getName(),"pos");
      std::cout<<"RectParent param info-pos=";
      if( m1.size()>0)
        std::cout<<m1[0];
      else
        std::cout<<"-----";
      if( m2.size()>0)
        std::cout<<m2[0];
      else
        std::cout<<"-----";
      std::cout<<std::endl;

      //---------------------------- Calculate q and hkl vectors-----------------
     Kernel::Matrix<double> UB(3,3,false);
     vector<Kernel::V3D>  hkl_vectors;
     vector<Kernel::V3D>  q_vectors;

     for( size_t i =0; i< nData; i+=3)
     {
       double xIndx = xValues[(int)i];
       if( xIndx != floor(xIndx))
       {
         g_log.error("Improper workspace set xVals must be integer");
         throw invalid_argument("Improper workspace. xVals must be integer");
       }

       int pkIndex = (int) xIndx;
       if( pkIndex < 0 || pkIndex >(int) pwks->rowCount())
       {

         g_log.error("Improper workspace set");
         throw invalid_argument("Improper workspace. xVals correspond to an index in the PeaksWorkspace");
       }

       IPeak & peak_old = pwks->getPeak((int)pkIndex);
       Geometry::Instrument_const_sptr inst = peak_old.getInstrument();
       if( inst->getComponentID() != instSave->getComponentID())
       {
         g_log.error("All peaks must have the same instrument");
         throw invalid_argument("All peaks must have the same instrument");
       }
       V3D detPos_old =peak_old.getDetPos();

       double T0 = peak_old.getTOF()+ getParameter("t0");

       int ID =peak_old.getDetectorID();


       Kernel::V3D hkl = peak_old.getHKL();
       peak_old.setDetectorID(ID); //set det positions
       Peak peak( instChange,ID , peak_old.getWavelength(),
                          hkl,  peak_old.getGoniometerMatrix());

       Wavelength wl;
       MomentumTransfer Q;
       //Calc L2

       V3D detPos =Rect->getRelativePosAtXY( peak_old.getCol(), peak_old.getRow())
                        + panelCenter;

       std::cout<< "pmap and pmapSv stuff"<< std::endl;
       string namee = Rect->getName();
       std::cout<<"Rect rel Pos=";
       if(pmapSv->getV3D( namee,"pos").size()>0)
         std::cout<<pmapSv->getV3D( namee,"pos")[0];
       else
         std::cout<<"-----";
       std::cout<<","<< pmap->getV3D(namee,"pos")[0]
      // <<  Rect->getAtXY(peak_old.getCol(), peak_old.getRow())->base()->getPos()
       <<  peak_old.getDetPos()<<peak.getDetPos()
       <<std::endl;

       std::cout<<"scalex";
       vector<double> scalexVals= pmapSv->getDouble(namee,"scalex");
       if(scalexVals.size()>0)
          std::cout<<pmapSv->getDouble(namee,"scalex")[0];
       else
         std::cout<< -1;

       std::cout<<","<<pmap->getDouble(namee,"scalex")[0]<<std::endl;

       if( pmapSv->getDouble(namee,"scaley").size()>0)
          std::cout<<pmapSv->getDouble(namee,"scaley")[0];
       else
         std::cout<< -1;
       std::cout<<","<<pmap->getDouble(namee,"scaley")[0]<<std::endl;

       std::cout<<"rel det pos-s ere aft="<< detPos_old-panelCenter_old
                 <<(detPos-panelCenter)<<std::endl;
       std::cout<<"pos before and after"<<detPos_old<<","<<detPos<<std::endl;
       std::cout<<"----------------------------------------"<<std::endl;
       V3D detDir = detPos-sample->getPos();
       double L2= detDir.norm();
       detDir.normalize();

       V3D beamDir = sample->getPos() - source->getPos();
       beamDir.normalize();
       double ScatAngle= detDir.angle(beamDir);


       wl.initialize( getParameter("l0"), L2,ScatAngle,
                           0,peak_old.getInitialEnergy(),0.0);
       Q.initialize( getParameter("l0"), L2,ScatAngle,
                                  0,peak_old.getInitialEnergy(),0.0);

       V3D Qvec = detDir -beamDir;//<--
       Qvec.normalize();
       Qvec = Qvec*Q.singleFromTOF( T0);
       peak.setWavelength( wl.singleFromTOF(T0));
        peak.setIntensity( peak_old.getIntensity());
        peak.setSigmaIntensity(peak_old.getSigmaIntensity());
        peak.setRunNumber(peak_old.getRunNumber());
        peak.setBinCount( peak_old.getBinCount());

        peak.setDetectorID( ID );

        std::cout<<"comparison before and after"<<std::endl;
        std::cout<<"pos"<<peak_old.getDetPos()<<peak.getDetPos()<<std::endl;

        std::cout<<"pos1"<<peak_old.getDetector()->getPos()
            <<peak.getDetector()->getPos()<<std::endl;

        std::cout<<"pos2"<<peak_old.getDetector()->getRelativePos()<<peak.getDetector()->getRelativePos()<<std::endl;
        std::cout<<"L0,wl,tof="<<peak_old.getL1()<<","<<peak_old.getWavelength()
               << ","<<peak_old.getTOF()<<"//"<<
              peak.getL1()<<","<<peak.getWavelength()
                       <<","<<peak.getTOF()<<std::endl;
        std::cout<<"Pos rel det center="<<
                (peak_old.getDetPos()-panelCenter_old)<<(peak.getDetPos()-panelCenter)<<std::endl;
        std::cout<<"relposAt vs pos"<<Rect->getRelativePosAtXY( peak.getCol(),peak.getRow())<<std::endl;
        std::cout<<"---------------------------------------------------"<<std::endl;

        double hkl1[3] = {hkl.X(), hkl.Y(), hkl.Z()};

        bool ok = true;
        for( int k =0; k<3 && ok ;k++) //eliminate tolerance cause only those peaks that are
                                       // OK should be here
        {
          double off =hkl1[k]-floor(hkl1[k]);
          if(off < tolerance)

            hkl1[k]=floor(hkl1[k]);

          else if(  1-off <tolerance)

            hkl1[k]= floor(hkl1[k])+1;

          else

            ok= false;
        }

        if( ok && (hkl.X()!=0 || hkl.Y() !=0 || hkl.Z() !=0))
        {

          hkl_vectors.push_back( Kernel::V3D( hkl1[0],hkl1[1],hkl1[2]));
          q_vectors.push_back( Qvec);
        }

     }


   /*  std::cout<<" hkl results="<<std::setw(20)<<"  q result="<<std::endl;
     for( int i=0; i< min<size_t>(hkl_vectors.size(),q_vectors.size()); i++)
       std::cout<<hkl_vectors[i]<<setw(20)<<q_vectors[i]<<std::endl;
  */
//----------------------------------Calculate out ----------------------------------
     Geometry::IndexingUtils::Optimize_UB (UB, hkl_vectors, q_vectors);

     Geometry::OrientedLattice lat;
     lat.setUB(UB);

     const Kernel::DblMatrix U =lat.getU();


     Kernel::DblMatrix UB0 = U*B0;

     double chiSq= 0;// for debug log message
     for( size_t i=0; i< q_vectors.size(); i++)
     {
           Kernel::V3D err =UB0*hkl_vectors[i]-q_vectors[i];

           out[3*i]=err[0];
           out[3*i+1]=err[1];
           out[3*i+2]=err[2];
           chiSq +=out[3*i]*out[3*i]+out[3*i+1]*out[3*i+1]+out[3*i+2]*out[3*i+2];

     }

     for( size_t i = 3*q_vectors.size(); i < nData; i++)
         out[i]=0;

     g_log.debug()<<"Parameters"<<std::endl;
     for( size_t i=0; i< this->nParams(); i++)
       g_log.debug()<<setw(20)<<this->parameterName(i)<<setw(20)<<this->getParameter(i)<<std::endl;

     g_log.debug()<<"      chi Squared="<<chiSq<<std::endl;
     std::cout<<std::endl;

  }




  DataObjects::Workspace2D_sptr SCDPanelErrors::calcWorkspace( DataObjects::PeaksWorkspace_sptr  pwks, std::vector< std::string>& bankNames,
      double tolerance)
  {
    int N=0;
    Mantid::MantidVecPtr pX;
    if( tolerance < 0)
      tolerance = .5;
    tolerance = min<double>(.5, tolerance);

    Mantid::MantidVec& xRef = pX.access();
    Mantid::MantidVecPtr yvals;
    Mantid::MantidVec &yvalB = yvals.access();

    for (size_t k = 0; k < bankNames.size(); k++)
        for (size_t j = 0; j < pwks->rowCount(); j++)
        {
          API::IPeak& peak = pwks->getPeak((int) j);
          if (peak.getBankName().compare(bankNames[k]) == 0)
            if (peak.getH() != 0 || peak.getK() != 0 || peak.getK() != 0)
              if (peak.getH() - floor(peak.getH()) < tolerance || floor(peak.getH() + 1) - peak.getH()
                  < tolerance)
                if (peak.getK() - floor(peak.getK()) < tolerance || floor(peak.getK() + 1) - peak.getK()
                    < tolerance)
                  if (peak.getL() - floor(peak.getL()) < tolerance || floor(peak.getL() + 1)
                      - peak.getL() < tolerance)
                  {
                    N++;
                    xRef.push_back((double) j);
                    xRef.push_back((double) j);
                    xRef.push_back((double) j);
                    yvalB.push_back(0.0);
                    yvalB.push_back(0.0);
                    yvalB.push_back(0.0);

                  }
        }



    MatrixWorkspace_sptr mwkspc = API::WorkspaceFactory::Instance().create("Workspace2D",(size_t)3,3*N,3*N);

    mwkspc->setX(0, pX);
    mwkspc->setX(1, pX);
    mwkspc->setX(2, pX);
    mwkspc->setData(0, yvals);
    mwkspc->setData(0, yvals);
    mwkspc->setData(0, yvals);

    return boost::dynamic_pointer_cast< DataObjects::Workspace2D >(mwkspc);
  }


  void SCDPanelErrors::setUpCommonData()
  {
    if( !CommonDataSetUp)
    {
      g_log.error("Workspace added more than once");
      throw logic_error("Workspace added more than once");
    }
    CommonDataSetUp = true;
  }
}
}
