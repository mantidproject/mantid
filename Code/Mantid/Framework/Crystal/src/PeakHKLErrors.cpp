/*
 * PeakHKLErrors.cpp
 *
 *  Created on: Jan 26, 2013
 *      Author: ruth
 */
#include "MantidCrystal/PeakHKLErrors.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"
#include <string>
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/Matrix.h"
#include "MantidAPI/IPeak.h"
#include "MantidDataObjects/Peak.h"
#include "MantidCrystal/SCDPanelErrors.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidKernel/V3D.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidKernel/Quat.h"
#include "MantidAPI/IConstraint.h"

using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using Mantid::Geometry::CompAssembly;
using Mantid::Geometry::IObjComponent_const_sptr;

namespace Mantid
{

  namespace Crystal
  {

    Kernel::Logger& PeakHKLErrors::g_log = Kernel::Logger::get("PeakHKLErrors");

    DECLARE_FUNCTION( PeakHKLErrors )

    PeakHKLErrors::PeakHKLErrors():ParamFunction(), IFunction1D()
    {
      OptRuns = "";
      PeakWorkspaceName = "";
      initMode=0;
    }


    PeakHKLErrors:: ~PeakHKLErrors()
    {

    }

    void PeakHKLErrors::init()
    {
      declareParameter( "SampleXOffset", 0.0, "Sample x offset" );
      declareParameter( "SampleYOffset", 0.0, "Sample y offset" );
      declareParameter( "SampleZOffset", 0.0, "Sample z offset" );
      initMode =1;
      if( OptRuns == "")
        return;

      initMode = 2;
      setUpOptRuns();

    }
/**
 * Declares parameters for the chi,phi and omega angles for the run numbers
 * where these will be optimized.
 */
    void PeakHKLErrors::setUpOptRuns()
    {

      std::vector<std::string> OptRunNums;
      std::string OptRunstemp( OptRuns );
      if( OptRuns.size() >0 && OptRuns.at( 0 ) == '/' )
        OptRunstemp = OptRunstemp.substr( 1, OptRunstemp.size() - 1 );

      if( OptRunstemp.size() >0 && OptRunstemp.at( OptRunstemp.size() - 1 ) == '/' )
        OptRunstemp = OptRunstemp.substr( 0, OptRunstemp.size() - 1 );

      boost::split( OptRunNums, OptRunstemp, boost::is_any_of( "/" ) );

      for( size_t i = 0; i<OptRunNums.size(); i++ )
      {
        declareParameter( "chi" + OptRunNums[i], 0.0, "Chi sample orientation value" );
        declareParameter( "phi" + OptRunNums[i], 0.0, "Phi sample orientation value" );
        declareParameter( "omega" + OptRunNums[i], 0.0, "Omega sample orientation value" );
      }
    }

