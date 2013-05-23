/*WIKI*


This algorithm calibrates sets of Rectangular Detectors in one instrument.
The initial path, time offset,panel width's, panel height's, panel locations and orientation are all
adjusted so the error in q positions from the theoretical q positions is minimized.  Also, there
are options to optimize taking into account sample position and to have the rotations be rigid rotations.

Some features:

1) Panels can be grouped.
   All panels in a group will move the same way and rotate the same way.  If rigid rotations are
    used, each panel is rotated about the center of the instrument, along with panel pixels rotating
    around the panel's center. The height and  widths of the panels in a group will
     all change by the same factor

2) The user can select which quantities to keep fixed during the optimization.

3) The results can be saved to an ISAW-like DetCal file or in an xml file that can be used with the LoadParameter algorithm.

4) Results from a previous optimization can be applied before another optimization is done.
   The Levenberg-Marquardt optimization algorithm is used. Later iterations may have too small of changes for the parameters to
   get to another optimum value.  Restarting allows for the consideration of parameter values further away and also can change
   constraints for the parameter values. This is also useful when fine tuning parameters that do not influence the errors as
   much as other parameters.

5) There are several output tables indicating the results of the fit
   A) ResultWorkspace contains the results from fitting.
     -t0 is in microseconds
     -L0 is in meters
     -*Xoffset,*Yoffset,and *Zoffset are in meters
     -*Xrot,*Yrot, and *Zrot are in degrees. Note that Zrot is done first, then Yrot , the Xrot.

   B)QErrorWorkspace contains the Error in Q values for each peak, along with other associated information about the peak

   C)CovarianceInfo contains the "correlations"(*100) between each of the parameters

 6) Maximum changes in the quantities that are altered during optimization are now settable.

*WIKI*/


/*
* SCDCalibratePanels.cpp
*
*  Created on: Mar 12, 2012
*      Author: ruth
*/
//TODO
//  Rotate should also rotate around centroid of Group




#include "MantidCrystal/SCDCalibratePanels.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/Quat.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ListValidator.h"

#include "MantidAPI/Workspace.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ITableWorkspace.h"
#include <boost/algorithm/string/trim.hpp>
#include <iostream>
#include <fstream>
#include <math.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include "MantidKernel/Property.h"
#include "MantidAPI/IFunction.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"

