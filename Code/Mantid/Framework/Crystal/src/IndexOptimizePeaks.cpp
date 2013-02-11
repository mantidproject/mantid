/*WIKI*

 This algorithm basically indexes peaks with the crystal orientation matrix stored in the peaks workspace.

 -The optimization is on the goniometer settings for the runs in the peaks workspace and also the sample
 orientation for the experiment.

 -If the crystal orientation matrix, UB, was created from one run, that run may not need to have its goniometer
 settings optimized.  There is a property to list the run numbers to not have their goniometer settings optimized.

 *WIKI*/
/*
 * IndexOptimizePeaks.cpp
 *
 *  Created on: Jan 26, 2013
 *      Author: ruth
 */

#include "MantidCrystal/IndexOptimizePeaks.h"
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
#include "MantidCrystal/PeakhklErrors.h"
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

    Kernel::Logger& IndexOptimizePeaks::g_log = Kernel::Logger::get("IndexOptimizePeaks");

    DECLARE_ALGORITHM( IndexOptimizePeaks )

    IndexOptimizePeaks::IndexOptimizePeaks() :
      Algorithm()
    {

    }
    IndexOptimizePeaks::~IndexOptimizePeaks()
    {

    }

    void IndexOptimizePeaks::initDocs()
    {
      this->setWikiSummary(
          "This algorithms indexes peaks after optimizing goniometer setting  and sample orientation." );
    }

    void IndexOptimizePeaks::init()
    {
      declareProperty(
          new WorkspaceProperty< PeaksWorkspace> ( "Peaks", "", Direction::Input ),
          "Workspace of Peaks with UB loaded" );
      declareProperty( new  ArrayProperty<int>( std::string( "NOoptimizeRuns" ), Direction::Input ),
          "List of run Numbers NOT to optimize goniometer settings" );

      declareProperty( new WorkspaceProperty< PeaksWorkspace> ( "OutputWorkspace", "",
          Direction::Output ), "Output Workspace of Peaks with optimized sample Orientations" );

      declareProperty( new WorkspaceProperty<TableWorkspace> ( "ResultWorkspace", "",
           Direction::Output ), "Workspace of Results" );

      declareProperty( "Chi2overDoF", -1.0,"chi squared over dof", Direction::Output);
      declareProperty( "nPeaks", -1,"Number of Peaks Used", Direction::Output);
      declareProperty( "nParams", -1,"Number of Parameters fit", Direction::Output);

      declareProperty( "IndexPeaks" , false ,"Index the resultant peaks");

      declareProperty( "Tolerance" , .12, "Tolerance for indexing peaks" );


      declareProperty( "RoundHKLs" ,true, "Round H, K and L values to integers " );

      declareProperty( "NumIndexed" , 0  , "Number of indexed peaks" , Direction::Output );

      declareProperty( "AverageError" , 0.0  , "Gets set with the average HKL indexing error" , Direction::Output );

      setPropertySettings ( "Tolerance" , new Kernel::EnabledWhenProperty( "IndexPeaks",
          Kernel::IS_EQUAL_TO , "1" ) );

      setPropertySettings ( "NumIndexed" , new Kernel::EnabledWhenProperty( "IndexPeaks",
          Kernel::IS_EQUAL_TO , "1" ) );

      setPropertySettings ( "AverageError" , new Kernel::EnabledWhenProperty( "IndexPeaks",
          Kernel::IS_EQUAL_TO , "1" ) );

    }

    void IndexOptimizePeaks::exec()
    {
      PeaksWorkspace_sptr Peaks = getProperty( "Peaks" );

      PeaksWorkspace_sptr OutPeaks = getProperty( "OutputWorkspace" );
      if( Peaks != OutPeaks)
      {
        boost::shared_ptr<PeaksWorkspace>X(Peaks->clone());
        OutPeaks= X;
      }

      std::vector<int> NOoptimizeRuns = getProperty( "NOoptimizeRuns" );
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
      for ( int i = 0; i < Peaks->getNumberPeaks(); i++ )
      {
         IPeak& peak = Peaks->getPeak( i );
        int runNum = peak.getRunNumber();
        std::vector<int>::iterator it = RunNumList.begin();
        for ( ; it != RunNumList.end() && *it != runNum; it++ )
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
            if( fabs(x) >= .25)
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

      std::string FuncArg = "name=PeakhklErrors,PeakWorkspaceName=" + getPropertyValue( "Peaks" )
          + "";

      std::string OptRunNums;

      std::vector<std::string> ChRunNumList;
      std::string predChar="";
      for ( std::vector<int>::iterator it = RunNumList.begin(); it != RunNumList.end(); it++ )
      {
        int runNum = *it;

        std::vector<int>::iterator it1 = NOoptimizeRuns.begin();
        for ( ; it1 != NOoptimizeRuns.end() && *it1 != runNum; it1++ )
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

          oss1 << startConstraint << chiphiomega[0] - 5 << "<chi" << runNum << "<" << chiphiomega[0] + 5;
          oss1 << "," << chiphiomega[1] - 5 << "<phi" << runNum << "<" << chiphiomega[1] + 5;

          oss1 << "," << chiphiomega[2] - 5 << "<omega" << runNum << "<" << chiphiomega[2] + 5;
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

      fit_alg->setProperty( "Output" , "out" );

      fit_alg->executeAsChildAlg();

      double chisq = fit_alg->getProperty( "OutputChi2overDoF" );

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

      std::string NormMatName = fit_alg->getPropertyValue( "OutputNormalisedCovarianceMatrix" );

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
      std::string ResultWorkspaceName = getPropertyValue( "ResultWorkspace" );
      AnalysisDataService::Instance().addOrReplace( ResultWorkspaceName , RRes );
      setPropertyValue( "ResultWorkspace" , ResultWorkspaceName );

      //----------- update instrument -------------------------
      IPeak& peak =Peaks->getPeak(0);
      boost::shared_ptr<const Instrument>OldInstrument = peak.getInstrument();
      boost::shared_ptr<const ParameterMap>pmap_old = OldInstrument->getParameterMap();
      boost::shared_ptr<ParameterMap>pmap_new( new ParameterMap());

     PeakhklErrors::cLone( pmap_new, OldInstrument , pmap_old );

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


      std::string OutputPeaksName= getPropertyValue("OutputWorkspace");

       setPropertyValue( "OutputWorkspace", OutputPeaksName);
       setProperty( "OutputWorkspace", OutPeaks);

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

          setPropertyValue( "OutputWorkspace", OutputPeaksName);
          setProperty( "OutputWorkspace", OutPeaks);
          setPropertyValue( "OutputWorkspace", OutputPeaksName);

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