    /**
     * "Clones" a parameter map duplicating all Parameters with double,V3D,int and string parameter
     * values that apply to the given component and all(most) of the components children.
     *
     * If the component is an instrument, this parameter map can be used to create
     * a separate parameterized instrument close to the original instrument.
     *
     * NOTE: For speed purposes, if a component( or subcomponent) has too many children(180
     * or more),the parameters corresponding to these children( and subchildren) will not
     * be added to the parameter map
     *
     *
     * @param pmap       The new parameter map to which the new Parameters are to be added
     *
     * @param component  The component along with most of its children and subchildren for
     *                   which Parameters that correspond to these will be considered.
     *
     * @param pmapSv     The old parameter map from which copies of the parameters corresponding
     *                   to the given component or subchild are added to pmap
     */
    void  PeakHKLErrors::cLone( boost::shared_ptr< Geometry::ParameterMap> &pmap,
              boost::shared_ptr< const Geometry::IComponent> component ,
              boost::shared_ptr<const Geometry::ParameterMap> &pmapSv )
    {
      if( !component )
        return;
      if( component->isParametrized() )
      {

        std::set<std::string> nms = pmapSv->names( component.get() );
        for( std::set<std::string>::iterator it = nms.begin(); it != nms.end(); ++it )
        {

          if( pmapSv->contains( component.get(), *it, "double" ) )
            {
              std::vector<double> dparams = pmapSv->getDouble( component->getName(),  *it );
              pmap->addDouble( component.get(), *it, dparams[0] );
              continue;
            }


          if( pmapSv->contains( component.get(), *it, "V3D" ) )
            {
              std::vector<V3D> V3Dparams = pmapSv->getV3D( component->getName(), *it );
              pmap->addV3D( component.get(), *it, V3Dparams[0] );
              continue;
            }


          if( pmapSv->contains( component.get( ), *it, "int"))
            {
              std::vector<int> iparams = pmapSv->getType<int>( component->getName( ), *it);
              pmap->addInt( component.get(), *it, iparams[0] );
              continue;
            }


          if( pmapSv->contains( component.get(), *it, "string" ) )
            {
              std::vector<std::string> sparams = pmapSv->getString( component->getName(), *it );
              pmap->addString( component.get(), *it, sparams[0] );
              continue;
            }


          if( pmapSv->contains( component.get(), *it, "Quat" ) )
            {
              std::vector<Kernel::Quat> sparams = pmapSv->getType<Kernel::Quat>( component->getName(), *it );
              pmap->addQuat( component.get(), *it, sparams[0] );
              continue;
            }
        }

        boost::shared_ptr<const CompAssembly> parent = boost::dynamic_pointer_cast<const CompAssembly>( component );
        if( parent && parent->nelements() < 180 )//# need speed up. Assume pixel elements of a Panel have no attributes
          for( int child = 0; child< parent->nelements(); child++ )
          {
            boost::shared_ptr<const Geometry::IComponent> kid = boost::const_pointer_cast<const Geometry::IComponent>( parent->getChild( child ) );
            if( kid )
            cLone( pmap,  kid, pmapSv );
          }
      }
    }

    /**
     * Creates a new parameterized instrument for which the parameter values can be changed
     *
     * @param Peaks - a PeaksWorkspace used to get the original instrument.  The instrument from the 0th peak is
     *                the one that is used.
     *
     * NOTE: All the peaks in the PeaksWorkspace must use the same instrument.
     */
    boost::shared_ptr<Geometry::Instrument> PeakHKLErrors::getNewInstrument( PeaksWorkspace_sptr Peaks )const
    {
      Geometry::Instrument_const_sptr instSave = Peaks->getPeak( 0 ).getInstrument();
           boost::shared_ptr<Geometry::ParameterMap> pmap( new Geometry::ParameterMap() );
           boost::shared_ptr<const Geometry::ParameterMap> pmapSv = instSave->getParameterMap();

           if ( !instSave )
           {
             g_log.error( " Peaks workspace does not have an instrument" );
             throw std::invalid_argument( " Not all peaks have an instrument" );
           }
           boost::shared_ptr<Geometry::Instrument> instChange( new Geometry::Instrument() );

           if ( !instSave->isParametrized() )
           {

             boost::shared_ptr<Geometry::Instrument> instClone( instSave->clone() );
             boost::shared_ptr<Geometry::Instrument> Pinsta( new Geometry::Instrument( instSave, pmap ) );

             instChange = Pinsta;
           }
           else //catch(... )
           {
             boost::shared_ptr<Geometry::Instrument> P1( new Geometry::Instrument( instSave->baseInstrument(),
                 pmap ) );
             instChange = P1;

           }

           if ( !instChange )
           {
             g_log.error( "Cannot 'clone' instrument" );
             throw std::logic_error( "Cannot clone instrument" );

           }
           //------------------"clone" orig instruments pmap -------------------

           cLone(  pmap, instSave, pmapSv );
           IObjComponent_const_sptr sample = instChange->getSample();
           V3D sampPos = sample->getRelativePos();
           V3D sampOffsets( getParameter( "SampleXOffset" ), getParameter( "SampleYOffset" ), getParameter( "SampleZOffset" ) );

           pmap->addPositionCoordinate( sample.get(),  std::string("x"),  sampPos.X() + sampOffsets.X() );
           pmap->addPositionCoordinate( sample.get(), std::string("y"), sampPos.Y() + sampOffsets.Y() );
           pmap->addPositionCoordinate( sample.get(), std::string("z"), sampPos.Z() + sampOffsets.Z() );

           return instChange;

    }