using namespace Mantid::DataObjects;
using namespace  Mantid::API;
using namespace std;
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

      // g_log.setLevel(7);
    }


    SCDCalibratePanels::~SCDCalibratePanels()
    {

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
      if (Z[ 1 ] != 0 || Z[ 2 ] != 0)
      {
        double tx = atan2(-Z[ 1 ], Z[ 2 ]);
        double tz = atan2(-Y[ 0 ], X[ 0 ]);
        double cosy = Z[ 2 ] / cos(tx);
        double ty = atan2(Z[ 0 ], cosy);
        Rotx = (tx * 180 / M_PI);
        Roty = (ty * 180 / M_PI);
        Rotz = (tz * 180 / M_PI);
      }else //roty = 90 0r 270 def
      {
        double k= 1;
        if( Z[ 0 ] < 0 )
          k=-1;
        double roty=k*90;
        double rotx=0;
        double rotz= atan2(X[ 2 ],Y[ 2 ]);

        Rotx = (rotx * 180 / M_PI);
        Roty = (roty * 180 / M_PI);
        Rotz = (rotz * 180 / M_PI);
      }
    }



    DataObjects::Workspace2D_sptr SCDCalibratePanels::calcWorkspace( DataObjects::PeaksWorkspace_sptr & pwks,
      vector< string>& bankNames,
      double tolerance, vector< int >&bounds)
    {
      int N=0;
      Mantid::MantidVecPtr pX;
      if( tolerance < 0)
        tolerance = .5;
      tolerance = min< double >(.5, tolerance);

      Mantid::MantidVec& xRef = pX.access();
      Mantid::MantidVecPtr yvals;
      Mantid::MantidVecPtr errs;
      Mantid::MantidVec &yvalB = yvals.access();
      Mantid::MantidVec &errB = errs.access();
      bounds.clear();
      bounds.push_back(0);

      for (size_t k = 0; k < bankNames.size(); ++k)
      {
        for (size_t j = 0; j < (size_t)pwks->getNumberPeaks(); ++j)
        {
          API::IPeak& peak = pwks->getPeak((int) j);

          if (peak.getBankName().compare(bankNames[ k ]) == 0)
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

      if( N < 4)//If not well indexed
        return boost::shared_ptr<DataObjects::Workspace2D>(new DataObjects::Workspace2D);

      std::cout<<"Number indexed peaks used="<<N<<std::endl;
      mwkspc= API::WorkspaceFactory::Instance().create("Workspace2D",(size_t)3,3*N,3*N);

      mwkspc->setX(0, pX);
      mwkspc->setX(1, pX);
      mwkspc->setX(2, pX);
      mwkspc->setData(0, yvals,errs);
      mwkspc->setData(1, pX);
      mwkspc->setData(2, yvals);

      return boost::dynamic_pointer_cast< DataObjects::Workspace2D >(mwkspc);
    }



    void  SCDCalibratePanels::CalculateGroups(set< string >& AllBankNames, string Grouping, string bankPrefix,
      string bankingCode, vector<vector< string > > &Groups)
    {
      Groups.clear();

      if( Grouping == "OnePanelPerGroup" )
      {
        for( set< string >::iterator it = AllBankNames.begin(); it != AllBankNames.end(); ++it )
        {
          string bankName = (*it);
          vector< string > vbankName;
          vbankName.push_back( bankName );
          Groups.push_back( vbankName );
        }

      }else if( Grouping == "AllPanelsInOneGroup" )
      {  vector< string > vbankName;
      for( set< string >::iterator it = AllBankNames.begin(); it != AllBankNames.end(); ++it )
      {
        string bankName = (*it);

        vbankName.push_back( bankName);
      }

      Groups.push_back(vbankName);
      }else if( Grouping == "SpecifyGroups" )
      {//TODO catch lexical cast throws , tell where error is
        boost::trim(bankingCode);

        vector< string > GroupA;
        boost::split(GroupA, bankingCode, boost::is_any_of("]"));
        set< string > usedInts;

        for( size_t Gr = 0;  Gr < GroupA.size(); ++Gr )
        {
          string S = GroupA[ Gr ];

          boost::trim(S);

          if( S.empty()) break;
          if (S[0]==',')
            S.erase(0,1);
          boost::trim(S);
          if( S[0]=='[')
            S.erase(0, 1);
          boost::trim(S);

          vector< string >GroupB;
          boost::split( GroupB, S, boost::is_any_of(","));

          vector< string > Group0;
          for( size_t panelRange = 0; panelRange < GroupB.size(); ++panelRange )
          {
            string rangeOfBanks = GroupB[ panelRange ];

            boost::trim(rangeOfBanks);

            vector< string > StrtStopStep;
            boost::split( StrtStopStep, rangeOfBanks, boost::is_any_of(":") );

            if( StrtStopStep.size() > 3 )
            {
              g_log.error("Improper use of : in " + rangeOfBanks);
              throw invalid_argument("Improper use of : in " + rangeOfBanks);
            }
            int start, stop ,step;
            step = 1;

            if( StrtStopStep.size() == 3)
            {
              boost::trim(StrtStopStep[2]);
              step = boost::lexical_cast<int>(StrtStopStep[ 2 ]);

              if( step <=0 ) step = 0;
            }
            start = -1;
            if( !StrtStopStep.empty())
            {
              boost::trim(StrtStopStep[0]);
              start = boost::lexical_cast<int>(StrtStopStep[ 0 ].c_str());

            }
            if( start <= 0)
            {
              g_log.error( "Improper use of : in " +  rangeOfBanks );
              throw invalid_argument("Improper use of : in " + rangeOfBanks );
            }
            stop = start;

            if( StrtStopStep.size() >= 2 )
            {
              boost::trim(StrtStopStep[1]);
              stop = boost::lexical_cast<int>(StrtStopStep[ 1 ].c_str());

              if( stop <=0 ) stop = start;
            }

            for (long ind = start; ind <= stop; ind += step)
            {
              ostringstream oss (ostringstream::out);
              oss<<bankPrefix<<ind;

              string bankName = oss.str();

              string postName = bankName.substr(bankPrefix.length());


              if (AllBankNames.find(string(bankName) )!= AllBankNames.end())
                if (usedInts.find(postName) == usedInts.end())
                {
                  Group0.push_back( bankName );
                  usedInts.insert( postName );
                }
            }
          }
          if( !Group0.empty())
            Groups.push_back( Group0 );

        }
      }else
      {
        g_log.error("No mode " + Grouping + " defined yet");
        throw invalid_argument("No mode " + Grouping + " defined yet");
      }

    }



    boost::shared_ptr<const Instrument> SCDCalibratePanels::GetNewCalibInstrument(
      boost::shared_ptr<const Instrument>   instrument,
      string preprocessCommand,
      string preprocessFilename,
      double &timeOffset,
      double &L0,
      vector< string >  & AllBankNames)
    {
      if( preprocessCommand == "A)No PreProcessing")
        return instrument;

      bool xml = false;
      if( preprocessCommand == "C)Apply a LoadParameter.xml type file")
        xml = true;



      boost::shared_ptr<const ParameterMap> pmap0 = instrument->getParameterMap();
      boost::shared_ptr<ParameterMap> pmap1( new ParameterMap());



      for( vector< string >::iterator vit = AllBankNames.begin();
        vit != AllBankNames.end(); ++vit )
      {
        string bankName = (*vit);
        updateBankParams( instrument->getComponentByName(bankName), pmap1, pmap0);
      }

      //---------------------update params for moderator.------------------------------
     // updateSourceParams(instrument->getSource(), pmap1, pmap0);

      boost::shared_ptr<const Instrument> newInstr(new Instrument(instrument->baseInstrument(), pmap1));

      double L1, norm = 1.0;
      V3D beamline, sampPos;
      instrument->getInstrumentParameters(L1, beamline, norm, sampPos);
      FixUpSourceParameterMap(newInstr, L0, sampPos, pmap0);




      if( xml)
      {
        vector<int> detIDs = instrument->getDetectorIDs();
        MatrixWorkspace_sptr wsM = WorkspaceFactory::Instance().create("Workspace2D", detIDs.size(),
            (size_t) 100, (size_t) 100);

        Workspace2D_sptr ws = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(wsM);
        ws->setInstrument(newInstr);
        ws->populateInstrumentParameters();
        //ws->setName("rrrr");

        boost::shared_ptr<Algorithm> loadParFile = createChildAlgorithm("LoadParameterFile");
        loadParFile->initialize();
        loadParFile->setProperty("Workspace", ws);
        loadParFile->setProperty("Filename", preprocessFilename);
        loadParFile->executeAsChildAlg();

        boost::shared_ptr<const Instrument> newInstrument = ws->getInstrument();
        newInstrument->getInstrumentParameters(L0, beamline, norm, sampPos);
        return newInstrument;

      }else
      { set<string>bankNames;
        LoadISawDetCal(newInstr,bankNames,timeOffset,preprocessFilename,
                  "bank");
        return newInstr;
      }
    }


    void SCDCalibratePanels::CalcInitParams(  RectangularDetector_const_sptr bank_rect,
      Instrument_const_sptr instrument,
      Instrument_const_sptr  PreCalibinstrument,
      double & detWidthScale0,double &detHeightScale0,
      double &Xoffset0,double &Yoffset0,double &Zoffset0,
      double &Xrot0,double &Yrot0,double &Zrot0)
    {
      string bankName = bank_rect->getName();
      RectangularDetector_const_sptr newBank =
        boost::dynamic_pointer_cast<const RectangularDetector>
        ( PreCalibinstrument->getComponentByName( bankName) );

      if( !newBank)
      {
        detWidthScale0 = 1;
        detHeightScale0 = 1;
        Xoffset0 = 0;
        Yoffset0 = 0;
        Zoffset0 = 0;
        Xrot0 = 0;
        Yrot0 = 0;
        Zrot0 = 0;
        g_log.notice() << "Improper PreCalibInstrument for " << bankName << endl;
        return;
      }

      boost::shared_ptr<Geometry::ParameterMap> pmap = instrument->getParameterMap();
      boost::shared_ptr<Geometry::ParameterMap> pmapPre = PreCalibinstrument->getParameterMap();

      vector< V3D > RelPosI = pmap->getV3D(bankName,"pos");
      vector< V3D > RelPosPre = pmapPre->getV3D(bankName,"pos");

      V3D posI,
        posPre;

      if(!RelPosI.empty())
        posI = RelPosI[ 0 ];
      else
        posI = bank_rect->getRelativePos();

      if(!RelPosPre.empty())
        posPre = RelPosPre[ 0 ];
      else
        posPre = newBank->getRelativePos();

      V3D change = posPre -posI;

      Xoffset0 = change.X();
      Yoffset0 = change.Y();
      Zoffset0 = change.Z();

      double scalexI = 1;
      double scalexPre = 1;
      double scaleyI = 1;
      double scaleyPre = 1;

      vector< double > ScalexI = pmap->getDouble( bankName, "scalex");
      vector< double > ScalexPre = pmapPre->getDouble( bankName, "scalex");
      vector< double > ScaleyI = pmap->getDouble( bankName, "scaley");
      vector< double > ScaleyPre = pmapPre->getDouble( bankName, "scaley");

      if( !ScalexI.empty())
        scalexI = ScalexI[ 0 ];

      if( !ScaleyI.empty())
        scaleyI = ScaleyI[ 0 ];

      if(!ScalexPre.empty())
        scalexPre = ScalexPre[ 0 ];

      if( !ScaleyPre.empty())
        scaleyPre = ScaleyPre[ 0 ];

      //scaling

      detWidthScale0 = scalexPre/scalexI;
      detHeightScale0 = scaleyPre/scaleyI;

      Quat rotI = bank_rect->getRelativeRot();
      Quat rotPre = newBank->getRelativeRot();

      rotI.inverse();
      Quat ChgRot = rotPre*rotI;

      Quat2RotxRotyRotz(ChgRot,Xrot0,Yrot0,Zrot0);

    }



    void  SCDCalibratePanels::exec ()
    {
      PeaksWorkspace_sptr peaksWs = getProperty("PeakWorkspace");

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

      //----------------- Set Up Bank Name Vectors -------------------------
      set< string > AllBankNames;
      for( int i = 0; i < peaksWs->getNumberPeaks(); ++i)
        AllBankNames.insert( peaksWs->getPeak(i).getBankName());

      vector<vector< string > >Groups;
      CalculateGroups( AllBankNames, Grouping, bankPrefix, bankingCode, Groups );

      vector< string >banksVec;
      vector<vector< string > >::iterator it;
      for(  it = Groups.begin(); it != Groups.end(); ++it )
      {

        for( vector< string >::iterator itt = (*it).begin(); itt!=(*it).end(); ++itt )
        {
          banksVec.push_back( (*itt) );
        }
      }

      //------------------ Set Up Workspace for IFitFunction Fit---------------
      vector< int >bounds;
      Workspace2D_sptr   ws = calcWorkspace( peaksWs, banksVec,tolerance, bounds );
      if( ws->getNumberHistograms() < 2)
      {
        g_log.error(" Not enough data to fit parameters ");
        throw length_error( " Not enough data to fit parameters " );
      }


      //----------- Initialize peaksWorkspace, initial parameter values etc.---------

      boost::shared_ptr<const Instrument> instrument = peaksWs->getPeak(0).getInstrument();
      double T0 = 0;
      if((string) getProperty("PreProcessInstrument") == "C)Apply a LoadParameter.xml type file")
        T0=  getProperty("InitialTimeOffset");//!*****

      double L0 = peaksWs->getPeak(0).getL1();
      boost::shared_ptr<const Instrument> PreCalibinstrument =
        GetNewCalibInstrument(instrument,
        (string) getProperty("PreProcessInstrument"),
        (string) getProperty("PreProcFilename"),
        T0, L0, banksVec);
      g_log.debug()<<"Initial L0,T0="<<L0<<","<<T0<<endl;
      V3D samplePos = peaksWs->getPeak( 0 ).getInstrument()->getSample()->getPos();

      string PeakWSName = getPropertyValue( "PeakWorkspace");
      if(PeakWSName.length()<1)
      {
        PeakWSName = "xxx";
        AnalysisDataService::Instance().addOrReplace("xxx",peaksWs );
      }

      string FunctionArgument;
      string Constraints("");
      string TiesArgument;
      int i = -1;//position in ParamResults Array.
      int nbanksSoFar = 0;


      FrameworkManager::Instance();
      bool first = false;
      int NGroups = (int)Groups.size();
      double detWidthScale0,
        detHeightScale0,
        Xoffset0,
        Yoffset0,
        Zoffset0,
        Xrot0,
        Yrot0,
        Zrot0;


      //------------------- For each Group set up Function, --------------------------
      //---------------Ties, and Constraint Properties for Fit algorithm--------------------
      ostringstream oss (ostringstream::out);
      oss.precision(4);
      string BankNameString = "";
      for( vector<vector< string > >::iterator itv = Groups.begin(); itv !=Groups.end(); ++itv)
      {
        if( itv != Groups.begin())
          BankNameString +="!";
        for( vector< string >::iterator it1 = (*itv).begin(); it1 !=(*itv).end(); ++it1)
        {
          if( it1 !=(*itv).begin())
            BankNameString +="/";

          BankNameString +=(*it1);
        }
      }

      int RotGroups=0;
      if( getProperty("RotateCenters"))
        RotGroups=1;
      int SampOffsets=0;
      if( getProperty("AllowSampleShift"))
        SampOffsets=1;

      oss << "name=SCDPanelErrors, PeakWorkspaceName=\""<<PeakWSName<<"\",";
      oss << "a=" << fixed << a << "," << "b=" << fixed << b << "," << "c=" << fixed << c << "," << "alpha=" << fixed << alpha << "," << "beta=" << fixed << beta
        << "," << "gamma=" << fixed << gamma << ","<< "NGroups="<<NGroups<<",BankNames ="<<BankNameString<<","
        <<"startX=-1,endX=-1,RotateCenters="<<RotGroups<<",SampleOffsets="<<SampOffsets<<",";
      oss<<   "l0=" << fixed << L0 << "," << "t0=" << fixed << T0 ;

      ostringstream oss1 ( ostringstream::out);


      double maxXYOffset = getProperty("MaxPositionChange_meters");

      oss1.precision( 4);
      for( vector<vector< string > >::iterator itv = Groups.begin(); itv !=Groups.end(); ++itv)
      {
        i++;

        boost::shared_ptr<const RectangularDetector> bank_rect;
        string Gprefix ="f"+ boost::lexical_cast<string>(i)+"_";


        string name=(*itv)[0];
        boost::shared_ptr<const IComponent> bank_cmp = instrument->getComponentByName(name);
        bank_rect =
          boost::dynamic_pointer_cast<const RectangularDetector>( bank_cmp);

        if( !bank_rect)
        {
          g_log.error("No Rectangular detector bank " + banksVec[ 0 ] + " in instrument");
          throw invalid_argument("No Rectangular detector bank " + banksVec[ 0 ] + " in instrument");
        }




        // if( it1 == (*itv).begin())
        CalcInitParams( bank_rect, instrument,  PreCalibinstrument, detWidthScale0
          ,detHeightScale0, Xoffset0, Yoffset0, Zoffset0, Xrot0, Yrot0, Zrot0);


        // --- set Function property ----------------------

        oss << "," <<  Gprefix<<"detWidthScale=" << fixed << detWidthScale0 << ","
          << Gprefix<<"detHeightScale=" << fixed << detHeightScale0 << ","
          <<Gprefix<<"Xoffset=" << Xoffset0 << "," <<Gprefix<< "Yoffset=" << Yoffset0 << "," << Gprefix<<"Zoffset=";

        oss     << Zoffset0 << "," << Gprefix<<"Xrot=" << Xrot0 << "," << Gprefix<<"Yrot=" << Yrot0 << "," <<Gprefix<< "Zrot=" << Zrot0 ;

        int startX = bounds[ nbanksSoFar ];
        int endXp1 = bounds[ nbanksSoFar + (*itv).size() ];
        if( endXp1-startX < 12)
        {
          g_log.error() << "Bank Group " <<  BankNameString  << " does not have enough peaks for fitting" << endl;

          throw  runtime_error("Group " + BankNameString +" does not have enough peaks");
        }
        //   oss << "BankNames=" << "\"" << BankNameString << "\"" << ",startX=" << startX << ",endX=" << endXp1-1;
        nbanksSoFar = nbanksSoFar + (int)(*itv).size();

        //---------- set Ties argument ----------------------------------


        if( i == 0)
        {
          first = true;

        }



        if( !use_PanelWidth)
        {
          if( !first)
            oss1 << ",";

          first = false;

          oss1 <<Gprefix<<"detWidthScale=" << fixed << detWidthScale0;
        }



        if( !use_PanelHeight)
        {
          if( !first)
            oss1 << ",";

          first = false;

          oss1 <<  Gprefix<<"detHeightScale=" << fixed << detHeightScale0;
        }



        if( !use_PanelPosition)
        {
          if( !first)
            oss1 << ",";

          first = false;

          oss1 <<Gprefix<<"Xoffset=" << Xoffset0 << "," <<
            Gprefix<<"Yoffset=" <<Yoffset0 << "," <<
            Gprefix<<"Zoffset=" << Zoffset0;
        }



        if( ! use_PanelOrientation )
        {
          if( !first)
            oss1 << ",";

          first = false;

          oss1 << Gprefix<<"Xrot=" << Xrot0 << ","
            <<Gprefix<<"Yrot=" << Yrot0<< ","
            <<Gprefix<<"Zrot=" << Zrot0;
        }





        //--------------- set Constraints Property  -------------------------------

        ostringstream oss2 ( ostringstream::out);

        if( i == 0)
          oss2 <<  (.85*L0) << "<" <<  "l0<"  <<  (1.15*L0 ) << ",-5<" <<  "t0<5" ;

        double MaxRotOffset = getProperty("MaxRotationChange_degrees");
        oss2 <<  "," << (.85)*detWidthScale0 << "<" << Gprefix << "detWidthScale<" << (1.15)*detWidthScale0
          << "," << (.85)*detHeightScale0 << "<" << Gprefix << "detHeightScale<" << (1.15)*detHeightScale0
          <<","<< -maxXYOffset+Xoffset0 << "<" << Gprefix << "Xoffset<" << maxXYOffset+Xoffset0
          << "," << -maxXYOffset+Yoffset0 << "<" << Gprefix << "Yoffset<" << maxXYOffset+Yoffset0<<
          "," << -maxXYOffset+Zoffset0 << "<" << Gprefix << "Zoffset<" << maxXYOffset+Zoffset0<<","
          << -MaxRotOffset<<"<" << Gprefix << "Xrot<"<<MaxRotOffset<<",-"<<MaxRotOffset<<"<"
          << Gprefix << "Yrot<"<<MaxRotOffset<<",-"<<MaxRotOffset<< "<" << Gprefix << "Zrot<"<<MaxRotOffset
          ;


        Constraints += oss2.str();


      }//for vector< string > in Groups


//Constraints for sample offsets
     maxXYOffset = getProperty("MaxSamplePositionChange_meters");

     if( getProperty("AllowSampleShift"))
     {

        oss << ",SampleX=" << samplePos.X() << ",SampleY=" << samplePos.Y() << ",SampleZ="
            << samplePos.Z();

        ostringstream oss2(ostringstream::out);
        if (!Constraints.empty())
          oss2 << ",";

        oss2 << samplePos.X()-maxXYOffset << "<SampleX<" <<samplePos.X()+ maxXYOffset << "," <<
            samplePos.Y()-maxXYOffset << "<SampleY<"
            << samplePos.Y()+maxXYOffset << "," << samplePos.Z()-maxXYOffset << "<SampleZ<" << samplePos.Z()+maxXYOffset;

        Constraints += oss2.str();
      }

      if(!use_L0)
      {
        if( !first)
          oss1 << ",";

        first = false;
        oss1 <<"l0="<<fixed<<L0;

      }

      if(!use_timeOffset)
      {
        if( !first)
          oss1 << ",";

        //!! first = false;
        oss1 <<"t0="<<fixed<<T0;

      }
      FunctionArgument = oss.str();
      TiesArgument = oss1.str();
      //--------------------- Set up Fit Algorithm and Execute-------------------
      boost::shared_ptr< Algorithm > fit_alg = createChildAlgorithm( "Fit", .2, .9, true );

      if( ! fit_alg)
      {
        g_log.error( "Cannot find Fit algorithm");
        throw invalid_argument( "Cannot find Fit algorithm" );
      }
      g_log.debug()<<"Function="<<FunctionArgument<<endl;

      g_log.debug()<<"Constraints="<<Constraints<<endl;
      fit_alg->initialize();

      int Niterations =  getProperty( "NumIterations");

      fit_alg->setProperty( "Function",FunctionArgument);
      fit_alg->setProperty( "MaxIterations",Niterations );
      if( !TiesArgument.empty())
        fit_alg->setProperty( "Ties",TiesArgument);

      fit_alg->setProperty( "Constraints", Constraints);

      fit_alg->setProperty( "InputWorkspace", ws);

      fit_alg->setProperty( "CreateOutput",true);

      fit_alg->setProperty( "Output","out");

      fit_alg->executeAsChildAlg();

      g_log.debug()<<"Finished executing Fit algorithm"<<endl;

      string OutputStatus =fit_alg->getProperty("OutputStatus");
      g_log.notice() <<"Output Status="<<OutputStatus<<endl;

      declareProperty(
        new API::WorkspaceProperty<API::ITableWorkspace>
        ("OutputNormalisedCovarianceMatrix","",Kernel::Direction::Output),
        "The name of the TableWorkspace in which to store the final covariance matrix" );
      //string NormMatName=fit_alg->getPropertyValue("OutputNormalisedCovarianceMatrix");


      ITableWorkspace_sptr NormCov= fit_alg->getProperty("OutputNormalisedCovarianceMatrix");

      // setProperty("OutputNormalisedCovarianceMatrix", NormCov);
      AnalysisDataService::Instance().addOrReplace( string("CovarianceInfo"),NormCov);
      setPropertyValue("OutputNormalisedCovarianceMatrix",   string("CovarianceInfo"));
      //--------------------- Get and Process Results -----------------------
      double chisq = fit_alg->getProperty( "OutputChi2overDoF");
      setProperty("ChiSqOverDOF", chisq);
      ITableWorkspace_sptr RRes = fit_alg->getProperty( "OutputParameters");
      vector< double >params;
      vector< double >errs ;
      vector< string >names;
      double sigma= sqrt(chisq);

      if( chisq < 0 ||  chisq != chisq)
        sigma = -1;
      string fieldBaseNames = ";l0;t0;detWidthScale;detHeightScale;Xoffset;Yoffset;Zoffset;Xrot;Yrot;Zrot;";
      if( getProperty("AllowSampleShift"))
        fieldBaseNames +="SampleX;SampleY;SampleZ;";
      for( int prm = 0; prm < (int)RRes->rowCount(); ++prm )
      {
        string namee =RRes->getRef< string >( "Name", prm );
        size_t dotPos = namee.find('_');
        if (dotPos >= namee.size())
          dotPos = 0;
        else
          dotPos++;
        string Field = namee.substr(dotPos);
        size_t FieldNum= fieldBaseNames.find(";"+Field+";");
        if( FieldNum > fieldBaseNames.size())
          continue;
        if( dotPos !=0)
        {
          int col = atoi( namee.substr( 1,dotPos).c_str());
          if( col < 0 || col >=NGroups)
            continue;
        }
        names.push_back( namee );
        params.push_back( RRes->getRef< double >( "Value", prm ));
        double err =  RRes->getRef< double >( "Error", prm );
        errs.push_back( sigma * err );

      }

      //------------------- Report chi^2 value --------------------
      int nVars =8;// NGroups;

      if( !use_PanelWidth) nVars--;
      if( !use_PanelHeight)nVars--;
      if( !use_PanelPosition) nVars -=3;
      if( !use_PanelOrientation) nVars -=3;
      nVars *= NGroups ;
      nVars += 2;

      if( !use_L0)nVars--;
      if( !use_timeOffset)nVars--;

      // g_log.notice() << "      nVars=" <<nVars<< endl;
      int NDof = ( (int)ws->dataX( 0).size()- nVars);
      setProperty("DOF",NDof);
      g_log.notice() << "ChiSqoverDoF =" << chisq << " NDof =" << NDof << endl;

      map<string,double> result;

      for( size_t i = 0; i < min< size_t >( params.size(), names.size() ); ++i )
      {
        result[ names[ i ] ] = params[ i ];

      }




      //--------------------- Create Result Table Workspace-------------------
      int nn(0);
      if( getProperty("AllowSampleShift"))
        nn=3;
      TableWorkspace_sptr Result( new TableWorkspace( 1+2*(10 + nn)));

      Result->addColumn( "str","Field");

      for( int g = 0; g < NGroups; ++g )
      {

        string GroupName = string("Group") + boost::lexical_cast<string>(g);
        Result->addColumn( "double",GroupName);
      }

      //double sqrtChiSqoverDof = sqrt( chisq);
      vector<string>TableFieldNames;
      for( int p=0; p <(int) names.size(); ++p)
      {
        string fieldName = names[p];
        size_t dotPos = fieldName.find('_');
        if (dotPos >= fieldName.size())
          dotPos = 0;
        else
          dotPos++;
        string Field = fieldName.substr(dotPos);
        int it;
        for(  it=0;  it< (int)TableFieldNames.size(); it++)
        {
          if( TableFieldNames[it]==Field)
            break;
        }
        if( it >= (int)TableFieldNames.size())
          TableFieldNames.push_back(Field);
      }
      for( int p=0; p< (int)TableFieldNames.size(); p++)
        Result->cell< string >( p,0) =  TableFieldNames[p];
      Result->cell<string>(TableFieldNames.size(),0)="Errors";
      for( int p=0; p< (int)TableFieldNames.size(); p++)
        Result->cell<string>(TableFieldNames.size()+p+1,0)=TableFieldNames[p];


      for( int p = 0; p < (int)names.size(); ++p )
      {
        string fieldName = names[ p ];
        size_t dotPos = fieldName.find( '_');
        if( dotPos >= fieldName.size())
          dotPos = 0;
        else
          dotPos++;
        string Field = fieldName.substr( dotPos);
        int FieldNum=0;
        int it=0;
        bool foundIt = false;
        for( it=0;it < (int)TableFieldNames.size() ; it++)
        {
          if( TableFieldNames[it] == Field)
          {
            foundIt = true;
            break;
          }
          FieldNum++;
        }
        if(foundIt == false)
          continue;
        int col=1;
        if( dotPos !=0)
        {
          col = atoi( fieldName.substr( 1,dotPos).c_str())+1;
        }
        Result->cell<double>(FieldNum,col)=params[p];
        Result->cell<double>(FieldNum+10+nn+1,col)=errs[p];

        /*int FieldNum = (int)fieldBaseNames.find( ";" + Field + ";" ) / 15;
        int col = 0;
        if( FieldNum <= 0)
        col = -1;
        else if( dotPos > 0)
        col = atoi( fieldName.substr( 1,dotPos).c_str());
        FieldNum--;

        if( col == 0)
        {
        Result->cell< string >( FieldNum,0) = Field;
        Result->cell< string >( FieldNum + 10,0) = "Err:" + Field;
        }
        if( col >=0)
        {
        Result->cell< double >( FieldNum, col + 1) = params[ p ];
        Result->cell< double >( FieldNum+10,col + 1) = errs[ p ] * sqrtChiSqoverDof;
        }
        */
      }

      //    setProperty( "ResultWorkspace", Result);
      string ResultWorkspaceName= getPropertyValue( "ResultWorkspace");
      AnalysisDataService::Instance().addOrReplace(ResultWorkspaceName, Result);
      setPropertyValue( "ResultWorkspace", ResultWorkspaceName);
      Result->setComment(string("t0(microseconds),l0 & offsets(meters),rot(degrees"));


      //---------------- Create new instrument with ------------------------
      //--------------new parameters to SAVE to files---------------------

      boost::shared_ptr<ParameterMap> pmap( new ParameterMap());
      boost::shared_ptr<const ParameterMap> pmapOld = instrument->getParameterMap();
      boost::shared_ptr<const Instrument> NewInstrument( new Instrument( instrument->baseInstrument(), pmap));

      i = -1;
      // cout<<"Ere set new values into instrument"<<endl;
      for( vector<vector< string > >::iterator itv = Groups.begin(); itv != Groups.end(); ++itv )
      {
        i++;

        boost::shared_ptr<const RectangularDetector> bank_rect;
        double rotx,roty,rotz;

        string prefix = "f"+boost::lexical_cast<string>(i)+"_";


        rotx = result[ prefix + "Xrot" ];

        roty = result[ prefix + "Yrot" ];

        rotz = result[ prefix + "Zrot" ];

        Quat newRelRot = Quat( rotx,V3D( 1,0,0))*Quat( roty,V3D( 0,1,0))*Quat( rotz,V3D( 0,0,1));//*RelRot;

        FixUpBankParameterMap( (*itv),
          NewInstrument,
          V3D(result[ prefix + "Xoffset" ], result[ prefix + "Yoffset" ],result[ prefix + "Zoffset" ]),
          newRelRot,
          result[ prefix+"detWidthScale" ],
          result[ prefix+"detHeightScale" ],
          pmapOld, getProperty("RotateCenters"));

      }//For @ group

      V3D sampPos(NewInstrument->getSample()->getPos());//should be (0,0,0)???
      if( getProperty("AllowSampleShift"))
        sampPos= V3D( result["SampleX"],result["SampleY"],result["SampleZ"]);

      FixUpSourceParameterMap( NewInstrument, result["l0"],sampPos, pmapOld);

      //---------------------- Save new instrument to DetCal-------------
      //-----------------------or xml(for LoadParameterFile) files-----------
      string DetCalFileName = getProperty( "DetCalFilename");
      string XmlFileName = getProperty( "XmlFilename");
      if( !DetCalFileName.empty() )
      {

        SaveIsawDetCal( NewInstrument,AllBankNames, result[ "t0" ], DetCalFileName);
        g_log.notice()<<"Saved DetCal file in "<<DetCalFileName<< endl;

      }

      SaveXmlFile(  XmlFileName, Groups,NewInstrument);


      //----------------- Calculate & Create Qerror table------------------

      TableWorkspace_sptr QErrTable( new TableWorkspace(ws->dataX(0).size()/3));
      QErrTable->addColumn("int","Bank Number");
      QErrTable->addColumn("int","Peak Number");
      QErrTable->addColumn("int","Peak Row");
      QErrTable->addColumn("double","Error in Q");
      QErrTable->addColumn("int","Peak Column");
      QErrTable->addColumn("int","Run Number");
      QErrTable->addColumn("double","wl");
      QErrTable->addColumn("double","tof");
      QErrTable->addColumn("double","d-spacing");
      QErrTable->addColumn("double","L2");
      QErrTable->addColumn("double","Scat");
      QErrTable->addColumn("double","y");

      //--------------- Create Function argument for the FunctionHandler------------


      size_t nData = ws->dataX(0).size();
      vector<double> out(nData);
      vector<double> xVals = ws->dataX(0);

      CreateFxnGetValues( ws,NGroups, names,params,BankNameString, out.data(),xVals.data(),nData);


      string prevBankName ="";
      int BankNumDef = 200;
      int TableRow = 0;
      for (size_t q = 0; q < nData; q += 3)
      {
        int pk = (int) xVals[q];
        const API::IPeak & peak = peaksWs->getPeak(pk);

        string bankName =peak.getBankName();
        size_t pos = bankName.find_last_not_of("0123456789");
        int bankNum;
        if( pos < bankName.size())
          bankNum = boost::lexical_cast<int>(bankName.substr(pos+1));
        else if( bankName == prevBankName)
          bankNum = BankNumDef;
        else
        {
          prevBankName = bankName;
          BankNumDef ++;
          bankNum = BankNumDef;
        }

        QErrTable->cell<int> (TableRow, 0) = bankNum ;
        QErrTable->cell<int> (TableRow, 1) = pk;
        QErrTable->cell<int> (TableRow, 2) = peak.getRow();
        QErrTable->cell<int> (TableRow, 4) = peak.getCol();
        QErrTable->cell<double> (TableRow, 3) = sqrt(out[q] * out[q] + out[q + 1] * out[q + 1] + out[q
          + 2] * out[q + 2]);
        //chiSq += out[q] * out[q] + out[q + 1] * out[q + 1] + out[q + 2] * out[q + 2];

        QErrTable->cell<int>(TableRow,5) = peak.getRunNumber();
        QErrTable->cell<double>(TableRow,6) = peak.getWavelength();
        QErrTable->cell<double>(TableRow,7) = peak.getTOF();
        QErrTable->cell<double>(TableRow,8) = peak.getDSpacing();

        QErrTable->cell<double>(TableRow,9) = peak.getL2();
        QErrTable->cell<double>(TableRow,10) = peak.getScattering();
        QErrTable->cell<double>(TableRow,11) = peak.getDetPos().Y();

        TableRow++;
      }

      //setProperty("QErrorWorkspace", QErrTable);
      string QErrorWorkspaceName = getPropertyValue("QErrorWorkspace");
      QErrTable->setComment(string("Errors in Q for each Peak"));
      AnalysisDataService::Instance().addOrReplace(QErrorWorkspaceName, QErrTable);
      setPropertyValue("QErrorWorkspace", QErrorWorkspaceName);

    }
// New instrumemt is OldInstrument.base with a cloned parameter map on the
// panels, sample etc.
    void  SCDCalibratePanels::LoadISawDetCal(
           boost::shared_ptr<const Instrument> &instrument,
           set<string> &AllBankName,double &T0,string filename,
           string bankPrefixName)
    {
      /* if( AllBankName.empty())
         {
            g_log.debug()<<"There are NO banks(panels) to load" << endl;
            throw invalid_argument("There are NO banks(panels) to load");
         }
         */
       V3D beamline, samplePos;
       double beamlineLen,L0;
       instrument->getInstrumentParameters(L0,beamline, beamlineLen,samplePos);
       int  count, id, nrows, ncols;
       double width, height, depth, detd, x, y,
              z, base_x, base_y, base_z, up_x, up_y, up_z;

       ifstream input(filename.c_str(), ios_base::in);
       string line;

       boost::shared_ptr<Mantid::Geometry::ParameterMap> pmap= instrument->getParameterMap();
       while(getline(input, line))
          {
            if(line[0] == '7')
            {
              double mL1;
              stringstream(line) >> count >> mL1 >> T0;
              double scaleL0= .01*mL1/beamlineLen;
              const IObjComponent_const_sptr source=instrument->getSource();
              V3D NewSourcePos= samplePos-beamline*scaleL0*2.0;

              V3D RelSourcePos = source->getRelativePos()+NewSourcePos-source->getPos();
              pmap->addPositionCoordinate(source.get(),"x",RelSourcePos.X());
              pmap->addPositionCoordinate(source.get(),"y",RelSourcePos.Y());
              pmap->addPositionCoordinate(source.get(),"z",RelSourcePos.Z());
            }

            if(line[0] != '5') continue;
            stringstream(line) >> count >> id >> nrows >> ncols >> width >> height >> depth >> detd
                                         >> x >> y >> z >> base_x >> base_y >> base_z >> up_x >> up_y >> up_z;

            string bankName= bankPrefixName+boost::lexical_cast<string>(id);

            if( !AllBankName.empty() && AllBankName.find(bankName) == AllBankName.end())
              continue;
            boost::shared_ptr<const RectangularDetector> det =
                boost::dynamic_pointer_cast<const RectangularDetector>(instrument->getComponentByName(bankName,3));
            if( !det)
              continue;

            //Adjust pmap to the new scaling
            double scalex=1.0;//previous scale factor on this detector
            double scaley=1.0;
            if( pmap->contains(det.get(),"scalex"))
              scalex=pmap->getDouble( det->getName(),"scalex")[0];
            if( pmap->contains(det.get(),"scaley"))
              scaley=pmap->getDouble( det->getName(),"scaley")[0];
            double ScaleX= scalex*0.01*width/det->xsize();
            double ScaleY = scaley*0.01*height/det->ysize();
            pmap->addDouble( det.get(),"scalex",ScaleX);
            pmap->addDouble( det.get(),"scaley",ScaleY);


            //Adjust pmap to the new center position. Note:in pmap the pos values
            //                                          are rel positions to parent
            x *= 0.01;
            y *= 0.01;
            z *= 0.01;
            V3D pos= det->getPos();
            V3D RelPos= V3D(x,y,z)-pos;
            if( pmap->contains(det.get(),"pos"))
              RelPos += pmap->getV3D( det->getName(),"pos")[0];
            pmap->addPositionCoordinate(det.get(),"x",RelPos.X());
            pmap->addPositionCoordinate(det.get(),"y",RelPos.Y());
            pmap->addPositionCoordinate(det.get(),"z",RelPos.Z());

            //Adjust pmap to the orientation of the panel
            V3D rX = V3D(base_x, base_y, base_z);
                    rX.normalize();
                    V3D rY = V3D(up_x, up_y, up_z);
                    rY.normalize();
                    //V3D rZ=rX.cross_prod(rY);

                    //These are the original axes
                    V3D oX = V3D(1.,0.,0.);
                    V3D oY = V3D(0.,1.,0.);
                    V3D oZ = V3D(0.,0.,1.);

                    //Axis that rotates X
                    V3D ax1 = oX.cross_prod(rX);
                    //Rotation angle from oX to rX
                    double angle1 = oX.angle(rX);
                    angle1 *=180.0/M_PI;
                    //Create the first quaternion
                    Quat Q1(angle1, ax1);

                    //Now we rotate the original Y using Q1
                    V3D roY = oY;
                    Q1.rotate(roY);
                    //Find the axis that rotates oYr onto rY
                    V3D ax2 = roY.cross_prod(rY);
                    double angle2 = roY.angle(rY);
                    angle2 *=180.0/M_PI;
                    Quat Q2(angle2, ax2);

                    //Final = those two rotations in succession; Q1 is done first.
                    Quat Rot = Q2 * Q1;

                    // Then find the corresponding relative position
                   //boost::shared_ptr<const IComponent> comp = instrument->getComponentByName(detname);
                    boost::shared_ptr<const IComponent> parent = det->getParent();
                    if (parent)
                    {
                        Quat rot0 = parent->getRelativeRot();
                        rot0.inverse();
                        Rot = Rot * rot0;
                    }
                    boost::shared_ptr<const IComponent>grandparent = parent->getParent();
                    if (grandparent)//Why this is not correct but most Rectangular detectors have no grandparent.
                    {
                        Quat rot0 = grandparent->getRelativeRot();
                        rot0.inverse();
                        Rot = Rot * rot0;
                    }


                    // Set or overwrite "rot" instrument parameter.
                    pmap->addQuat(det.get(),"rot",Rot);



       }//While reading thru file
    }
    /**
     * Really this is the operator SaveIsawDetCal but only the results of the given
     * banks are saved.  L0 and T0 are also saved.
     *
     * @param  NewInstrument  -The instrument with the correct panel geometries
     *                         and initial path length
     *
     * @param AllBankName  -the set of the NewInstrument names of the banks(panels)
     * @param T0           -The time offset
     * @param FileName     -The name of the DetCal file to save the results to
     */
    void SCDCalibratePanels::SaveIsawDetCal( boost::shared_ptr<const Instrument> &NewInstrument,
         set<string> &AllBankName,double T0,string FileName)
    {
      double beamline_norm,L0=0.0;
      V3D beamline,  samplePos;
      filebuf fb;
      fb.open(FileName.c_str(), ios::out);
      ostream os(&fb);
      os << "# NEW CALIBRATION FILE FORMAT (in NeXus/SNS coordinates):" << endl;
      os << "# Lengths are in centimeters." << endl;
      os << "# Base and up give directions of unit vectors for a local " << endl;
      os << "# x,y coordinate system on the face of the detector." << endl;
      os << "#" << endl;
      os << "#" << endl;
      os << "#" << DateAndTime::getCurrentTime().toISO8601String() << endl;
      os << "6         L1    T0_SHIFT" << endl;


      NewInstrument->getInstrumentParameters(L0, beamline, beamline_norm, samplePos);
      os << "7 " << setw(10);
      os << setprecision(4) << fixed << (L0 * 100);
      os << setw(12) << setprecision(3) << fixed;
      // Time offset of 0.00 for now
      os << setw(12) << setprecision(4) << T0 << endl;

      os
          << "4 DETNUM  NROWS  NCOLS   WIDTH   HEIGHT   DEPTH   DETD   CenterX   CenterY   CenterZ    BaseX    BaseY    BaseZ      UpX      UpY      UpZ"
          << endl;
      for (set<string>::iterator it = AllBankName.begin(); it != AllBankName.end(); ++it)
      {
        string bankName = *it;
        string::reverse_iterator rit = bankName.rbegin();
        string S;
        while (isdigit(*rit))
        {
          S = *rit +S;
          ++rit;
        }
        int bankNum = boost::lexical_cast<int>(S);

        boost::shared_ptr<const RectangularDetector> det = boost::dynamic_pointer_cast<
            const RectangularDetector>(NewInstrument->getComponentByName(bankName));
        if (!det)
          continue;
        V3D center = det->getPos();
        double detd = (center - NewInstrument->getSample()->getPos()).norm();
        // Base unit vector (along the horizontal, X axis)
        V3D base = det->getAtXY(det->xpixels() - 1, 0)->getPos() - det->getAtXY(0, 0)->getPos();
        base.normalize();
        // Up unit vector (along the vertical, Y axis)
        V3D up = det->getAtXY(0, det->ypixels() - 1)->getPos() - det->getAtXY(0, 0)->getPos();
        up.normalize();

        // Write the line
        os << "5 " << setw(6) << right << bankNum << " " << setw(6) << right
            << det->xpixels() << " " << setw(6) << right << det->ypixels() << " "
            << setw(7) << right << fixed << setprecision(4) << 100.0 * det->xsize()
            << " " << setw(7) << right << fixed << setprecision(4) << 100.0
            * det->ysize() << " " << "  0.2000 " << setw(6) << right << fixed
            << setprecision(2) << 100.0 * detd << " " << setw(9) << right << fixed
            << setprecision(4) << 100.0 * center.X() << " " << setw(9) << right
            << fixed << setprecision(4) << 100.0 * center.Y() << " " << setw(9)
            << right << fixed << setprecision(4) << 100.0 * center.Z() << " "
            << setw(8) << right << fixed << setprecision(5) << base.X() << " "
            << setw(8) << right << fixed << setprecision(5) << base.Y() << " "
            << setw(8) << right << fixed << setprecision(5) << base.Z() << " "
            << setw(8) << right << fixed << setprecision(5) << up.X() << " "
            << setw(8) << right << fixed << setprecision(5) << up.Y() << " "
            << setw(8) << right << fixed << setprecision(5) << up.Z() << " "
            << endl;

      }

      fb.close();
    }


    void SCDCalibratePanels::init()
    {
      declareProperty(new WorkspaceProperty<PeaksWorkspace> ("PeakWorkspace", "",
        Kernel::Direction::Input), "Workspace of Indexed Peaks");

      vector< string > choices;
      choices.push_back("OnePanelPerGroup");
      choices.push_back("AllPanelsInOneGroup");
      choices.push_back("SpecifyGroups");
      declareProperty(string("PanelGroups"), string("OnePanelPerGroup"), boost::make_shared<
        Kernel::StringListValidator>(choices), "Select grouping of Panels");

      declareProperty("PanelNamePrefix", "bank", "Prefix for the names of panels(followed by a number)");
      declareProperty("Grouping", "[ 1:20,22],[3,5,7]",
        "A bracketed([]) list of groupings( comma or :(for range) separated list of bank numbers");

      declareProperty("a", 0.0, "Lattice Parameter a");
      declareProperty("b", 0.0, "Lattice Parameter b");
      declareProperty("c", 0.0, "Lattice Parameter c");
      declareProperty("alpha", 0.0, "Lattice Parameter alpha in degrees");
      declareProperty("beta", 0.0, "Lattice Parameter beta in degrees");
      declareProperty("gamma", 0.0, "Lattice Parameter gamma in degrees");

      declareProperty("use_L0", true, "Fit the L0(source to sample) distance");
      declareProperty("use_timeOffset", true, "Fit the time offset value");
      declareProperty("use_PanelWidth", true, "Fit the Panel Width value");
      declareProperty("use_PanelHeight", true, "Fit the Panel Height");
      declareProperty("use_PanelPosition", true, "Fit the PanelPosition");
      declareProperty("use_PanelOrientation", true, "Fit the PanelOrientation");
      declareProperty("RotateCenters", false,"Rotate bank Centers with panel orientations");
      declareProperty("AllowSampleShift",false,"Allow and fit for a sample that is off center");


      declareProperty("tolerance", .12, "offset of hkl values from integer for GOOD Peaks");

      vector< string > exts;
      exts.push_back(".DetCal");
      exts.push_back(".Det_Cal");
      declareProperty(new FileProperty("DetCalFilename", "", FileProperty::OptionalSave, exts),
        "Path to an ISAW-style .detcal file to save.");

      vector< string > exts1;
      exts1.push_back(".xml");
      declareProperty(new FileProperty("XmlFilename", "", FileProperty::OptionalSave, exts1),
        "Path to an Mantid .xml description(for LoadParameterFile) file to save.");


      vector< string > preProcessOptions;
      preProcessOptions.push_back(string("A)No PreProcessing"));
      preProcessOptions.push_back("B)Apply a ISAW.DetCal File");
      preProcessOptions.push_back("C)Apply a LoadParameter.xml type file");

      declareProperty(string("PreProcessInstrument"), string("A)No PreProcessing"), boost::make_shared<
        Kernel::StringListValidator>(preProcessOptions), "Select PreProcessing info");
      // declareProperty("PreProcessInstrument", string("No PreProcessing"), boost::shared_ptr<
      //        Kernel::ValidatorAnyList<string> >(new Kernel::ValidatorAnyList<string>(preProcessOptions)), "Select PreProcessing info");

      vector< string > exts2;
      exts2.push_back(".DetCal");
      exts2.push_back(".xml");
      declareProperty(new FileProperty("PreProcFilename", "", FileProperty::OptionalLoad, exts2),
        "Path to file with preprocessing information");

      declareProperty("InitialTimeOffset", 0.0, "Initial time offset when using xml files");


      declareProperty(new WorkspaceProperty<TableWorkspace> ("ResultWorkspace", "ResultWorkspace",
        Kernel::Direction::Output), "Workspace of Results");

      declareProperty(new WorkspaceProperty<TableWorkspace> ("QErrorWorkspace", "QErrorWorkspace",
        Kernel::Direction::Output), "Workspace of Errors in Q");
      //------------------------------------ Tolerance settings-------------------------

      declareProperty( "NumIterations",60,"Number of iterations");
      declareProperty("MaxRotationChange_degrees",5.0,"Maximum Change in Rotations about x,y,or z in degrees(def=5)");
      declareProperty("MaxPositionChange_meters",.010,"Maximum Change in Panel positions in meters(def=.01)");
      declareProperty("MaxSamplePositionChange_meters",.005,"Maximum Change in Sample position in meters(def=.005)");

      setPropertyGroup("NumIterations", "Tolerance settings");
      setPropertyGroup("MaxRotationChange_degrees", "Tolerance settings");
      setPropertyGroup("MaxPositionChange_meters", "Tolerance settings");
      setPropertyGroup("MaxSamplePositionChange_meters", "Tolerance settings");


      declareProperty("ChiSqOverDOF",-1.0,"ChiSqOverDOF",Kernel::Direction::Output);
      declareProperty("DOF",-1,"Degrees of Freedom",Kernel::Direction::Output);
      setPropertySettings("PanelNamePrefix", new EnabledWhenProperty( "PanelGroups",
        Kernel::IS_EQUAL_TO, "SpecifyGroups"));

      setPropertySettings("Grouping", new EnabledWhenProperty( "PanelGroups", Kernel::IS_EQUAL_TO,
        "SpecifyGroups"));

      setPropertySettings("PreProcFilename", new EnabledWhenProperty( "PreProcessInstrument",
        Kernel::IS_NOT_EQUAL_TO, "A)No PreProcessing"));

      setPropertySettings("InitialTimeOffset", new EnabledWhenProperty("PreProcessInstrument",
        Kernel::IS_EQUAL_TO, "C)Apply a LoadParameter.xml type file"));

      setPropertySettings("MaxSamplePositionChange_meters",new EnabledWhenProperty("AllowSampleShift",
          Kernel::IS_EQUAL_TO, "1" ));

      setPropertySettings("MaxRotationChange_degrees",new EnabledWhenProperty("use_PanelOrientation",
          Kernel::IS_EQUAL_TO, "1" ));

    }


    void  SCDCalibratePanels::initDocs ()
    {
      this->setWikiSummary("Calibrates Panels for Rectangular Detectors only");
      this->setOptionalMessage("Panel parameters, sample position,L0 and T0 are optimized to minimize errors between theoretical and actual q values for the peaks");

    }

    void  SCDCalibratePanels::CreateFxnGetValues(Workspace2D_sptr const ws,
      int const NGroups,
      vector<string>const names,
      vector<double>const params,
      string const BankNameString,
      double *out,
      const double *xVals,
      const size_t nData)        const
    {
      boost::shared_ptr<IFunction1D> fit = boost::dynamic_pointer_cast<IFunction1D>(
        FunctionFactory::Instance().createFunction("SCDPanelErrors"));
      if( !fit)
        cout<<"Could not create fit function"<<endl;

      fit->setAttribute("a",IFunction::Attribute((double)getProperty("a")));
      fit->setAttribute("b",IFunction::Attribute((double)getProperty("b")));
      fit->setAttribute("c",IFunction::Attribute((double)getProperty("c")));
      fit->setAttribute("alpha",IFunction::Attribute((double)getProperty("alpha")));
      fit->setAttribute("beta",IFunction::Attribute((double)getProperty("beta")));
      fit->setAttribute("gamma",IFunction::Attribute((double)getProperty("gamma")));
      string PeakWSName = getPropertyValue("PeakWorkspace");
      if( PeakWSName.length()<1)
        PeakWSName="xxx";
      fit->setAttribute("PeakWorkspaceName",IFunction::Attribute(PeakWSName));
      fit->setAttribute("startX",IFunction::Attribute(-1));
      fit->setAttribute("endX",IFunction::Attribute(-1));
      fit->setAttribute("NGroups",IFunction::Attribute(NGroups));
      fit->setAttribute("BankNames", IFunction::Attribute(BankNameString));


      string fieldBase[8]={"detWidth","detHeight","Xoffset","Yoffset","Zoffset",
        "Xrot","Yrot","Zrot"};
      set<string>FieldB(fieldBase, fieldBase+8);

      for (int g = 0; g < NGroups; ++g)
      {
        //Now add parameter values
        ostringstream prefixStrm(ostringstream::out);
        prefixStrm << "f" << g << "_";
        string prefix = prefixStrm.str();

        for (int nm = 0; nm < (int) names.size(); ++nm)
        {
          if (names[nm].compare(0, prefix.length(), prefix) == 0)
          {
            string prm = names[nm].substr(prefix.length());
            if (FieldB.find( prm ) != FieldB.end())
            {
              fit->setParameter(names[nm], params[nm]);

            }
          }
          else if (names[nm] == "l0" || names[nm] == "t0")
            fit->setParameter(names[nm], params[nm]);
        }
      }

      fit->setWorkspace(ws);

      //------Call SCDPanelErrors to get the q errors ------------------
      fit->function1D(out, xVals, nData);


    }


    void SCDCalibratePanels::updateBankParams( boost::shared_ptr<const Geometry::IComponent>  bank_const,
      boost::shared_ptr<Geometry::ParameterMap> pmap,
      boost::shared_ptr<const Geometry::ParameterMap>pmapSv)
    {
      vector< V3D > posv= pmapSv->getV3D( bank_const->getName(),"pos");

      if (!posv.empty())
      {
        V3D pos = posv[ 0 ];
        pmap->addDouble(bank_const.get(), "x", pos.X());
        pmap->addDouble(bank_const.get(), "y", pos.Y());
        pmap->addDouble(bank_const.get(), "z", pos.Z());
        pmap->addV3D(bank_const.get(), "pos", pos);

      }

      boost::shared_ptr< Parameter > rot = pmapSv->get(bank_const.get(),("rot"));
      if( rot)
      {
        pmap->addQuat(bank_const.get(), "rot",rot->value<Quat>());


      }

      vector< double > scalex = pmapSv->getDouble(bank_const->getName(),"scalex");
      vector< double > scaley = pmapSv->getDouble(bank_const->getName(),"scaley");
      if( !scalex.empty())
      {
        pmap->addDouble(bank_const.get(),"scalex", scalex[ 0 ]);

      }
      if( !scaley.empty())
      {
        pmap->addDouble(bank_const.get(),"scaley", scaley[ 0 ]);

      }

      boost::shared_ptr< const Geometry::IComponent > parent = bank_const->getParent();
      if( parent )
      {
        updateBankParams( parent, pmap, pmapSv);
      }

    }


    void SCDCalibratePanels::updateSourceParams(boost::shared_ptr<const Geometry::IObjComponent> bank_const,
      boost::shared_ptr<Geometry::ParameterMap> pmap, boost::shared_ptr<const Geometry::ParameterMap> pmapSv)
    {
      vector< V3D > posv = pmapSv->getV3D(bank_const->getName(), "pos");

      if (!posv.empty())
      {
        V3D pos = posv[ 0 ];
        pmap->addDouble(bank_const.get(), "x", pos.X());
        pmap->addDouble(bank_const.get(), "y", pos.Y());
        pmap->addDouble(bank_const.get(), "z", pos.Z());
        pmap->addV3D(bank_const.get(), "pos", pos);
      }

      boost::shared_ptr<Parameter> rot = pmapSv->get(bank_const.get(), "rot");
      if (rot)
        pmap->addQuat(bank_const.get(), "rot", rot->value<Quat>());
    }


    void SCDCalibratePanels::FixUpSourceParameterMap( boost::shared_ptr<const Instrument> NewInstrument,
      double const L0,V3D const newSampPos,
      boost::shared_ptr<const ParameterMap> const pmapOld)
    {
      boost::shared_ptr<ParameterMap> pmap = NewInstrument->getParameterMap();
      IObjComponent_const_sptr source = NewInstrument->getSource();
      updateSourceParams(source, pmap, pmapOld);

      IObjComponent_const_sptr sample = NewInstrument->getSample();
      V3D SamplePos = sample->getPos();
      if( SamplePos != newSampPos)
      {
        V3D newSampRelPos = newSampPos-SamplePos;
        pmap->addPositionCoordinate(sample.get(), string( "x" ), newSampRelPos.X());
        pmap->addPositionCoordinate(sample.get(), string( "y" ), newSampRelPos.Y());
        pmap->addPositionCoordinate(sample.get(), string( "z" ), newSampRelPos.Z());

      }
      V3D sourceRelPos = source->getRelativePos();
      V3D sourcePos = source->getPos();
      V3D parentSourcePos = sourcePos - sourceRelPos;
      V3D source2sampleDir = SamplePos - source->getPos();

      double scalee = L0 / source2sampleDir.norm();
      V3D newsourcePos = sample->getPos() - source2sampleDir * scalee;
      V3D newsourceRelPos = newsourcePos - parentSourcePos;

      pmap->addPositionCoordinate(source.get(), string( "x" ), newsourceRelPos.X());
      pmap->addPositionCoordinate(source.get(), string( "y" ), newsourceRelPos.Y());
      pmap->addPositionCoordinate(source.get(), string( "z" ), newsourceRelPos.Z());
    }



    void SCDCalibratePanels::FixUpBankParameterMap(vector<string> const bankNames,
      boost::shared_ptr<const Instrument> NewInstrument, V3D const pos, Quat const rot, double const DetWScale,
      double const DetHtScale, boost::shared_ptr<const ParameterMap> const pmapOld, bool RotCenters)
    {
      boost::shared_ptr<ParameterMap> pmap = NewInstrument->getParameterMap();

      for (vector<string>::const_iterator it1 = bankNames.begin(); it1 != bankNames.end(); ++it1)
      {

        const string bankName = (*it1);

        boost::shared_ptr<const IComponent> bank1 = NewInstrument->getComponentByName(bankName);
        boost::shared_ptr<const Geometry::RectangularDetector> bank = boost::dynamic_pointer_cast<const RectangularDetector>(bank1);//Component
        updateBankParams(bank, pmap, pmapOld);

        Quat RelRot = bank->getRelativeRot();
        Quat newRelRot = rot * RelRot;
        double rotx, roty, rotz;
        Quat2RotxRotyRotz(newRelRot, rotx, roty, rotz);

        pmap->addRotationParam(bank.get(), string("rotx"), rotx);
        pmap->addRotationParam(bank.get(), string("roty"), roty);
        pmap->addRotationParam(bank.get(), string("rotz"), rotz);
        pmap->addQuat(bank.get(), "rot", newRelRot);//Should not have had to do this???
        //---------Rotate center of bank ----------------------
        V3D Center =bank->getPos();
        V3D Center_orig( Center);
        if(RotCenters) rot.rotate(Center);

        V3D pos1 = bank->getRelativePos();

        pmap->addPositionCoordinate(bank.get(), string("x"), pos.X() + pos1.X()+
          Center.X()-Center_orig.X());
        pmap->addPositionCoordinate(bank.get(), string("y"), pos.Y() + pos1.Y()+
          Center.Y()-Center_orig.Y());
        pmap->addPositionCoordinate(bank.get(), string("z"), pos.Z() + pos1.Z()+
          Center.Z()-Center_orig.Z());

        Quat2RotxRotyRotz(rot, rotx, roty, rotz);
        //cout<<"A dCenter rot for"<<bankName<<Center-Center_orig<<
        //    "using rots="<< rotx<<","<<roty<<","<<rotz<<endl;
        // cout<<"       before/aft Centers"<<Center_orig<<Center<<endl;
        // cout<<"    Thru 1st prt param fix for "<<bankName<<". pos="<<bank->getPos()<<endl;
        //cout<<"A"<<posNR<<pos<<pos1<<endl;
        // pmap->addPositionCoordinate(bank.get(), string("x"), pos.X() + pos1.X());
        // pmap->addPositionCoordinate(bank.get(), string("y"), pos.Y() + pos1.Y());
        // pmap->addPositionCoordinate(bank.get(), string("z"), pos.Z() + pos1.Z());

        vector<double> oldScalex = pmap->getDouble(bank->getName(), string("scalex"));
        vector<double> oldScaley = pmap->getDouble(bank->getName(), string("scaley"));

        double scalex, scaley;
        if (!oldScalex.empty())
          scalex = oldScalex[0] * DetWScale;
        else
          scalex = DetWScale;

        if (!oldScaley.empty())
          scaley = oldScaley[0] * DetHtScale;
        else
          scaley = DetHtScale;

        pmap->addDouble(bank.get(), string("scalex"), scalex);
        pmap->addDouble(bank.get(), string("scaley"), scaley);
        //cout<<"Thru param fix for "<<bankName<<". pos="<<bank->getPos()<<endl;
      }//For @ bank
    }


    void SCDCalibratePanels::SaveXmlFile(string const FileName, vector<vector<string> > const Groups,
      Instrument_const_sptr const instrument)const
    {
      if (FileName.empty())
        return;

      ofstream oss3(FileName.c_str());
      oss3 << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" << endl;
      oss3 << " <parameter-file instrument=\"" << instrument->getName() << "\" date=\""
        << instrument->getValidFromDate().toISO8601String() << "\">" << endl;
      ParameterMap_sptr pmap = instrument->getParameterMap();

      for (vector<vector<string> >::const_iterator it = Groups.begin(); it != Groups.end(); ++it)
        for( vector<string>::const_iterator it1=(*it).begin(); it1 !=(*it).end(); ++it1)
        {
          string bankName = (*it1);

          oss3 << "<component-link name=\"" << bankName << "\">" << endl;

          boost::shared_ptr<const IComponent> bank = instrument->getComponentByName(bankName);

          Quat RelRot = bank->getRelativeRot();

          double rotx, roty, rotz;

          SCDCalibratePanels::Quat2RotxRotyRotz(RelRot, rotx, roty, rotz);

          oss3 << "  <parameter name =\"rotx\"><value val=\"" << rotx << "\" /> </parameter>" << endl;
          oss3 << "  <parameter name =\"roty\"><value val=\"" << roty << "\" /> </parameter>" << endl;
          oss3 << "  <parameter name =\"rotz\"><value val=\"" << rotz << "\" /> </parameter>" << endl;

          V3D pos1 = bank->getRelativePos();

          oss3 << "  <parameter name =\"x\"><value val=\"" << pos1.X() << "\" /> </parameter>"
            << endl;

          oss3 << "  <parameter name =\"y\"><value val=\"" << pos1.Y() << "\" /> </parameter>"
            << endl;

          oss3 << "  <parameter name =\"z\"><value val=\"" << pos1.Z() << "\" /> </parameter>"
            << endl;

          vector<double> oldScalex = pmap->getDouble(bank->getName(), string("scalex"));
          vector<double> oldScaley = pmap->getDouble(bank->getName(), string("scaley"));

          double scalex, scaley;
          if (!oldScalex.empty())
            scalex = oldScalex[0];
          else
            scalex = 1;

          if (!oldScaley.empty())
            scaley = oldScaley[0];
          else
            scaley = 1;

          oss3 << "  <parameter name =\"scalex\"><value val=\"" << scalex << "\" /> </parameter>"
            << endl;
          oss3 << "  <parameter name =\"scaley\"><value val=\"" << scaley << "\" /> </parameter>"
            << endl;
          oss3 << "</component-link>" << endl;
        }//For @ bank

        IObjComponent_const_sptr source = instrument->getSource();

        oss3 << "<component-link name=\"" << source->getName() << "\">" << endl;
        IObjComponent_const_sptr sample = instrument->getSample();
        V3D sourceRelPos = source->getRelativePos();

        oss3 << "  <parameter name =\"x\"><value val=\"" << sourceRelPos.X() << "\" /> </parameter>" << endl;

        oss3 << "  <parameter name =\"y\"><value val=\"" << sourceRelPos.Y() << "\" /> </parameter>" << endl;

        oss3 << "  <parameter name =\"z\"><value val=\"" << sourceRelPos.Z() << "\" /> </parameter>" << endl;

        oss3 << "</component-link>" << endl;

        oss3 << "</parameter-file>" << endl;

        oss3.flush();
        oss3.close();

    }





  }//namespace Crystal
}//namespace Mantid
