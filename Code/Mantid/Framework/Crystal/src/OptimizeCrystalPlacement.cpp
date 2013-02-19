/*WIKI*

 This algorithm basically optimizes sample positions and sample orientations( chi,phi, and omega) for an experiment.

 -If the crystal orientation matrix, UB, was created from one run, that run may not need to have its goniometer
 settings optimized.  There is a property to list the run numbers to NOT have their goniometer settings optimized.

 -The crystal orientation matrix, UB, from the PeaksWorkspace should index all the runs "very well". Otherwise iterations that build a UB with corrected sample orientations slowly may be necessary.

 *WIKI*/
/*
 * OptimizeCrystalPlacement.cpp
 *
 *  Created on: Jan 26, 2013
 *      Author: ruth
 */
#include "MantidCrystal/OptimizeCrystalPlacement.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include <boost/lexical_cast.hpp>

#include <math.h>
#include "MantidKernel/V3D.h"
#include <iostream>
#include <sstream>
#include <map>
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/IPeak.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidCrystal/PeakHKLErrors.h"
#include "MantidCrystal/SCDCalibratePanels.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using Mantid::Geometry::Instrument;
using Mantid::Geometry::ParameterMap;

namespace Mantid
{

  namespace Crystal
  {

    Kernel::Logger& OptimizeCrystalPlacement::g_log = Kernel::Logger::get("OptimizeCrystalPlacement");

    DECLARE_ALGORITHM( OptimizeCrystalPlacement )

    OptimizeCrystalPlacement::OptimizeCrystalPlacement() :
      Algorithm()
    {

    }
    OptimizeCrystalPlacement::~OptimizeCrystalPlacement()
    {

    }

    void OptimizeCrystalPlacement::initDocs()
    {
      this->setWikiSummary(
          "This algorithms indexes peaks after optimizing goniometer setting  and sample orientation." );
    }

    void OptimizeCrystalPlacement::init()
    {
      declareProperty(
          new WorkspaceProperty< PeaksWorkspace> ( "PeaksWorkspace", "", Direction::Input ),
          "Workspace of Peaks with UB loaded" );
      declareProperty( new  ArrayProperty<int>( std::string( "KeepGoniometerFixedfor" ), Direction::Input ),
          "List of run Numbers for which the goniometer settings will NOT be changed" );

      declareProperty( new WorkspaceProperty< PeaksWorkspace> ( "ModifiedPeaksWorkspace", "",
          Direction::Output ), "Output Workspace of Peaks with optimized sample Orientations" );

      declareProperty( new WorkspaceProperty<TableWorkspace> ( "FitInfoTable", "",
           Direction::Output ), "Workspace of Results" );

      declareProperty("ToleranceChiPhiOmega",5.0,"Max offset in degrees from current settings");

      declareProperty("MaxIntHKLOffsetPeaks2Use",.25,
          "Use only peaks whose h,k,and l offsets from and integer are below this level");

      declareProperty("MaxHKLPeaks2Use",-1.0,
          "If less than 0 all peaks are used, otherwise only peaks whose h,k, and l values are below the level are used");

      declareProperty("IncludeVaryingSampleOffsets", true,
          "If true sample offsets will be adjusted to give better fits, otherwise they will be fixed as zero");


      declareProperty( "Chi2overDoF", -1.0,"chi squared over dof", Direction::Output);
      declareProperty( "nPeaks", -1,"Number of Peaks Used", Direction::Output);
      declareProperty( "nParams", -1,"Number of Parameters fit", Direction::Output);







    }