    /**
     * Updates the map from run number to GoniometerMatrix
     *
     * @param Peaks    The PeaksWorkspace whose peaks contain the run numbers
     *                   along with the corresponding GoniometerMatrix
     *
     * @param OptRuns  A '/' separated "list" of run numbers to include in the
     *                  map. This string must also start and end with a '/'
     *
     * @param Res      The resultant map.
     */
   void PeakHKLErrors::getRun2MatMap( PeaksWorkspace_sptr & Peaks, const std::string &OptRuns,
       std::map<int, Mantid::Kernel::Matrix<double> > &Res)const
    {

      for (int i = 0; i < Peaks->getNumberPeaks(); ++i)
      {
        IPeak & peak_old = Peaks->getPeak((int)i);

        int runNum = peak_old.getRunNumber();
        std::string runNumStr = boost::lexical_cast<std::string>(runNum);
        size_t N = OptRuns.find("/" + runNumStr + "/");
        if (N < OptRuns.size())
        {
          double chi = getParameter("chi" + boost::lexical_cast<std::string>(runNumStr));
          double phi = getParameter("phi" + boost::lexical_cast<std::string>(runNumStr));
          double omega = getParameter("omega" + boost::lexical_cast<std::string>(runNumStr));
          Mantid::Geometry::Goniometer uniGonio;
          uniGonio.makeUniversalGoniometer();
          uniGonio.setRotationAngle("phi", phi);
          uniGonio.setRotationAngle("chi", chi);
          uniGonio.setRotationAngle("omega", omega);
          Res[runNum] = uniGonio.getR();
        }
      }

    }

     void PeakHKLErrors::function1D  ( double *out, const double *xValues, const size_t nData )const
     {
      PeaksWorkspace_sptr Peaks =
          AnalysisDataService::Instance().retrieveWS<PeaksWorkspace> ( PeakWorkspaceName );

      boost::shared_ptr<Geometry::Instrument> instNew = getNewInstrument( Peaks );

      if ( !Peaks )
        throw std::invalid_argument( "Peaks not stored under the name " + PeakWorkspaceName );

      std::map<int, Mantid::Kernel::Matrix<double> > RunNum2GonMatrixMap;
      getRun2MatMap( Peaks, OptRuns,  RunNum2GonMatrixMap);
      const DblMatrix & UBx = Peaks->sample().getOrientedLattice().getUB();

      DblMatrix UBinv( UBx );
      UBinv.Invert();
      UBinv /= ( 2 * M_PI );

      double ChiSqTot = 0.0;
      for( size_t i = 0; i< nData; i++ )
      {
        int peakNum = (int)( .5 + xValues[i] );
        IPeak & peak_old = Peaks->getPeak( peakNum );

        int runNum = peak_old.getRunNumber();
        std::string runNumStr = boost::lexical_cast<std::string>( runNum );
        Peak peak = SCDPanelErrors::createNewPeak( peak_old, instNew, 0, peak_old.getL1() );

        size_t N = OptRuns.find( "/" + runNumStr + "/" );
        if( N < OptRuns.size() )
        {

          peak.setGoniometerMatrix( RunNum2GonMatrixMap[runNum] );
        }
       // Kernel::Matrix<double> Gon  = peak.getGoniometerMatrix(  );
       // std::cout<<"Gon = "<< Gon<<std::endl;
        V3D hkl = UBinv * peak.getQSampleFrame();

        double d = 0;
        for( int k = 0; k<3; k++ )
        {
          double d1 = hkl[k] - floor( hkl[k] );
          if( d1>.5 ) d1 = d1 - 1;
          if( d1 < -.5 ) d1 = d1 + 1;
          if( fabs( d1 )>fabs( d ) )
            d = d1;
        }
        out[i] = d;

        ChiSqTot += d*d;
      }

      g_log.debug() << "------------------------Function-----------------------------------------------"<<std::endl;
      for( size_t p = 0 ; p < nParams() ; p++ )
      {
        g_log.debug() << parameterName(p)<<"("<<getParameter(p)<<"),";
        if ((p + 1) % 6 ==0 )
          g_log.debug() << std::endl;
      }
      g_log.debug() << std::endl;
      g_log.debug() <<"Off constraints=";
      for (size_t p = 0; p < nParams(); p++)
       {
         IConstraint* constr = getConstraint( p );
         if( constr  )
           if( (constr->check() > 0))
               g_log.debug() << "(" << parameterName( p ) << "=" << constr->check() << ");";
       }
      g_log.debug()<< std::endl;

      g_log.debug() << "    Chi**2 = " << ChiSqTot << "     nData = "<< nData <<std::endl;
    }

