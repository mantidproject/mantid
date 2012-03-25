/*
 * SCDCalibratePanels.cpp
 *
 *  Created on: Mar 12, 2012
 *      Author: ruth
 */

#include "MantidCrystal/SCDCalibratePanels.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/Quat.h"
#include "MantidGeometry/Instrument.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ValidatorAnyList.h"
#include <iostream>
#include <fstream>
#include <math.h>
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Workspace.h"
using namespace Mantid::DataObjects;
using namespace std;
using namespace  Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace Mantid
{
namespace Crystal

{
  Kernel::Logger& SCDCalibratePanels::g_log = Kernel::Logger::get("SCDCalibratePanels");

  DECLARE_ALGORITHM(SCDCalibratePanels)

  SCDCalibratePanels::SCDCalibratePanels():API::Algorithm()
  {


  }

  SCDCalibratePanels::~SCDCalibratePanels()
  {

  }

  void updateParams(  boost::shared_ptr<const Geometry::ParameterMap>    pmapSv,
                       boost::shared_ptr<Geometry::ParameterMap>   pmap,
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
       {
         N++;
         pmap->addV3D( component.get(), name, posParams[0]);
       }

       vector<Quat> rotParams = component->getRotationParameter(name , false);

       if( rotParams.size() > 0)
       {
         N++;
         pmap->addQuat( component.get(), name, rotParams[0]);
       }

       vector<string> strParams = component->getStringParameter(name,false);

       if( strParams.size() > 0)
       {
         N++;
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
  DataObjects::Workspace2D_sptr calcWorkspace( DataObjects::PeaksWorkspace_sptr & pwks,
                                                                std::vector< std::string>& bankNames,
                                                                 double tolerance, vector<int>&bounds)
   {
     int N=0;
     Mantid::MantidVecPtr pX;
     if( tolerance < 0)
       tolerance = .5;
     tolerance = min<double>(.5, tolerance);

     Mantid::MantidVec& xRef = pX.access();
     Mantid::MantidVecPtr yvals;
     Mantid::MantidVecPtr errs;
     Mantid::MantidVec &yvalB = yvals.access();
     Mantid::MantidVec &errB = errs.access();
    bounds.clear();
    bounds.push_back(0);
    std::cout<<"bankNames.size()"<<std::endl;
    for (size_t k = 0; k < bankNames.size(); k++)
    { //std::cout<<"bankName,tolerance "<<bankNames[k]<<"/"<<tolerance;
      for (size_t j = 0; j < pwks->getNumberPeaks(); j++)
         {
           API::IPeak& peak = pwks->getPeak((int) j);
           std::cout<<" "<<peak.getBankName()<<std::endl;
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
                     errB.push_back(1.0);
                     errB.push_back(1.0);
                     errB.push_back(1.0);

                   }
         }//for @ peak
      bounds.push_back(N);
    }//for @ bank name

     MatrixWorkspace_sptr mwkspc;

     if( N < 4)
       return boost::shared_ptr<DataObjects::Workspace2D>(new DataObjects::Workspace2D);


     mwkspc= API::WorkspaceFactory::Instance().create("Workspace2D",(size_t)3,3*N,3*N);

     mwkspc->setX(0, pX);
     mwkspc->setX(1, pX);
     mwkspc->setX(2, pX);
     mwkspc->setData(0, yvals,errs);
     mwkspc->setData(1, pX);
     mwkspc->setData(2, yvals);

     return boost::dynamic_pointer_cast< DataObjects::Workspace2D >(mwkspc);
   }

  void CalculateGroups(set<string> AllBankNames, string Grouping, string bankPrefix,
        string bankingCode, vector<vector<string> > &Groups)
  {
    Groups.clear();
    cout<<"Grouping name="<<Grouping<<endl;
    if( Grouping =="OnePanelPerGroup")
    {
      for(set<string>::iterator it=AllBankNames.begin(); it !=AllBankNames.end(); it++)
      {
        string bankName = (*it);
        vector<string> vbankName;
        vbankName.push_back( bankName);
        Groups.push_back(vbankName);
      }
      std::cout<< "Groups size="<<Groups.size()<<endl;
      std::cout<<"Groups[0].size="<<Groups[0].size()<<","<<Groups[1].size()<<","<<Groups[1].size()<<endl;
      }else if( Grouping == "AllPanelsInOneGroup")
      {  vector<string> vbankName;
         for(set<string>::iterator it=AllBankNames.begin(); it !=AllBankNames.end(); it++)
            {
              string bankName = (*it);

              vbankName.push_back( bankName);
            }

         Groups.push_back(vbankName);
      }else
      {
       // g_log.error("Grouping "+bankingCode +" not supported yet");
        throw std::invalid_argument("Grouping "+bankingCode +" not supported yet");

       }
  }
  void  SCDCalibratePanels::exec ()
  {/*//Testing to get Rotx*Roty*Rotz = Quat
    Quat Rot(0.790991,0.0574798,0.462807,0.396029);

          Quat Rot1(Rot);
          double deg,ax,ay,az;
          Rot.getAngleAxis(deg,ax,ay,az);

          V3D origLine(ax,ay,az);
          cout<<"??? same Quats"<< Quat(deg, V3D(ax,ay,az))<< Rot<<endl;
          cout<<"original Rot="<<Rot<<endl;
          std::cout<<" Rot's deg,ax0,ax1,ax2="<<deg<<","<<ax<<","<<ay<<","<<az<<endl;
          double rotx,roty,rotz;
          rotx= atan2(ay,az)*180/M_PI;//Fixup cause not wrt to y-axis
          V3D dir1(ax,ay,az);
          dir1.normalize();
          Quat Z(rotx, V3D(1,0,0));Z.normalize();
          Quat Z8(Z);
          Z8.inverse();
          Rot1= Z*Rot1;Rot1.normalize();
          Rot1.getAngleAxis(deg,ax,ay,az);
          cout<<"Quat after Rotx="<<Rot1<<deg<<","<<ax<<","<<ay<<","<<az<<endl;
          V3D Xx( origLine);
          Rot1.rotate( Xx);
          cout<<" dir1 aft 1st rot of Quad"<<Xx<<endl;
          Z.rotate(dir1);
          cout<<"dir1 aft 1st rotation="<<dir1<<endl;
          roty=-atan2( dir1.X(),dir1.Z())*180/M_PI;
          Quat Z1(roty, V3D(0,1,0));Z1.normalize();
          Z1.rotate(dir1);
          cout<<"dir1 aft 2nd rotation="<<dir1<<endl;
          Rot1 = Z1*Rot1; Rot1.normalize();
          Xx = origLine;
          Rot1.rotate(Xx);
          cout<<"dir1 aft 2nd rot of Quad"<< Xx<<endl;
          rotz =deg;
          std::cout<<"rotxyz="<<rotx<<","<<roty<<","<<rotz<<endl;
          (Z1*Z).rotate(origLine);
          cout<<"orig line --> aft 2 rotation="<<origLine<<endl;
          cout<<" rel rotation ="<< (Quat(rotx,V3D(1,0,0))*Quat(roty,V3D(0,1,0))*Quat(rotz,V3D(0,0,1)))<<endl;
          cout<<"check?="<<(Quat(rotz,V3D(0,0,1))*Quat(roty,V3D(0,1,0))*Quat(rotx,V3D(1,0,0))*Rot)<<endl;
          */
    PeaksWorkspace_sptr peaksWs =getProperty("PeakWorkspace");

    double a = getProperty("a");
    double b = getProperty("b");
    double c = getProperty("c");
    double alpha = getProperty("alpha");
    double beta = getProperty("beta");
    double gamma = getProperty("gamma");


    double tolerance = getProperty("tolerance");


    bool use_L0 = getProperty("use_L0");
    bool use_timeOffset = getProperty("use_timeOffset");
    bool use_PanelWidth = getProperty("use_PanelWidth");
    bool use_PanelHeight = getProperty("use_PanelHeight");
    bool use_PanelPosition = getProperty("use_PanelPosition");
    bool use_PanelOrientation = getProperty("use_PanelOrientation");
    string Grouping = getProperty( "PanelGroups");
    string bankPrefix = getProperty("PanelNamePrefix");
    string bankingCode = getProperty("Grouping");
    std::cout<<"Grouping="<<Grouping<<endl;
    const int NParameters =10;


    set<string> AllBankNames;
    for( int i=0; i < peaksWs->getNumberPeaks(); i++)
      AllBankNames.insert( peaksWs->getPeak(i).getBankName());

    vector<vector<string> >Groups;
    CalculateGroups( AllBankNames, Grouping,bankPrefix,bankingCode, Groups);

    vector<string>banksVec;
    vector<vector<string> >::iterator it;
    for(  it = Groups.begin(); it != Groups.end(); it++)
    {
      cout<<"in Group iterator size of subgroup="<< (*it).size()<<endl;
      for( vector<string>::iterator itt=(*it).begin(); itt!=(*it).end(); itt++)
      { cout<<"banks="<<(*itt)<<endl;
        banksVec.push_back( (*itt));
      }
    }

    vector<int>bounds;
    Workspace2D_sptr   ws = calcWorkspace( peaksWs, banksVec,tolerance,bounds);
    cout<<"BOUNDS size="<<bounds.size()<<endl;
    for(int i=0;i<bounds.size();i++)
      cout<<"bounds["<<i<<"]="<<bounds[i]<<endl;
    if( ws->getNumberHistograms() < 2)
    {
        g_log.error(" Not enough data to fit parameters ");
        throw std::length_error( " Not enough data to fit parameters " );
           //ParamResults[i][1]=ParamResults[i][4]=ParamResults[i][5]=ParamResults[i][6]=ParamResults[i][7]
           //       =ParamResults[i][8]=ParamResults[i][9]=ParamResults[i][10]=0;
           //      continue;// Not enough to do fitting
    }


 /*   map<string,int>name2indxMap;
    name2indxMap["l0"]=0;
    name2indxMap["t0"]=1;
    name2indxMap["detWidthScale"]=2;
    name2indxMap["detHeightScale"]=3;
    name2indxMap["Xoffset"]=4;
    name2indxMap["Yoffset"]=5;
    name2indxMap["Zoffset"]=6;
    name2indxMap["Xrot"]=7;
    name2indxMap["Yrot"]=8;
    name2indxMap["Zrot"]=9;


*/
    cout<<"F"<<endl;
    boost::shared_ptr<const Instrument> instrument = peaksWs->getPeak(0).getInstrument();

    AnalysisDataService::Instance().addOrReplace("xxx",peaksWs );
    cout<<"G"<<endl;
    double L0 =peaksWs->getPeak(0).getL1();
    double T0 = 0;
    string FunctionArgument;
    string Constraints("");
    string TiesArgument;
    int i=-1;//position in ParamResults Array.
    int nbanksSoFar =0;
   for( vector<vector<string> >::iterator itv=Groups.begin(); itv !=Groups.end();itv++)
   {
     i++;
     string BankNameString="";
     boost::shared_ptr<const RectangularDetector> bank_rect;
    for( vector<string>::iterator it1 = (*itv).begin(); it1 !=(*itv).end(); it1++)
    {

     boost::shared_ptr<const IComponent> bank_cmp = instrument->getComponentByName((*it1));
     bank_rect=
          boost::dynamic_pointer_cast<const RectangularDetector>( bank_cmp);

      if( !bank_rect)
      {
        g_log.error("No Rectangular detector bank "+ banksVec[0]+ " in instrument");
        throw std::invalid_argument("No Rectangular detector bank "+ banksVec[0]+ " in instrument");
      }
      if( BankNameString.size()>0)
        BankNameString +=",";
      BankNameString +=(*it1);
      FrameworkManager::Instance();

    }

      // --- set Function property ----------------------
      cout<<"J"<<endl;
      ostringstream oss (ostringstream::out);
      oss.precision(4);
       if( i>0) oss<<";";
      oss<<"name=SCDPanelErrors, PeakWorkspaceName=\"xxx\",";
      oss<<"a="<<fixed<<a<<","<<"b="<<fixed<<b<<","<<"c="<<fixed<<c<<","<<"alpha="<<fixed<<alpha<<","<<"beta="<<fixed<<beta
           <<","<<"gamma="<<fixed<<gamma<<",";

      oss<<"l0="<<fixed<<L0<<","<<"t0="<<fixed<<0.0<<","<<"detWidthScale="<<fixed<<1.0<<","<<"detHeightScale="<<fixed<<1.0<<","
          <<"Xoffset="<<0.0<<","<<"Yoffset="<<0.0<<","<<"Zoffset=";

      oss     <<0.0<<","<<"Xrot="<<0.0<<","<<"Yrot="<<0.0<<","<<"Zrot="<<0.0 <<",";


      oss<< "BankNames="<<BankNameString<<",startX="<<bounds[nbanksSoFar]<<",endX="<<bounds[nbanksSoFar+(*itv).size()]-1;
      nbanksSoFar = nbanksSoFar+(*itv).size();

     FunctionArgument +=oss.str();


      //---------- set Ties argument ----------------------------------
      ostringstream oss1 (ostringstream::out);

      oss1.precision(4);
      bool first =false;
      if( i==0)
      {
        first = true;
        cout<<"first for ties is true"<<endl;
      }
      if(!use_L0 )
      {
        if( !first)
          oss1<<",";
        first = false;
        oss1<<"f"<<0<<".l0="<<fixed<<L0;
      }
      if(!use_timeOffset)
          {
            if( !first)
              oss1<<",";
            first = false;
            oss1<<"f"<<0<<".t0="<<0.0;
          }


      { if( !first)
          oss1<<",";
        first = false;
        oss1<<"f"<<i<<".l0="<<"f"<<0<<".l0,";

        oss1<<"f"<<i<<".t0="<<"f"<<0<<".t0";
        first = false;
      }
      if(!use_PanelWidth)
      {
        if( !first)
          oss1<<",";
        first = false;
        oss1<<"f"<<i<<".detWidthScale"<<fixed<<1.0;
      }

      if(!use_PanelHeight)
      {
        if( !first)
          oss1<<",";
        first = false;
        oss1<<"f"<<i<<".detHeight="<<fixed<<1.0;
      }

      if(!use_PanelPosition)
      {
        if( !first)
          oss1<<",";
        first = false;
        oss1<<"f"<<i<<".Xoffset="<<0.0<<","<<",f"<<i<<".Yoffset="<<0.0<<","<<",f"<<i<<".Zoffset="<<0.0;
      }

      if(! use_PanelOrientation )
      {
        if( !first)
          oss1<<",";
        first = false;
        oss1<<"f"<<i<<".Xrot="<<0.0<<",f"<<i<<".Yrot="<<0.0<<",f"<<i<<".Zrot="<<0.0;
      }

      TiesArgument +=oss1.str();



      //--------------- set Constraints Property  -------------------------------

      ostringstream oss2 (ostringstream::out);
      double maxXYOffset= 10*max<double>(bank_rect->xstep(), bank_rect->ystep());
      if( i !=0)
        oss2 <<",";
      oss2<< (.85*L0) << "<f" <<i<< ".l0<" << (1.15*L0 )<< "," << (.85)<<"<f"<<i<<".detWidthScale<"<<(1.15)
              <<","<<(.85)<<"<f"<<i<<".detHeightScale<"<<(1.15)<<",-5<f"<<i<<".t0<5,"<<-maxXYOffset
              <<"<f"<<i<<".Xoffset<"
              <<maxXYOffset<<",-"<<maxXYOffset<<"<f"<<i<<".Yoffset<"<<maxXYOffset<<",-"<<maxXYOffset
              <<"<f"<<i<<".Zoffset<"
              <<maxXYOffset<<",-10<f"<<i<<".Xrot<10,-10<f"<<i<<".Yrot<10,-10<f"<<i<<".Zrot<10";

      Constraints+=oss2.str();






/*      for( int ii=0; ii< names.size();ii++)
      {
        int indx = name2indxMap[names[ii]];
       // std::cout<<"setting params in base="<<ii<<","<<indx<<","<<names[ii]<<","<<params[indx]<<std::endl;
        if( indx <NParameters)
        {
          ParamResults[i][2*indx]=params[ii];
          ParamResults[i][2*indx+1]=errs[ii];

        }
      }
      ParamResults[i][2*NParameters]= chisq;
*/



   }//for vector<string> in Groups
   boost::shared_ptr<Algorithm> fit_alg = createSubAlgorithm("Fit");

   if( ! fit_alg)
   {
     g_log.error("Cannot find Fit algorithm");
     throw invalid_argument("Cannot find Fit algorithm" );
   }
   std::cout<<"Function="<<FunctionArgument<<std::endl;
      std::cout<<"Ties="<<TiesArgument<<std::endl;
      std::cout<<"Constraints="<<Constraints<<std::endl;

   fit_alg->setProperty("InputWorkspace", ws);
   fit_alg->setProperty("WorkspaceIndex", 0);
   fit_alg->setProperty("MaxIterations", 100);
   fit_alg->setProperty("Function",FunctionArgument);
   if(TiesArgument.size() > 0)
       fit_alg->setProperty("Ties",TiesArgument);

   fit_alg->setProperty("Constraints", Constraints);

   fit_alg->initialize();

   fit_alg->executeAsSubAlg();

    double chisq = fit_alg->getProperty("OutputChi2overDoF");
    std::vector<double>params = fit_alg->getProperty("Parameters");
 //      std::vector<double>errs = fit_alg->getProperty("Errors");
    std::vector<string>names = fit_alg->getProperty("ParameterNames");

    map<string,double> result;
    for( size_t i=0; i<min<size_t>(params.size(),names.size());i++)
      result[names[i]]=params[i];

    boost::shared_ptr<ParameterMap> pmap( new ParameterMap());
    boost::shared_ptr<ParameterMap> pmapOld = instrument->getParameterMap();
    updateParams( pmapOld, pmap, boost::dynamic_pointer_cast<const IComponent>(instrument));

    boost::shared_ptr<const Instrument> NewInstrument(new Instrument( instrument->baseInstrument(), pmap));


    string instName = instrument->getName();

    ostringstream oss3 (ostringstream::out);
    oss3<<" <?xml version=\"1.0\" encoding=\"UTF-8\" ?>"<<std::endl;
    oss3<<" <parameter-file instrument=\""<<instName<<"\" date=\"blah...\">"<<std::endl;


    i=-1;
    for( vector<vector<string> >::iterator itv=Groups.begin(); itv !=Groups.end();itv++)
    {
      i++;
      string BankNameString="";
      boost::shared_ptr<const RectangularDetector> bank_rect;
     for( vector<string>::iterator it1 = (*itv).begin(); it1 !=(*itv).end(); it1++)

    {

      string bankName = (*it1);


       oss3<<"<component-link name=\""<<bankName<<"\">"<<std::endl;
       boost::shared_ptr<const IComponent> bank = NewInstrument->getComponentByName( bankName);

       vector<double> oldRotx =pmap->getDouble(bank->getName(),string("rotx"));
       vector<double> oldRoty =pmap->getDouble(bank->getName(),string("roty"));
       vector<double> oldRotz =pmap->getDouble(bank->getName(),string("rotz"));
       Quat Rot = bank->getRelativeRot();
       Quat Rot1(Rot);
       double deg,ax,ay,az;
       Rot.getAngleAxis(deg,ax,ay,az);

       double rotx,roty,rotz;
       char ichar[3] ;
       sprintf(ichar,"%d",i);
       string istring(ichar);
       if( oldRotx.size()>0)
        rotx = oldRotx[0]+result["f"+istring+".Xrot"];
       else
         rotx=result["f"+istring+".Xrot"];
         pmap->addRotationParam( bank.get(),string("rotx"), rotx);
       if( oldRoty.size()>0)
        roty= oldRoty[0]+result["f"+istring+".Yrot"];
       else
         roty= result["f"+istring+".Yrot"];
       pmap->addRotationParam( bank.get(),string("roty"), roty);
       if( oldRotz.size()>0)
         rotz= oldRotz[0]+result["f"+istring+".Zrot"];
       else
         rotz= result["f"+istring+".Zrot"];

       pmap->addRotationParam( bank.get(),"string(rotz)", rotz);
       oss3<<"  < parameter name =\"rotx\"><value val=\""<<rotx<<"\" /> </parameter>"<<std::endl;
       oss3<<"  < parameter name =\"roty\"><value val=\""<<roty<<"\" /> </parameter>"<<std::endl;

       oss3<<"  < parameter name =\"rotz\"><value val=\""<<rotz<<"\" /> </parameter>"<<std::endl;


       V3D pos1 = bank->getRelativePos();

       pmap->addPositionCoordinate(bank.get(), string("x"), result["f"+istring+".Xoffset"] + pos1.X());
       pmap->addPositionCoordinate(bank.get(), string("y"),result["f"+istring+".Yoffset"] + pos1.Y());
       pmap->addPositionCoordinate(bank.get(), string("z"),result["f"+istring+".Zoffset"]+ pos1.Z());

       oss3<<"  < parameter name =\"x\"><value val=\""<<result["f"+istring+".Xoffset"] + pos1.X()<<"\" /> </parameter>"<<std::endl;

       oss3<<"  < parameter name =\"y\"><value val=\""<<result["f"+istring+".Yoffset"] + pos1.Y()<<"\" /> </parameter>"<<std::endl;

       oss3<<"  < parameter name =\"z\"><value val=\""<<result["f"+istring+".Zoffset"]+ pos1.Z()<<"\" /> </parameter>"<<std::endl;


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
       oss3<<"  < parameter name =\"scalex\"><value val=\""<<scalex<<"\" /> </parameter>"<<std::endl;
       oss3<<"  < parameter name =\"scaley\"><value val=\""<<scaley<<"\" /> </parameter>"<<std::endl;
       oss3<<"</component-link>"<<std::endl;


    }//For @ bank
    }//For @ group
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
    oss3<<"  < parameter name =\"x\"><value val=\""<< newsourceRelPos.X()<<"\" /> </parameter>"<<std::endl;

    oss3<<"  < parameter name =\"y\"><value val=\""<< newsourceRelPos.Y()<<"\" /> </parameter>"<<std::endl;

    oss3<<"  < parameter name =\"z\"><value val=\""<<newsourceRelPos.Z()<<"\" /> </parameter>"<<std::endl;


    oss3<<"</component-link>"<<std::endl;
    std::cout<<"after setting instrument"<<std::endl;
    oss3<<"</parameter-file>"<< std::endl;
    //peaksWs->setInstrument( NewInstrument);
    string DetCalFileName = getProperty("DetCalFilename");
    string XmlFileName = getProperty("XmlFilename");
    if( DetCalFileName.size()>0)
    {std::cout<< "DetCal= "<<DetCalFileName<<endl;
     boost::shared_ptr<Algorithm>fit_alg = createSubAlgorithm("SaveIsawDetCal");

     Workspace2D_sptr ws( new Workspace2D());
     ws->setInstrument(  NewInstrument);

     fit_alg->setProperty("InputWorkspace",ws);

     fit_alg->setProperty("Filename", DetCalFileName);

     fit_alg->initialize();
     fit_alg->setProperty("TimeOffset",1.5);

     fit_alg->executeAsSubAlg();

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

   void  SCDCalibratePanels::init ()
   {
     declareProperty(new WorkspaceProperty<PeaksWorkspace> ("PeakWorkspace", "", Kernel::Direction::Input),
              "                                              Workspace of Peaks");
     vector<string>choices;
     choices.push_back( "OnePanelPerGroup");
     choices.push_back( "AllPanelsInOneGroup");
     choices.push_back("SpecifyGroups");
     declareProperty(string("PanelGroups"),string("OnePanelPerGroup"),new Kernel::ListValidator(choices),"Select grouping of Panels");

     declareProperty("PanelNamePrefix","bank","Prefix for the names of panels(followed by a number)");
     declareProperty("Grouping", "1:20,22;3,5,7",
                          "A semicolon separated list of groupings( comma or :-range separated");

     declareProperty("a", 0.0, "Lattice Parameter a");
     declareProperty("b", 0.0, "Lattice Parameter b");
     declareProperty("c", 0.0, "Lattice Parameter c");
     declareProperty("alpha", 0.0, "Lattice Parameter alpha in degrees");
     declareProperty("beta", 0.0, "Lattice Parameter beta in degrees");
     declareProperty("gamma", 0.0, "Lattice Parameter gamma in degrees");

     declareProperty("use_L0", true,"Fit the L0(source to sample) distance");
     declareProperty("use_timeOffset", true,"Fit the time offset value");
     declareProperty("use_PanelWidth", true,"Fit the Panel Width value");
     declareProperty("use_PanelHeight", true,"Fit the Panel Height");
     declareProperty("use_PanelPosition", true,"Fit the PanelPosition");
     declareProperty("use_PanelOrientation", true,"Fit the PanelOrientation");

     declareProperty("tolerance", .12,"offset of hkl values from integer for GOOD Peaks");
     vector<string>exts;
     exts.push_back(".DetCal");
     declareProperty(new FileProperty("DetCalFilename", "", FileProperty::Save, exts),
                                          "Path to an ISAW-style .detcal file to save.");
     vector<string>exts1;
     exts1.push_back(".xml");

     declareProperty(new FileProperty("XmlFilename", "", FileProperty::Save, exts1),
                                          "Path to an Mantid .xml description(for LoadInstrument) file to save.");


//                           Doesn't work
      setPropertySettings("PanelNamePrefix", new EnabledWhenProperty(this, "PanelGroups", Kernel::IS_EQUAL_TO, "SpecifyGroups") );
      setPropertySettings("Grouping", new EnabledWhenProperty(this, "PanelGroups", Kernel::IS_EQUAL_TO, "SpecifyGroups") );


   }

  void  SCDCalibratePanels::initDocs ()
  {
    this->setWikiSummary("Calibrates Panels for Rectangular Detectors only");
    this->setOptionalMessage("Panel parameters, L0 and T0 are optimized to minimize errors between theoretical and actual q values for the peaks");

  }


}//namespace Crystal
}//namespace Mantid
