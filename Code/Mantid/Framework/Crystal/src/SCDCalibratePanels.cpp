/*WIKI*


This algorithm calibrates sets of Rectangular Detectors in one instrument. The initial path, time offset,
panel width's, panel height's, panel locations and orientation are all adjusted so the error
in q from theoretical positions is minimized.

Some features
1) *Panels can be grouped. All panels in a group will move the same way and rotate the same way.  There height and
   widths will all change by the same factor

2) The user can select which quantities to adjust

3) The results can be save in an ISAW-like DetCal file or in an xml file that can be used in the
    LoadParameter algorithm.

3) *Results from a previous optimization can be applied before another optimization is done.

* means not fully implemented yet.

*WIKI*/


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
#include <iostream>
#include <fstream>
#include <math.h>
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Workspace.h"
#include <boost/algorithm/string.hpp>
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


  void SCDCalibratePanels::updateParams(  boost::shared_ptr<const Geometry::ParameterMap>    pmapSv,
                       boost::shared_ptr<Geometry::ParameterMap>   pmap,
                       boost::shared_ptr<const IComponent>  component)
   {
     set<string> pnamesSv = component->getParameterNames( false );

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



  void  SCDCalibratePanels::Quat2RotxRotyRotz(const Quat Q, double &Rotx,double &Roty,double &Rotz)
    {
      Quat R(Q);
      R.normalize();
      V3D X(1,0,0);
      V3D Y(0,1,0);
      V3D Z(0,0,1);
      R.rotate(X);
      R.rotate(Y);
      R.rotate(Z);
      if (Z[1] != 0 || Z[2] != 0)
      {
        double tx = atan2(-Z[1], Z[2]);
        double tz = atan2(-Y[0], X[0]);
        double cosy = Z[2] / cos(tx);
        double ty = atan2(Z[0], cosy);
        Rotx = (tx * 180 / M_PI);
        Roty = (ty * 180 / M_PI);
        Rotz = (tz * 180 / M_PI);
      }else //roty = 90 0r 270 def
      {
        double k=1;
        if( Z[0]<0)
         k=-1;
        double roty=k*90;
        double rotx=0;
        double rotz= atan2(X[2],Y[2]);

        Rotx = (rotx * 180 / M_PI);
        Roty = (roty * 180 / M_PI);
        Rotz = (rotz * 180 / M_PI);
      }
    }



  DataObjects::Workspace2D_sptr SCDCalibratePanels::calcWorkspace( DataObjects::PeaksWorkspace_sptr & pwks,
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

    for (size_t k = 0; k < bankNames.size(); k++)
    {
      for (size_t j = 0; j < (size_t)pwks->getNumberPeaks(); j++)
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
                     N += 3;
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



  void  SCDCalibratePanels::CalculateGroups(set<string>& AllBankNames, string Grouping, string bankPrefix,
        string bankingCode, vector<vector<string> > &Groups)
  {
    Groups.clear();

    if( Grouping =="OnePanelPerGroup")
    {
      for(set<string>::iterator it=AllBankNames.begin(); it !=AllBankNames.end(); it++)
      {
        string bankName = (*it);
        vector<string> vbankName;
        vbankName.push_back( bankName);
        Groups.push_back(vbankName);
      }

      }else if( Grouping == "AllPanelsInOneGroup")
      {  vector<string> vbankName;
         for(set<string>::iterator it=AllBankNames.begin(); it !=AllBankNames.end(); it++)
            {
              string bankName = (*it);

              vbankName.push_back( bankName);
            }

         Groups.push_back(vbankName);
      }else if( Grouping == "SpecifyGroups")
      {
        vector<string> GroupA;
        boost::split(GroupA, bankingCode, boost::is_any_of("]"));
        set<string> usedInts;

        for( size_t i = 0; i < GroupA.size(); i++)
        {
          string S = GroupA[i];

          while( S.length()>0 && S.at(0)==' ')
                     S = S.erase(0);
          if( S.length()>0 && S.at(0)=='[')
            S = S.erase(0);
          while( S.length()>0 && S.at(S.length()-1)==' ')
              S = S.erase(S.length()-1);
          if( S.length() > 0 )if(  S.at(S.length()-1)==']')
            S=S.erase(S.length()-1);

          vector<string>GroupB;
          boost::split( GroupB, S, boost::is_any_of(","));

          vector<string> Group0;
          for( size_t k=0; k < GroupB.size(); k++)
          {
            string id =GroupB[k];

            while( id.length()>0 && id.at(0)==' ')
              id = id.erase(0);
            while( id.length()>0 && id.at(id.length()-1)==' ')
              id = id.erase(id.length()-1);

            if( AllBankNames.find(bankPrefix+id) != AllBankNames.end())
              if( usedInts.find(id) ==usedInts.end())
              {
                Group0.push_back(bankPrefix+id);
                usedInts.insert( id);
              }
          }
          Groups.push_back(Group0);

        }
       }else
       {
         g_log.error("No mode "+Grouping+" defined yet");
         throw std::invalid_argument("No mode "+Grouping+" defined yet");
       }
  }




  void  SCDCalibratePanels::exec ()
  {
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




    set<string> AllBankNames;
    for( int i=0; i < peaksWs->getNumberPeaks(); i++)
      AllBankNames.insert( peaksWs->getPeak(i).getBankName());

    vector<vector<string> >Groups;
    CalculateGroups( AllBankNames, Grouping,bankPrefix,bankingCode, Groups);

    vector<string>banksVec;
    vector<vector<string> >::iterator it;
    for(  it = Groups.begin(); it != Groups.end(); it++)
    {

      for( vector<string>::iterator itt=(*it).begin(); itt!=(*it).end(); itt++)
      {
        banksVec.push_back( (*itt));
      }
    }

    vector<int>bounds;
    Workspace2D_sptr   ws = calcWorkspace( peaksWs, banksVec,tolerance,bounds);
    if( ws->getNumberHistograms() < 2)
    {
        g_log.error(" Not enough data to fit parameters ");
        throw std::length_error( " Not enough data to fit parameters " );
    }


    boost::shared_ptr<const Instrument> instrument = peaksWs->getPeak(0).getInstrument();

    AnalysisDataService::Instance().addOrReplace("xxx",peaksWs );

    double L0 =peaksWs->getPeak(0).getL1();
    double T0;
    string FunctionArgument;
    string Constraints("");
    string TiesArgument;
    int i=-1;//position in ParamResults Array.
    int nbanksSoFar =0;

   bool first= false;
   int NGroups=(int)Groups.size();
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
        BankNameString +="/";  //Fit strips " and commas are separators

      BankNameString +=(*it1);
      FrameworkManager::Instance();

    }

      // --- set Function property ----------------------

      ostringstream oss (ostringstream::out);
      oss.precision(4);
       if( i>0) oss<<";";
      oss<<"name=SCDPanelErrors, PeakWorkspaceName=\"xxx\",";
      oss<<"a="<<fixed<<a<<","<<"b="<<fixed<<b<<","<<"c="<<fixed<<c<<","<<"alpha="<<fixed<<alpha<<","<<"beta="<<fixed<<beta
           <<","<<"gamma="<<fixed<<gamma<<",";

      oss<<"l0="<<fixed<<L0<<","<<"t0="<<fixed<<T0<<","<<"detWidthScale="<<fixed<<1.0<<","<<"detHeightScale="<<fixed<<1.0<<","
          <<"Xoffset="<<0.0<<","<<"Yoffset="<<0.0<<","<<"Zoffset=";

      oss     <<0.0<<","<<"Xrot="<<0.0<<","<<"Yrot="<<0.0<<","<<"Zrot="<<0.0 <<",";
      int startX = bounds[nbanksSoFar];
      int endXp1 = bounds[nbanksSoFar+(*itv).size()];
      if( endXp1-startX < 12)
      {
        g_log.error()<<"Bank Group "<< BankNameString <<" does not have enough peaks for fitting"<<endl;
        throw new std::runtime_error("Group " + BankNameString +" does not have enough peaks");
      }
      oss<< "BankNames="<<"\""<<BankNameString<<"\""<<",startX="<<startX<<",endX="<<endXp1-1;
      nbanksSoFar = nbanksSoFar+(int)(*itv).size();

     FunctionArgument +=oss.str();

      string prefix ="";
      string prefix0 ="";
      if( NGroups >1)
      {
        char ichar[3] ;
        sprintf(ichar,"%d",i);

        prefix = "f"+string(ichar)+".";
        prefix0="f0.";
      }
      //---------- set Ties argument ----------------------------------
      ostringstream oss1 (ostringstream::out);

      oss1.precision(4);
      if( i==0)
      {
        first = true;

      }
      if(!use_L0 && i==0 )
      {
        if( !first)
          oss1<<",";
        first = false;
        oss1<<prefix0<<"l0="<<fixed<<L0;
      }
      if(!use_timeOffset && i==0 )
          {
            if( !first)
              oss1<<",";
            first = false;
            oss1<<prefix0<<"t0="<<T0;
          }

      if( i > 0)
      { if( !first)
          oss1<<",";
        first = false;
        oss1<<prefix<<"l0="<<prefix0<<"l0,";

        oss1<<prefix<<"t0="<<prefix0<<"t0";
        first = false;
      }
      if(!use_PanelWidth)
      {
        if( !first)
          oss1<<",";
        first = false;
        oss1<<prefix<<"detWidthScale="<<fixed<<1.0;
      }

      if(!use_PanelHeight)
      {
        if( !first)
          oss1<<",";
        first = false;
        oss1<<prefix<<"detHeightScale="<<fixed<<1.0;
      }

    if(!use_PanelPosition)
      {
        if( !first)
          oss1<<",";
        first = false;
        oss1<<prefix<<"Xoffset="<<0.0<<","<<prefix<<"Yoffset="<<0.0<<","<<prefix<<"Zoffset="<<0.0;
      }

      if(! use_PanelOrientation )
      {
        if( !first)
          oss1<<",";
        first = false;
        oss1<<prefix<<"Xrot="<<0.0<<","<<prefix<<"Yrot="<<0.0<<","<<prefix<<"Zrot="<<0.0;
      }

      TiesArgument +=oss1.str();
      //--------------- set Constraints Property  -------------------------------

      ostringstream oss2 (ostringstream::out);
      double maxXYOffset= 10*max<double>(bank_rect->xstep(), bank_rect->ystep());
      if( i !=0)
        oss2 <<",";
      oss2<< (.85*L0) << "<" <<prefix<< "l0<" << (1.15*L0 )<< "," << (.85)<<"<"<<prefix<<"detWidthScale<"<<(1.15)
              <<","<<(.85)<<"<"<<prefix<<"detHeightScale<"<<(1.15)<<",-5<"<<prefix<<"t0<5,"<<-maxXYOffset
              <<"<"<<prefix<<"Xoffset<"
              <<maxXYOffset<<",-"<<maxXYOffset<<"<"<<prefix<<"Yoffset<"<<maxXYOffset<<",-"<<maxXYOffset
              <<"<"<<prefix<<"Zoffset<"
              <<maxXYOffset<<",-10<"<<prefix<<"Xrot<10,-10<"<<prefix<<"Yrot<10,-10<"<<prefix<<"Zrot<10";

      Constraints+=oss2.str();

   }//for vector<string> in Groups

   boost::shared_ptr<Algorithm> fit_alg = createSubAlgorithm("Fit",.2,.9,true);

   if( ! fit_alg)
   {
     g_log.error("Cannot find Fit algorithm");
     throw invalid_argument("Cannot find Fit algorithm" );
   }

   fit_alg->setProperty("InputWorkspace", ws);
   fit_alg->setProperty("WorkspaceIndex", 0);
   fit_alg->setProperty("MaxIterations", 20);
   fit_alg->setProperty("Function",FunctionArgument);
   if(TiesArgument.size() > 0)
       fit_alg->setProperty("Ties",TiesArgument);

   fit_alg->setProperty("Constraints", Constraints);

   fit_alg->initialize();


   fit_alg->executeAsSubAlg();


    //double chisq = fit_alg->getProperty("OutputChi2overDoF");
    std::vector<double>params = fit_alg->getProperty("Parameters");
    std::vector<double>errs = fit_alg->getProperty("Errors");
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
    oss3<<"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"<<std::endl;
    oss3<<" <parameter-file instrument=\""<<instName<<"\" date=\"2011-01-21T00:00:00\">"<<std::endl;


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
       Quat RelRot = bank->getRelativeRot();

       double rotx,roty,rotz;
       char ichar[3] ;
       sprintf(ichar,"%d",i);
       string prefix="";
       if( NGroups >1)
       {
         prefix = "f"+string(ichar)+".";
       }
       string istring(ichar);
        rotx=result[prefix+"Xrot"];

        roty= result[prefix+"Yrot"];
        rotz= result[prefix+"Zrot"];
        Quat newRelRot = Quat(rotx,V3D(1,0,0))*Quat(roty,V3D(0,1,0))*Quat(rotx,V3D(0,0,1))*RelRot;
        Quat2RotxRotyRotz( newRelRot, rotx,roty,rotz);

       pmap->addRotationParam( bank.get(),"string(rotx)", rotx);
       pmap->addRotationParam( bank.get(),"string(roty)", roty);
       pmap->addRotationParam( bank.get(),"string(rotz)", rotz);
       pmap->addQuat( bank.get(),"rot",newRelRot);//Should not have had to do this???

       oss3<<"  <parameter name =\"rotx\"><value val=\""<<rotx<<"\" /> </parameter>"<<std::endl;
       oss3<<"  <parameter name =\"roty\"><value val=\""<<roty<<"\" /> </parameter>"<<std::endl;
       oss3<<"  <parameter name =\"rotz\"><value val=\""<<rotz<<"\" /> </parameter>"<<std::endl;


       V3D pos1 = bank->getRelativePos();

       pmap->addPositionCoordinate(bank.get(), string("x"), result[prefix+"Xoffset"] + pos1.X());
       pmap->addPositionCoordinate(bank.get(), string("y"),result[prefix+"Yoffset"] + pos1.Y());
       pmap->addPositionCoordinate(bank.get(), string("z"),result[prefix+"Zoffset"]+ pos1.Z());

       oss3<<"  <parameter name =\"x\"><value val=\""<<result[prefix+"Xoffset"] + pos1.X()<<"\" /> </parameter>"<<std::endl;

       oss3<<"  <parameter name =\"y\"><value val=\""<<result[prefix+"Yoffset"] + pos1.Y()<<"\" /> </parameter>"<<std::endl;

       oss3<<"  <parameter name =\"z\"><value val=\""<<result[prefix+"Zoffset"]+ pos1.Z()<<"\" /> </parameter>"<<std::endl;


       vector<double>oldScalex = pmap->getDouble( bank->getName(),string("scalex"));
       vector<double>oldScaley = pmap->getDouble( bank->getName(),string("scaley"));

       double scalex,scaley;
       if( oldScalex.size()>0)
         scalex= oldScalex[0]+result[prefix+"detWidthScale"];
       else
         scalex=result[prefix+"detWidthScale"];

       if( oldScaley.size()>0)
         scaley= oldScaley[0]+result[prefix+"detHeightScale"];
       else
         scaley= result[prefix+"detHeightScale"];

       pmap->addDouble( bank.get(),string("scalex"), scalex);
       pmap->addDouble( bank.get(),string("scaley"), scaley);
       oss3<<"  <parameter name =\"scalex\"><value val=\""<<scalex<<"\" /> </parameter>"<<std::endl;
       oss3<<"  <parameter name =\"scaley\"><value val=\""<<scaley<<"\" /> </parameter>"<<std::endl;
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
    string prefix ="";
    if( NGroups > 1)
      prefix ="f0.";
    double scalee = result[prefix + "l0"]/source2sampleDir.norm();
    V3D newsourcePos = sample->getPos()- source2sampleDir*scalee;
    V3D newsourceRelPos = newsourcePos-parentSourcePos;

    pmap->addPositionCoordinate(source.get(), string("x"), newsourceRelPos.X());
    pmap->addPositionCoordinate(source.get(), string("y"), newsourceRelPos.Y());
    pmap->addPositionCoordinate(source.get(), string("z"),newsourceRelPos.Z());
    oss3<<"  < parameter name =\"x\"><value val=\""<< newsourceRelPos.X()<<"\" /> </parameter>"<<std::endl;

    oss3<<"  < parameter name =\"y\"><value val=\""<< newsourceRelPos.Y()<<"\" /> </parameter>"<<std::endl;

    oss3<<"  < parameter name =\"z\"><value val=\""<<newsourceRelPos.Z()<<"\" /> </parameter>"<<std::endl;


    oss3<<"</component-link>"<<std::endl;

    oss3<<"</parameter-file>"<< std::endl;

    string DetCalFileName = getProperty("DetCalFilename");
    string XmlFileName = getProperty("XmlFilename");
    if( DetCalFileName.size()>0)
    {

     boost::shared_ptr<Algorithm>fit_alg = createSubAlgorithm("SaveIsawDetCal");

     peaksWs->setInstrument( NewInstrument);
     fit_alg->initialize();
     fit_alg->setProperty("InputWorkspace",peaksWs);

     fit_alg->setProperty("Filename", DetCalFileName);
     string TT0="t0";
     if( NGroups > 1) TT0="f0.t0";

     fit_alg->setProperty("TimeOffset",result[TT0]);

     fit_alg->executeAsSubAlg();

    }


    if( XmlFileName.size() > 0)
    {

      filebuf fb;
      fb.open (XmlFileName.c_str(),ios::out);
      ostream os(&fb);
      os << oss3.str();
      fb.close();

    }
  }



   void  SCDCalibratePanels::init ()
   {
     declareProperty(new WorkspaceProperty<PeaksWorkspace> ("PeakWorkspace", "", Kernel::Direction::Input),
                                                                     "Workspace of Peaks");
     vector<string>choices;
     choices.push_back( "OnePanelPerGroup");
     choices.push_back( "AllPanelsInOneGroup");
     choices.push_back("SpecifyGroups");
     declareProperty(string("PanelGroups"),string("OnePanelPerGroup"),
           boost::make_shared<Kernel::StringListValidator>(choices),"Select grouping of Panels");

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
     exts.push_back(".Det_Cal");
     declareProperty(new FileProperty("DetCalFilename", "", FileProperty::Save, exts),
                                          "Path to an ISAW-style .detcal file to save.");
     vector<string>exts1;
     exts1.push_back(".xml");

     declareProperty(new FileProperty("XmlFilename", "", FileProperty::Save, exts1),
                                          "Path to an Mantid .xml description(for LoadParameterFile) file to save.");
      vector<string> preProcessOptions;
      preProcessOptions.push_back("No PreProcessing");
      preProcessOptions.push_back("Apply a ISAW.DetCal File");
      preProcessOptions.push_back("Apply a LoadParameter.xml type file");


      declareProperty("PreProcessInstrument","No PreProcessing",
          boost::make_shared<Kernel::StringListValidator>(preProcessOptions),"Select PreProcessing info");
      vector<string>exts2;
           exts2.push_back(".DetCal");
           exts2.push_back(".xml");
      declareProperty(new FileProperty("PreProcFilename", "", FileProperty::Save, exts2),
                                          "Path to file with preprocessing information");

      declareProperty( "InitialTimeOffset",0, "Initial time offset when using xml files");
      setPropertySettings("PanelNamePrefix", new EnabledWhenProperty(this, "PanelGroups", Kernel::IS_EQUAL_TO, "SpecifyGroups") );
      setPropertySettings("Grouping", new EnabledWhenProperty(this, "PanelGroups", Kernel::IS_EQUAL_TO, "SpecifyGroups") );

      setPropertySettings("PreProcFilename", new EnabledWhenProperty(this, "PreProcessInstrument", Kernel::IS_NOT_EQUAL_TO, "No PreProcessing") );
      setPropertySettings("InitialTimeOffset",new EnabledWhenProperty(this, "PreProcessInstrument", Kernel::IS_EQUAL_TO, "Apply LoadParameter.xml type file")) ;

   }

  void  SCDCalibratePanels::initDocs ()
  {
    this->setWikiSummary("Calibrates Panels for Rectangular Detectors only");
    this->setOptionalMessage("Panel parameters, L0 and T0 are optimized to minimize errors between theoretical and actual q values for the peaks");

  }


}//namespace Crystal
}//namespace Mantid