     void PeakHKLErrors::functionDeriv1D ( Jacobian* out, const double *xValues, const size_t nData )
     {
      PeaksWorkspace_sptr Peaks =
          AnalysisDataService::Instance().retrieveWS<PeaksWorkspace> ( PeakWorkspaceName );
      boost::shared_ptr<Geometry::Instrument> instNew = getNewInstrument( Peaks );

      const DblMatrix & UB = Peaks->sample().getOrientedLattice().getUB();
      DblMatrix UBinv( UB );
      UBinv.Invert();
      UBinv /= 2 * M_PI;

      std::map<int, Kernel::Matrix<double> > RunNums2GonMatrix;
      getRun2MatMap(  Peaks, OptRuns,RunNums2GonMatrix);

      g_log.debug()<<"----------------------------Derivative------------------------" << std::endl;

      V3D samplePosition = instNew->getSample()->getPos();
      IPeak& ppeak = Peaks->getPeak( 0 );
      double L0 = ppeak.getL1();
      double velocity = ( L0 + ppeak.getL2() ) / ppeak.getTOF();

       double K = 2 * M_PI / ppeak.getWavelength() / velocity;//2pi/lambda = K* velocity
       V3D beamDir = instNew->getBeamDirection();

       size_t paramNums[] = { parameterIndex( std::string( "SampleXOffset" ) ),
                              parameterIndex( std::string( "SampleYOffset" ) ),
                              parameterIndex( std::string( "SampleZOffset" ) ) };

      for ( size_t i = 0; i < nData; i++ )
      {
        int peakNum = (int)( .5 + xValues[i] );
        IPeak & peak_old = Peaks->getPeak( peakNum );
        Peak peak = SCDPanelErrors::createNewPeak( peak_old, instNew, 0, peak_old.getL1() );

        int runNum = peak_old.getRunNumber();
        std::string runNumStr = boost::lexical_cast<std::string>( runNum );

        for ( int kk = 0; kk <(int) nParams(); kk++ )
          out->set( i, kk , 0.0 );

        double chi, phi, omega;
        size_t chiParamNum, phiParamNum, omegaParamNum;

        size_t N = OptRuns.find( "/" +  runNumStr );
        if ( N < OptRuns.size() )
        {
          chi = getParameter( "chi" + ( runNumStr ) );
          phi = getParameter( "phi" + ( runNumStr ) );
          omega = getParameter( "omega" + ( runNumStr ) );

          peak.setGoniometerMatrix( RunNums2GonMatrix[runNum] );

          chiParamNum = parameterIndex( "chi" + ( runNumStr ) );
          phiParamNum = parameterIndex( "phi" + ( runNumStr ) );
          omegaParamNum = parameterIndex( "omega" + ( runNumStr ) );
        }
        else
        {

          Geometry::Goniometer Gon( peak.getGoniometerMatrix() );
          std::vector<double> phichiOmega = Gon.getEulerAngles( "YZY" );
          chi = phichiOmega[1];
          phi = phichiOmega[2];
          omega = phichiOmega[0];

          chiParamNum = phiParamNum =  omegaParamNum = nParams() + 10;
        }


        double TT = M_PI / 180.0;
        chi *= TT;
        phi *= TT;
        omega *= TT;
        V3D hkl = UBinv * peak.getQSampleFrame();
        int maxoffsetPos = -1;
        double maxOffset = 0.0;

         for ( int k = 0; k<3; k++ )
          {
            double d = hkl[k] - floor( hkl[k] );

             if( d >.5 ) d = d - 1;
             if( d < - .5 ) d = d + 1;
             if( fabs( d ) > fabs( maxOffset ) )
              {
                maxOffset = d;
                maxoffsetPos = k;
              }
          }
        if ( phiParamNum < nParams() )
        {
          //Rotation Matrices for chi, phi and omega
          double chiList[] =
          { cos( chi ), -sin( chi ), 0, sin( chi ), cos( chi ), 0, 0,  0 , 1 };
          double phiList[] =
          { cos( phi ), 0, sin( phi ), 0,  1, 0, -sin( phi ) , 0,  cos( phi ) };
          double omegaList[] =
          { cos( omega ), 0, sin( omega ), 0,   1, 0, -sin( omega ) , 0, cos( omega ) };

          //Derivatives of Rotation matrices for chi, phi, omega in radiams
          double DchiList[] =
          { -sin( chi ), -cos( chi ), 0, cos( chi ), -sin( chi ), 0, 0,  0 , 0 };
          double DphiList[] =
          {  -sin( phi ), 0, cos( phi ), 0,  0, 0, -cos( phi ) ,0, -sin( phi )  };
          double DomegaList[] =
          { -sin( omega ), 0, cos( omega ), 0, 0, 0, -cos( omega ) ,0, -sin( omega ) };

          //Convert lists into Matrices to get additional functionality
          std::vector<double> VV( chiList, chiList + 9 );

          Matrix<double> chiMatrix( VV );
          Matrix<double> phiMatrix( std::vector<double> ( phiList, phiList + 9 ) );
          Matrix<double> omegaMatrix( std::vector<double> ( omegaList, omegaList + 9  ) );
          Matrix<double> dchiMatrix( std::vector<double> ( DchiList, DchiList + 9  ) );
          Matrix<double> dphiMatrix( std::vector<double> ( DphiList, DphiList + 9  ) );
          Matrix<double> domegaMatrix( std::vector<double> ( DomegaList, DomegaList + 9  ) );

          //Calculate Derivatives wrt chi(phi,omega) in degrees
          Matrix<double> R = omegaMatrix * chiMatrix * dphiMatrix * ( M_PI/180. );
          V3D lab = peak.getQLabFrame();
          V3D Dhkl0 = UBinv * R.Transpose() * lab;

          R = omegaMatrix * dchiMatrix * phiMatrix * ( M_PI/180 );
          V3D Dhkl1 = UBinv * R.Transpose() * peak.getQLabFrame();
          R = domegaMatrix * chiMatrix * phiMatrix * ( M_PI/180 );
          V3D Dhkl2 = UBinv * R.Transpose() * peak.getQLabFrame();

          out->set( i, chiParamNum,  Dhkl1[maxoffsetPos] );
          out->set( i, phiParamNum,  Dhkl0[maxoffsetPos] );
          out->set( i, omegaParamNum,  Dhkl2[maxoffsetPos] );
          }//if optimize for chi phi and omega on this peak

          //-------------------- Sample Orientation derivatives ----------------------------------
           //Qlab = -KV + k|V|*beamdir
           //D = pos-sampPos
           //|V|= vmag=(L0 + D )/tof
           //t1= tof - L0/|V|   {time from sample to pixel}
           //V = D/t1
           V3D D = peak.getDetPos() - samplePosition;
           double vmag = ( L0 + D.norm() )/peak.getTOF();
           double t1 = peak.getTOF() - L0/vmag;
           V3D V = D/t1;

           //Derivs wrt sample x, y, z
           //Ddsx =( - 1, 0, 0),  d|D|^2/dsx 2|D|d|D|/dsx =d(tranp(D)* D)/dsx =2 Ddsx* tranp(D)
           //|D| also called Dmag
           V3D Dmagdsxsysz( D );
           Dmagdsxsysz *= ( -1/D.norm() );

           V3D vmagdsxsysz = Dmagdsxsysz/peak.getTOF();

           V3D t1dsxsysz = vmagdsxsysz * ( L0/vmag/vmag );
           Matrix<double> Gon = peak.getGoniometerMatrix();
           Gon.Invert();

           //x=0 is deriv wrt SampleXoffset, x=1 is deriv wrt SampleYoffset, etc.
           for( int x = 0; x< 3; x++ )
           {
             V3D pp;
             pp[x] = 1;
             V3D dQlab1 = pp/-t1 -  D * ( t1dsxsysz[x]/t1/t1 );
             V3D dQlab2 = beamDir * vmagdsxsysz[x];
             V3D dQlab = dQlab2 - dQlab1;
             dQlab *= K;

             V3D dQSamp = Gon * dQlab;
             V3D dhkl = UBinv * dQSamp;


             out->set( i,  paramNums[x], dhkl[ maxoffsetPos]  );

           }

      }

    }
}
}