    void OptimizeCrystalPlacement::exec()
    {
      PeaksWorkspace_sptr Peaks = getProperty( "PeaksWorkspace" );

      PeaksWorkspace_sptr OutPeaks = getProperty( "ModifiedPeaksWorkspace" );
      if( Peaks != OutPeaks)
      {
        boost::shared_ptr<PeaksWorkspace>X(Peaks->clone());
        OutPeaks= X;
      }

      std::vector<int> NOoptimizeRuns = getProperty( "KeepGoniometerFixedfor" );
      const DblMatrix X = Peaks->sample().getOrientedLattice().getUB();
      Matrix<double> UBinv( X );
      UBinv.Invert();

      std::vector<int> RunNumList;
      std::vector< V3D> ChiPhiOmega;
      Mantid::MantidVecPtr pX;
      Mantid::MantidVec& xRef = pX.access();
      Mantid::MantidVecPtr yvals;
      Mantid::MantidVecPtr errs;
      Mantid::MantidVec &yvalB = yvals.access();
      Mantid::MantidVec &errB = errs.access();

      int nPeaksUsed =0;
      double HKLintOffsetMax= getProperty("MaxIntHKLOffsetPeaks2Use");
      double HKLMax=getProperty("MaxHKLPeaks2Use");
      for ( int i = 0; i < Peaks->getNumberPeaks(); i++ )
      {
         IPeak& peak = Peaks->getPeak( i );
        int runNum = peak.getRunNumber();
        std::vector<int>::iterator it = RunNumList.begin();
        for ( ; it != RunNumList.end() && *it != runNum; ++it )
        {
        }

        if ( it == RunNumList.end() )
        {
          V3D hkl = UBinv * peak.getQSampleFrame();
          bool use = true;
          for( int k = 0; k < 3 && use; k++ )
          {
            double x = hkl[k]-floor(hkl[k]);
            if( x > .5 ) x -= 1;

            if( fabs(x) >=  HKLintOffsetMax)
              use = false;

            else if( HKLMax>0 && fabs(hkl[k])>HKLMax)
              use = false;

          }

          if( !use) continue;

          RunNumList.push_back( runNum );

          Geometry::Goniometer Gon( peak.getGoniometerMatrix() );
          std::vector<double> phichiOmega = Gon.getEulerAngles( "YZY" );
          ChiPhiOmega.push_back(  V3D( phichiOmega[1] , phichiOmega[2] , phichiOmega[0] ) );
        }

        nPeaksUsed++;
        xRef.push_back( ( double ) i );
        yvalB.push_back( 0.0 );
        errB.push_back( 1.0 );

      }

      MatrixWorkspace_sptr mwkspc;
      int N = Peaks->getNumberPeaks();
      mwkspc =  WorkspaceFactory::Instance().create( "Workspace2D" , (size_t) 1 , N , N );
      mwkspc->setX( 0 , pX );
      mwkspc->setData( 0 , yvals , errs );

      std::string FuncArg = "name=PeakHKLErrors,PeakWorkspaceName=" + getPropertyValue( "PeaksWorkspace" )
          + "";

      std::string OptRunNums;

      std::vector<std::string> ChRunNumList;
      std::string predChar="";
      for ( std::vector<int>::iterator it = RunNumList.begin(); it != RunNumList.end(); ++it )
      {
        int runNum = *it;

        std::vector<int>::iterator it1 = NOoptimizeRuns.begin();
        for ( ; it1 != NOoptimizeRuns.end() && *it1 != runNum; ++it1 )
        {
        }

        if ( it1 == NOoptimizeRuns.end() )
        {
          std::string runNumStr = boost::lexical_cast<std::string>( runNum );
          OptRunNums += predChar + runNumStr;
          predChar = "/";
          ChRunNumList.push_back( runNumStr );
        }
      }

      //if ( OptRunNums.size() > 0 )
      //  OptRunNums += "/";


      if ( OptRunNums.size() > 0 )
        FuncArg += ",OptRuns=" + OptRunNums;

      FuncArg += ",SampleXOffset=0,SampleYOffset=0,SampleZOffset=0";

      //------------- Add initial parameter values to FuncArg -----------

      std::ostringstream oss( std::ostringstream::out );
      oss.precision( 3 );
      std::ostringstream oss1( std::ostringstream::out );//constraints
      oss1.precision( 3 );

      std::string startConstraint = "";
      int nParams=3;
      double DegreeTol=getProperty("ToleranceChiPhiOmega");
      for ( size_t i = 0; i < RunNumList.size(); i++ )
      {
        int runNum = RunNumList[i];

        size_t k = 0;
        for ( ; k < NOoptimizeRuns.size(); k++ )
        {
          if ( NOoptimizeRuns[k] == runNum )
            break;
        }
        if ( k >= NOoptimizeRuns.size() )
        {
           V3D chiphiomega = ChiPhiOmega[i];
          oss << ",chi" << runNum << "=" << chiphiomega[0] << ",phi" << runNum << "=" << chiphiomega[1]
              << ",omega" << runNum << "=" << chiphiomega[2];

          oss1 << startConstraint << chiphiomega[0] - DegreeTol << "<chi" << runNum << "<" << chiphiomega[0] + DegreeTol;
          oss1 << "," << chiphiomega[1] - DegreeTol << "<phi" << runNum << "<" << chiphiomega[1] + DegreeTol;

          oss1 << "," << chiphiomega[2] - DegreeTol << "<omega" << runNum << "<" << chiphiomega[2] + DegreeTol;
          startConstraint = ",";
          nParams +=3 ;
        }

      }

      FuncArg += oss.str();
      std::string Constr = oss1.str();

      g_log.debug() << "Function argument=" << FuncArg<<std::endl;
      g_log.debug() << "Constraint argument=" << Constr<<std::endl;

      //--------------------- set up Fit algorithm call-----------------

      boost::shared_ptr<Algorithm> fit_alg = createChildAlgorithm( "Fit" , .1 , .93 , true );

      fit_alg->setProperty( "Function" , FuncArg );

      fit_alg->setProperty( "MaxIterations" , 60 );

      fit_alg->setProperty( "Constraints" , Constr );

      fit_alg->setProperty( "InputWorkspace" , mwkspc );

      fit_alg->setProperty( "CreateOutput" , true );

      if( (bool)getProperty("IncludeVaryingSampleOffsets"))
        fit_alg->setProperty("Ties","SampleXOffset=0,SampleYOffset=0,SampleXOffset=0");

      fit_alg->setProperty( "Output" , "out" );

      fit_alg->executeAsChildAlg();

      double chisq = fit_alg->getProperty( "OutputChi2overDoF" );
      std::cout<<"Fit finished. Status="<<(std::string)fit_alg->getProperty("OutputStatus")
                                        <<std::endl;

      setProperty("Chi2overDoF", chisq );

      setProperty( "nPeaks", nPeaksUsed) ;
      setProperty("nParams", nParams );
      g_log.debug() << "Chi2overDof=" << chisq <<"    # Peaks used="<< nPeaksUsed
                   << "# fitting parameters ="<< nParams << "   dof=" << (nPeaksUsed - nParams) <<std::endl;
      ITableWorkspace_sptr RRes = fit_alg->getProperty( "OutputParameters" );

      double sigma = sqrt( chisq );

      std::string OutputStatus = fit_alg->getProperty( "OutputStatus" );
      g_log.notice() << "Output Status=" << OutputStatus << std::endl;

      //------------------ Fix up Covariance output --------------------
      declareProperty( new  WorkspaceProperty< ITableWorkspace>(
          "OutputNormalisedCovarianceMatrix" , "" ,  Direction::Output ) ,
          "The name of the TableWorkspace in which to store the final covariance matrix" );

      //std::string NormMatName = fit_alg->getPropertyValue( "OutputNormalisedCovarianceMatrix" );

      ITableWorkspace_sptr NormCov = fit_alg->getProperty( "OutputNormalisedCovarianceMatrix" );

      // setProperty("OutputNormalisedCovarianceMatrix" , NormCov );
      AnalysisDataService::Instance().addOrReplace( std::string( "CovarianceInfo" ) , NormCov );
      setPropertyValue( "OutputNormalisedCovarianceMatrix" , std::string( "CovarianceInfo" ) );

      if ( chisq < 0 || chisq != chisq )
        sigma = -1;

      //------------- Fix up Result workspace values ----------------------------
      std::map< std::string,double> Results;
      for ( int prm = 0; prm < (int) RRes->rowCount(); ++prm )
      {
        std::string namee = RRes->getRef<std::string> ( "Name" , prm );

        std::string start= namee.substr(0,3);
        if( start !="chi" && start !="phi" && start !="ome" && start !="Sam")
          continue;

        double value = RRes->getRef<double>("Value",prm);
        Results[ namee] = value;
        double v = sigma * RRes->getRef<double> ( "Error" , prm );
        RRes->getRef<double> ( "Error" , prm ) = v;
        //change error , mult by sqrt

      }

    //-----------Fix up Resultant workspace return info -------------------
      std::string ResultWorkspaceName = getPropertyValue( "FitInfoTable" );
      AnalysisDataService::Instance().addOrReplace( ResultWorkspaceName , RRes );
      setPropertyValue( "FitInfoTable" , ResultWorkspaceName );

      //----------- update instrument -------------------------
      IPeak& peak =Peaks->getPeak(0);
      boost::shared_ptr<const Instrument>OldInstrument = peak.getInstrument();
      boost::shared_ptr<const ParameterMap>pmap_old = OldInstrument->getParameterMap();
      boost::shared_ptr<ParameterMap>pmap_new( new ParameterMap());

     PeakHKLErrors::cLone( pmap_new, OldInstrument , pmap_old );

      double L0 = peak.getL1();
      V3D oldSampPos = OldInstrument->getSample()->getPos();
      V3D newSampPos( oldSampPos.X()+Results["SampleXOffset"],
                      oldSampPos.Y()+Results["SampleYOffset"],
                        oldSampPos.Z()+Results["SampleZOffset"]);

      boost::shared_ptr<const Instrument>Inst =OldInstrument;

      if( OldInstrument->isParametrized())
          Inst= OldInstrument->baseInstrument();

      boost::shared_ptr<const Instrument>NewInstrument( new Instrument(Inst, pmap_new));

      SCDCalibratePanels::FixUpSourceParameterMap( NewInstrument,L0, newSampPos,pmap_old);

      for( int i=0;i<  OutPeaks->getNumberPeaks();i++)
        OutPeaks->setInstrument( NewInstrument);

      OutPeaks->setInstrument( NewInstrument );


      for( std::vector<std::string>::iterator it  =ChRunNumList.begin(); it !=ChRunNumList.end(); ++it)
      {
        std::string runNumStr = *it;
        double chi = Results[ "chi"+runNumStr ];
        double phi = Results[ "phi"+runNumStr ];
        double omega = Results[ "omega"+runNumStr ];
        Mantid::Geometry::Goniometer uniGonio;
        uniGonio.makeUniversalGoniometer();
        uniGonio.setRotationAngle( "phi", phi );
        uniGonio.setRotationAngle( "chi", chi );
        uniGonio.setRotationAngle( "omega", omega) ;
        Matrix<double> GonMatrix = uniGonio.getR();

        for( int i = 0 ; i < OutPeaks->getNumberPeaks() ; ++i)
          if( OutPeaks->getPeak(i).getRunNumber()== boost::lexical_cast<int>(runNumStr))
            {
               OutPeaks->getPeak( i ).setGoniometerMatrix( GonMatrix );

            }
      }


      std::string OutputPeaksName= getPropertyValue("ModifiedPeaksWorkspace");

       setPropertyValue( "ModifiedPeaksWorkspace", OutputPeaksName);
       setProperty( "ModifiedPeaksWorkspace", OutPeaks);

      //Note: this just runs IndexPeaks at the end. Could/Should be eliminated except the name
       // of this algorithm is index..Peaks, so maybe it should index the peaks.
      //--------------------------- index Output workspace -----------------------
       if ((bool) getProperty("IndexPeaks"))
      {
        boost::shared_ptr<Algorithm> index_alg = createChildAlgorithm("IndexPeaks", .1, .93, true);
        AnalysisDataService::Instance().addOrReplace(  OutputPeaksName, OutPeaks);
        //index_alg->setProperty("PeaksWorkspace", OutPeaks);
        index_alg->setPropertyValue("PeaksWorkspace", OutputPeaksName);
        index_alg->setProperty("Tolerance", (double) getProperty("Tolerance"));
        index_alg->setProperty("RoundHKLs", (bool) getProperty("RoundHKLs"));

        try
        {
          index_alg->executeAsChildAlg();
          index_alg->setPropertyValue( "PeaksWorkspace", OutputPeaksName );
          OutPeaks = index_alg->getProperty( "PeaksWorkspace" );
          AnalysisDataService::Instance().addOrReplace( OutputPeaksName, OutPeaks);;

          setPropertyValue( "ModifiedPeaksWorkspace", OutputPeaksName);
          setProperty( "ModifiedPeaksWorkspace", OutPeaks);
          setPropertyValue( "ModifiedPeaksWorkspace", OutputPeaksName);

        }catch(...)
        { g_log.debug() << "Could NOT index peaks" << std::endl;
          setProperty("NumIndexed", -1);
          setProperty("AverageError", -1.0);
          return;
        }

        setProperty("NumIndexed", (int) index_alg->getProperty("NumIndexed"));
        setProperty("AverageError", (double) index_alg->getProperty("AverageError"));

      }
    }//exec


  }//namespace Crystal

}//namespace Mantid
