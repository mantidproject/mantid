//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidMDAlgorithms/SimulateResolution.h"
#include <math.h>

// @todo: This needs a factory (copied from MC Absorbtion algo)
#include "MantidKernel/MersenneTwister.h"

#include <gsl/gsl_errno.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_roots.h>

#include "MantidKernel/Tolerance.h"
#include "MantidGeometry/Math/mathSupport.h"
#include "MantidKernel/Matrix.h"
#include "MantidAPI/IMDIterator.h"

#include <boost/shared_array.hpp>


using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::MDAlgorithms;

namespace Mantid
{
  namespace MDAlgorithms
  {
    // Constructor
    SimulateResolution::SimulateResolution() : m_randGen(NULL), m_randSeed(12345678), m_integrationMethod(mcIntegration), m_random(sobol), m_qRvec(NULL)
    {
      m_magForm = boost::shared_ptr<MDAlgorithms::MagneticFormFactor>( new MagneticFormFactor(0,0,500) );
      if( !m_randGen )
      {
        m_randGen = new Kernel::MersenneTwister;
        int seedValue = 12345;
        m_randGen->setSeed(seedValue);
      }
      m_mcOptVec.resize(9,true);; // Nine processes are modelled in current implementation
      m_mcVarCount.resize(m_mcOptVec.size());
      m_mcVarCount.at(0) = 1; //LineShape
      m_mcVarCount.at(1) = 2; //Aperture
      m_mcVarCount.at(2) = 1; //Chopper
      m_mcVarCount.at(3) = 1; //ChopperJitter
      m_mcVarCount.at(4) = 3; //SampleVolume
      m_mcVarCount.at(5) = 1; //DetectorDepth
      m_mcVarCount.at(6) = 2; //DetectorArea
      m_mcVarCount.at(7) = 1; //DetectorTimeBin
      m_mcVarCount.at(8) = 2; //CrystalMosaic
      setAttributeValue("MCLoopMin",100);
      setAttributeValue("MCLoopMax",1000);
      setAttributeValue("MCTol",1.e-5);
    }

    SimulateResolution::~SimulateResolution()
    {
      delete m_randGen;
    }
    void SimulateResolution::setRunDataInfo(boost::shared_ptr<Mantid::MDAlgorithms::RunParam> runData)
    {
      // temporary public method at set RunPara data
      m_runData.push_back(runData);
    }

    void SimulateResolution::setWorkspaceMD(WorkspaceGroup_sptr wsGroup)
    {
      m_mdWorkspaces = wsGroup;
    }

    double SimulateResolution::functionMD(const Mantid::API::IMDIterator& it) const
    {

      // Is runData setup?
      if(m_runData.empty())
        throw std::runtime_error("FunctionMD has no runData to extract properties from");

      double fgSignal = 0.;
      double fgError = 0.;
      // loop over each MDPoint in current MDBox
      for(size_t j=0; j < it.getNumEvents(); j++)
      {
        // Calculate convolution
        //RunParam rP; // TODO set from runIndex of internal point
        fgSignal += sqwConvolution(it,j,fgError);
        UNUSED_ARG(fgError);
      }

      // Return mean
      return fgSignal/double(it.getNumEvents());
    }

    double SimulateResolution::sqwConvolution(const Mantid::API::IMDIterator& it, size_t & event, double & error) const
    {
      // Only Monte Carlo at present
      if(m_integrationMethod == mcIntegration)
      {
        return sqwConvolutionMC(it, event, error);
      }
      return 0.0;
    }

    double SimulateResolution::sqwConvolutionMC(const Mantid::API::IMDIterator& it, size_t & event, double & error) const
    {
      // find the run and detector for this MDEvent
      int runID = it.getInnerRunIndex(event);
      int detectorID = it.getInnerDetectorID(event); UNUSED_ARG(detectorID);
      double simSig=0.;
      std::vector<double> qE;
      const double small=1e-10;
      for( size_t index=0;index<4;index++)
        qE.push_back(it.getInnerPosition(event,index));

      /// Pointer to the run parameters for each run which may be included in a cut
      boost::shared_ptr<Mantid::MDAlgorithms::RunParam> runData = m_runData.at(runID);
      // get crystal parameters and ubinv/uinv matrix for run
      //Mantid::Geometry::OrientedLattice& lattice; // = m_mdews->getExperimentInfo(0)->sample().getOrientedLattice();
      //lattice.;
      error=0.0;
      bool broad=userModelIsBroad();
      double weight;
      double acc;
      std::vector<double> result;
      std::vector<double> perturbQE;
      std::vector<double> yVec;
      double sum=0., sumSqr=0.;
      double eta2,eta3;
      // temp data which will come from runInfo
      double gauPre=0.0;
      std::vector<double> params;
      getParams(params);
      // get attributes - should only do once and store
      const int mcLoopLimit=getAttribute("MCLoopMax").asInt();
      const int mcLoopMin=getAttribute("MCLoopMin").asInt();
      const double mcTol=getAttribute("MCTol").asDouble();

      Kernel::V3D detPos,detDim;
      double deps;
      runData->getDetInfo(detectorID,detPos,detDim, deps);
      //double phi=0.37538367018968838, beta=2.618430210304493, x2Detector=6.034; // detector angles and distance TEMP as cobalt demo, 1st det
      double eps=qE[3];
      Kernel::DblMatrix dMat(3,3), dMatInv(3,3), bMat(6,11);
      // Initialise variables for Monte Carlo loop:
      // Get transformation and resolution matricies, and nominal Q:
      const double wi = sqrt(runData->getEi()/2.0721418);
      const double wf = sqrt((runData->getEi()-eps)/2.0721418);
      const double detTimeBin = (3.8323960e-4 * detPos[2] / (wf*wf*wf)) * deps;
      //std::vector<double> detectorBB = {0.0254, 0.0225, 0.6*0.025}; // Default for cobalt demo, no good in general

      dMatrix (detPos[0], detPos[1], dMat, dMatInv);
      bMatrix (wi, wf, runData->getX0(), runData->getXa(), runData->getX1(), detPos[2], runData->getThetam(), runData->getAngVel(),
               runData->getSMat(), dMat, bMat);

      // get nominal q vector
      std::vector<double> q0;
      for(size_t i=0;i<4;i++)
        q0.push_back(it.getInnerPosition(event,i));

      const int sizeRanvec=14;
      boost::shared_array<double> ranvec( new double[sizeRanvec]);
      gsl_qrng *qRvec;
      if(m_random==sobol)
        qRvec =gsl_qrng_alloc(gsl_qrng_sobol, sizeRanvec);
      else
        throw std::invalid_argument("Invalid random method");

      // Loop over quasi-random points in 14 dimensional space to determine the resolution effect
      // For each point find the effective Q-E point and evaluate the user scattering function there.
      // Then take average of all as expected scattering.
      for( int mcStep=1; mcStep<=mcLoopLimit; mcStep++)
      {
        // Get  quasi random point
        gsl_qrng_get( qRvec, ranvec.get() );
        // Get corresponding actual changes in coordinates and time
        mcYVec(ranvec.get() ,runData, detDim, detTimeBin,  yVec, eta2, eta3);
        // Map these changes to the perturbation in Q-E and add with nominal value
        mcMapYtoQEVec(wi,wf, q0, bMat, dMatInv, yVec, eta2, eta3,
                                       perturbQE);
        perturbQE[0] += q0[0];
        perturbQE[1] += q0[1];
        perturbQE[2] += q0[2];
        perturbQE[3] += q0[3];

        // Call user function on new point
        userSqw(runData, params,perturbQE,result);
        // Models are either broad of sharp and the data returned is different in these cases.
        if(broad)
          weight=result[0];
        else
        {
          const double dE=perturbQE[3]-result[1];
          weight=result[0]*exp(-gauPre*(dE*dE));
        }
        // Gather average, and estimate uncertainty
        sum+=weight;
        sumSqr+=weight*weight;
        // check for convergence every mcLoopMin steps, end of loop
        if( (mcStep % mcLoopMin) ==0 || mcStep==mcLoopLimit )
        {
          simSig=sum/mcStep;
          error=sqrt(fabs( ( sumSqr/mcStep+ - simSig*simSig ))/mcStep );

          // break on relative error or very small after mcLoop min iterations
          if(fabs(simSig)>small)
          {
            acc=fabs(error/simSig);
            if(acc<mcTol)
              break;
          }
          else
            break;
        }
      }
      gsl_qrng_free(qRvec);
      //UNUSED_ARG(lattice);
      return simSig;
    }

    void SimulateResolution::setMagneticForm(const int atomicNo, const int ionisation)
    {
      m_magForm->setFormFactor(atomicNo,ionisation,500);
    }

    double SimulateResolution::magneticForm(const double qSquared) const
    {
      return( m_magForm->formTable(qSquared));
    }

    // Return next pseudo or quasi random point in the N dimensional space
    void SimulateResolution::getNextPoint(std::vector<double>& point) const
    {
      if(point.size()<m_randSize) // make sure enough room for a point
        point.resize(m_randSize);
      if(m_random==mTwister) {
        for(size_t i=0;i<point.size();i++)
          point[i]=m_randGen->next();
      }
      else
      {
        gsl_qrng_get( m_qRvec, &point[0] );
      }
    }

    /**
     * Initialise the random number generator
     */
    void SimulateResolution::initRandom()
    {
      // set number of random variables needed per point according to MC options
      m_randSize=0;
      for(size_t i=0; i<m_mcOptVec.size();i++)
        m_randSize+=m_mcVarCount.at(i);
      // set up required generator - note that Sobol not in boost random number interface
      if(m_random==sobol)
      {
        if(m_qRvec!=NULL)
          gsl_qrng_free(m_qRvec);
        m_qRvec = gsl_qrng_alloc(gsl_qrng_sobol, static_cast<unsigned int> (m_randSize));
      }
      else //Non Sobol random numbers
      {
        if( !m_randGen )
          m_randGen = new Kernel::MersenneTwister;
        m_randGen->setSeed(m_randSeed);
      }
    }

    void SimulateResolution::initRandom(const bool methodSobol)
    {
      if(methodSobol)
        m_random = sobol;
      else
        m_random = mTwister;
      initRandom();
    }

    /**
     * Reset the random number generator
     */
    void SimulateResolution::resetRandomNumbers()
    {
      if( m_random == sobol )
      {
        if(m_qRvec!=NULL)
          gsl_qrng_free(m_qRvec);
        m_qRvec = gsl_qrng_alloc(gsl_qrng_sobol, static_cast<unsigned int> (m_randSize));
      }
      else
        if( m_randGen )
        {
          m_randGen->setSeed(m_randSeed);
        }
    }


    /**
     * bose function from tobyfit
     */
    double SimulateResolution::bose(const double eps, const double temp) const
    {

      if( temp<0. )
        return( (eps>=0)?(eps):(0.) );

      const double kB = 0.08617347;
      return( (kB*temp)*pop(eps/(kB*temp)) );
    }

    /**
     * formTable function from tobyfit - not yet implemented
     */
    double SimulateResolution::formTable(const double qsqr) const
    {
      UNUSED_ARG(qsqr);
      return 0.0; //to do - implement this look up table
    }

    /**
     * pop function from tobyfit
     */
    double SimulateResolution::pop(const double y) const
    {
      if ( (fabs(y) > 0.1 ) )
      {
        double ans = (fabs(y)) / (1.0 - (exp(-(fabs(y)))));
        return( (y<0)? ( ans*(exp(-fabs(y)))) : (ans) );
      }
      else
      {
        double by2=0.5, by6=1./6., by60=1./60., by42=1./42., by40=1./40.;
        return( 1.0 + by2*y*( 1.0 + by6*y*( 1.0 - by60*(y*y)
            *(1.0-by42*(y*y)*(1.0-by40*(y*y) )))) );
      }
    }

    /**
     * bMatrix function to determine the elements of matrix bMat - TGP thesis 1991 p112
     */
    void SimulateResolution::bMatrix(const double wi, const double wf, const double x0, const double xa, const double x1,
        const double x2, const double thetam, const double angvel,
        const Kernel::Matrix<double> & sMat, const Kernel::Matrix<double> & dMat,
        Kernel::Matrix<double> & bMat) const
    {
      //double precision wi, wf, x0, xa, x1, x2, thetam, angvel, sMat(3,3), dMat(3,3), bMat(6,11)
      /*
            ! Input:
            !     x0       moderator-chopper distance (m)
            !     xa       aperture-chopper distance  (m)
            !     x1       chopper-sample distance  (m)
            !     x2       sample-detector distance (m)
            !     thetam      moderator tilt angle  (rad)
            !     angvel      angular frequency of chopper
            !     wi       incident wavevector of nominal neutron (Ang^-1)
            !     wf       final wavevector of nominal neutron  (Ang^-1)
            !     sMat    matrix for re-expressing a sample coordinate in the laboratory frame
            !     dMat    matrix for expressing a laboratory coordinate in the detector frame
            !
            ! Output:
            !     bMat    matrix elements as defined on p112 of T.Perring's thesis (1991)
       */
      const size_t nRows=6, nCols=11; // expected size of bMat


      // Calculate velocities and times:
      // -------------------------------

      const double veli = 629.62237 * wi;
      const double velf = 629.62237 * wf;
      const double ti = x0/veli;
      const double tf = x2/velf;

      const Kernel::Matrix<double> ds = dMat*sMat; //TODO fix the sample size terms - they are wrong at present

      // Get some coefficients:
      // ----------------------

      const double g1 = (1.0 - angvel*(x0+x1)*tan(thetam)/veli );
      const double g2 = (1.0 - angvel*(x0-xa)*tan(thetam)/veli );
      const double f1 =  1.0 + (x1/x0)*g1;
      const double f2 =  1.0 + (x1/x0)*g2;
      const double gg1 = g1 / ( angvel*(xa+x1) );
      const double gg2 = g2 / ( angvel*(xa+x1) );
      const double ff1 = f1 / ( angvel*(xa+x1) );
      const double ff2 = f2 / ( angvel*(xa+x1) );

      const double cp_i = wi/ti;
      const double ct_i = wi/(xa+x1);
      const double cp_f = wf/tf;
      const double ct_f = wf/x2;

      if(bMat.numCols()<nCols || bMat.numRows()<nRows)
        throw std::invalid_argument("bMat too small in bMatrix");
      // Calculate the matrix elements:
      //-------------------------------

      int beam=2,up=1,horiz=0; // allow for Mantid axes differing to Tobyfit ones
      bMat [beam][0] =  cp_i;
      bMat [beam][1] = -cp_i * gg1;
      bMat [beam][2] =  0.0;
      bMat [beam][3] = -cp_i;
      bMat [beam][4] =  cp_i * gg2 * sMat[1][0];
      bMat [beam][5] =  cp_i * gg2 * sMat[1][1];
      bMat [beam][6] =  cp_i * gg2 * sMat[1][2];
      bMat [beam][7] =  0.0;
      bMat [beam][8] =  0.0;
      bMat [beam][9] =  0.0;
      bMat [beam][10]=  0.0;

      bMat [horiz][0] =  0.0;
      bMat [horiz][1] = -ct_i;
      bMat [horiz][2] =  0.0;
      bMat [horiz][3] =  0.0;
      bMat [horiz][4] =  ct_i * sMat[1][0];
      bMat [horiz][5] =  ct_i * sMat[1][1];
      bMat [horiz][6] =  ct_i * sMat[1][2];
      bMat [horiz][7] =  0.0;
      bMat [horiz][8] =  0.0;
      bMat [horiz][9] =  0.0;
      bMat [horiz][10]=  0.0;

      bMat [up][0] =  0.0;
      bMat [up][1] =  0.0;
      bMat [up][2] = -ct_i;
      bMat [up][3] =  0.0;
      bMat [up][4] =  ct_i * sMat[2][0];
      bMat [up][5] =  ct_i * sMat[2][1];
      bMat [up][6] =  ct_i * sMat[2][2];
      bMat [up][7] =  0.0;
      bMat [up][8] =  0.0;
      bMat [up][9] =  0.0;
      bMat [up][10]=  0.0;

      int beamf=beam+3, upf=up+3, horizf=horiz+3; // do output components 3,4,5
      bMat [beamf][0] =  cp_f * (-x1/x0);
      bMat [beamf][1] =  cp_f *  ff1;
      bMat [beamf][2] =  0.0;
      bMat [beamf][3] =  cp_f * (x0+x1)/x0;
      /*
      bMat [beamf][4] =  cp_f * ( sMat[beam][0]/veli - (dMat[0][0]*sMat[0][0]+dMat[0][1]*sMat[1][0]+dMat[0][2]*sMat[2][0])/velf
          - ff2*sMat[horiz][0] );
      bMat [beamf][5] =  cp_f * ( sMat[beam][1]/veli - (dMat[0][0]*sMat[0][1]+dMat[0][1]*sMat[1][1]+dMat[0][2]*sMat[2][1])/velf
          - ff2*sMat[horiz][1] );
      bMat [beamf][6] =  cp_f * ( sMat[beam][2]/veli - (dMat[0][0]*sMat[0][2]+dMat[0][1]*sMat[1][2]+dMat[0][2]*sMat[2][2])/velf
          - ff2*sMat[horiz][2] );
          */
      bMat [beamf][4] =  cp_f * ( sMat[beam][0]/veli - (ds[2][2])/velf
          - ff2*sMat[horiz][0] );
      bMat [beamf][5] =  cp_f * ( sMat[beam][1]/veli - (ds[2][0])/velf
          - ff2*sMat[horiz][1] );
      bMat [beamf][6] =  cp_f * ( sMat[beam][2]/veli - (ds[2][1])/velf
          - ff2*sMat[horiz][2] );
      bMat [beamf][7] =  cp_f/velf;
      bMat [beamf][8] =  0.0;
      bMat [beamf][9] =  0.0;
      bMat [beamf][10]= -cp_f;

      bMat [horizf][0] =  0.0;
      bMat [horizf][1] =  0.0;
      bMat [horizf][2] =  0.0;
      bMat [horizf][3] =  0.0;
      //bMat [horizf][4] = -ct_f * ( dMat[1][0]*sMat[0][0] + dMat[1][1]*sMat[1][0] + dMat[1][2]*sMat[2][0] );
      //bMat [horizf][5] = -ct_f * ( dMat[1][0]*sMat[0][1] + dMat[1][1]*sMat[1][1] + dMat[1][2]*sMat[2][1] );
      //bMat [horizf][6] = -ct_f * ( dMat[1][0]*sMat[0][2] + dMat[1][1]*sMat[1][2] + dMat[1][2]*sMat[2][2] );
      bMat [horizf][4] = -ct_f * ( ds[0][2] );
      bMat [horizf][5] = -ct_f * ( ds[0][0] );
      bMat [horizf][6] = -ct_f * ( ds[0][1] );
      bMat [horizf][7] =  0.0;
      bMat [horizf][8] =  ct_f;
      bMat [horizf][9] =  0.0;
      bMat [horizf][10]=  0.0;

      bMat [upf][0] =  0.0;
      bMat [upf][1] =  0.0;
      bMat [upf][2] =  0.0;
      bMat [upf][3] =  0.0;
      //bMat [upf][4] = -ct_f * ( dMat[2][0]*sMat[0][0] + dMat[2][1]*sMat[1][0] + dMat[2][2]*sMat[2][0] );
      //bMat [upf][5] = -ct_f * ( dMat[2][0]*sMat[0][1] + dMat[2][1]*sMat[1][1] + dMat[2][2]*sMat[2][1] );
      //bMat [upf][6] = -ct_f * ( dMat[2][0]*sMat[0][2] + dMat[2][1]*sMat[1][2] + dMat[2][2]*sMat[2][2] );
      bMat [upf][4] = -ct_f * ( ds[1][2] );
      bMat [upf][5] = -ct_f * ( ds[1][0] );
      bMat [upf][6] = -ct_f * ( ds[1][1] );
      bMat [upf][7] =  0.0;
      bMat [upf][8] =  0.0;
      bMat [upf][9] =  ct_f;
      bMat [upf][10]=  0.0;

    }

    /**
     * d_matrix function from tobyfit
     */

    void SimulateResolution::dMatrix(const double phi, const double beta, Kernel::Matrix<double> & dMat,
        Kernel::Matrix<double> & dMatInv) const
    {

      /*
            ! Input:
            !     phi       scattering angle of centre of detector
            !     beta      azimuthal  angle of centre of detector
            ! Output:
            !     dMat    matrix for converting laboratory coordinates to detector coordinates
            !     dinv_mat corresponding inverse matrix
       */
      Geometry::Goniometer gDet;
      gDet.makeUniversalGoniometer();
      //const double deg2rad=M_PI/180.;
      const double rad2deg=180./M_PI;
      gDet.setRotationAngle("phi", phi*rad2deg);
      gDet.setRotationAngle("chi", beta*rad2deg);
      dMat=gDet.getR();
      dMatInv=dMat;
      dMatInv.Invert();
      /*
      const double cp = cos(phi);
      const double sp = sin(phi);
      const double cb = cos(beta);
      const double sb = sin(beta);

      // Elements of matrix dMat:

      dMat[0][0] =  cp;
      dMat[0][1] =  sp*cb;
      dMat[0][2] =  sp*sb;
      dMat[1][0] = -sp;
      dMat[1][1] =  cp*cb;
      dMat[1][2] =  cp*sb;
      dMat[2][0] =  0.0;
      dMat[2][1] = -sb;
      dMat[2][2] =  cb;

      // Elements of matrix "dinvMat"
      dinvMat[0][0] =  cp;
      dinvMat[0][1] = -sp;
      dinvMat[0][2] =  0.0;
      dinvMat[1][0] =  sp*cb;
      dinvMat[1][1] =  cp*cb;
      dinvMat[1][2] = -sb;
      dinvMat[2][0] =  sp*sb;
      dinvMat[2][1] =  cp*sb;
      dinvMat[2][2] =  cb;
*/

    }

    /**
     * mc_yvec function from tobyfit - generate a random point in up to 13 dimensional space
     */
    void SimulateResolution::mcYVec(const double ranvec[], const boost::shared_ptr<Mantid::MDAlgorithms::RunParam> run,
        const std::vector<double> & detectorBB, const double detTimeBin, std::vector<double> & yVec, double & eta2, double & eta3 ) const
    {
      //(irun, det_width, det_height, det_timebin, mc_type, y_vec, eta_2, eta_3)

      /*


            ! Calculates the elements of the vector "Y" defined on pg. 112 of T.Perring's thesis (1991):
            ! ------------------------------------------------------------------------------------------
            ! Input:
            !     ranvec            double[13] set of random/quasi random sample points on unit hypercube
            !     irun              run index
            !     detectorBB        detector width, heigh, depth    (m)
            !     det_timebin       time width of detector bin (s)
            !     mc_type           =0 if pseudo-random number generator; =1 if quasi-random (Sobol sequence)
            ! Output:
            !     y_vec(1)  = t_m   deviation in departure time from moderator surface
            !     y_vec(2)  = y_a   y-coordinate of neutron at apperture
            !     y_vec(3)  = z_a   z-coordinate of neutron at apperture
            !     y_vec(4)  = t_ch' deviation in time of arrival at chopper
            !     y_vec(5)  = x_s   x-coordinate of point of scattering in sample frame
            !     y_vec(6)  = y_s   y-coordinate of point of scattering in sample frame
            !     y_vec(7)  = z_s   z-coordinate of point of scattering in sample frame
            !     y_vec(8)  = x_d   x-coordinate of point of detection in detector frame
            !     y_vec(9)  = y_d   y-coordinate of point of detection in detector frame
            !     y_vec(10) = z_d   z-coordinate of point of detection in detector frame
            !     y_vec(11) = t_d   deviation in detection time of neutron
            !     eta_2          in-plane mosaic
            !     eta_3          out-of-plane mosaic
            !
            !
            ! 03-01-28   TGP   Changed chopper jitter distribution to be triangular: avoids call to GASDEV
            !
       */
      const size_t nelms=11;

      int imc(0);

      if(yVec.size()<nelms)
         yVec.resize(nelms);
      // Sample over moderator time distribution:
      if (m_mcOptVec[mcLineShape])
      {
        yVec[0] = run->moderatorDepartTime(ranvec[imc++]);
      }
      else
        yVec[0] = 0.0;

      // Sample over beam-defining aperture:
      if (m_mcOptVec[mcAperture])
      {
        run->getAperturePoint(ranvec[imc],ranvec[imc+1],yVec[1],yVec[2]);
        imc+=2;
      }
      else {
        yVec[1] = 0.0;
        yVec[2] = 0.0;
      }


      // Sample over chopper time distribution: Assume symmetric triangular distributions
      if (m_mcOptVec[mcChopper])
      {
        //yVec[4] = m_dt_chop_eff * tridev(ranvec[imc++]);
        yVec[3] = run->chopperTimeDist(ranvec[imc++]);
      }
      else
        yVec[3] = 0.0;

      if (m_mcOptVec[mcChopperJitter])
      {
        yVec[3] += + run->chopperJitter(ranvec[imc++]); //m_tjit_sig * rt6 * tridev(ranvec[imc++]);
      }

      // Sample over crystal volume:
      if (m_mcOptVec[mcSample])
      {
        // need to pass three ranvec values here and increment count
        // sample shape is implicitly used in the method
        run->getSamplePoint ( ranvec[imc], ranvec[imc+1], ranvec[imc+2], yVec[4], yVec[5], yVec[6]);
        imc += 3;
      }
      else {
        yVec[4] = 0.0;
        yVec[5] = 0.0;
        yVec[6] = 0.0;
      }


      // Sample over detector volume:
      if (m_mcOptVec[mcDetectorDepth])
      { // a rough approximation
        //yVec[8] = 0.6 * det_width * (ranvec(imc) - 0.5); // Introduces error if use false detector parameters e.g.HET rings
        yVec[7] = 0.6 * detectorBB[2]   * (ranvec[imc++] - 0.5);   // Assume detectors are 25mm diameter
      }
      else
        yVec[7] = 0.0;

      if (m_mcOptVec[mcDetectorArea])
      {
        yVec[8] = detectorBB[0] * (ranvec[imc++] - 0.5); // need to check BB, need width, height
        yVec[9] = detectorBB[1] * (ranvec[imc++] - 0.5);
      }
      else {
        yVec[8] = 0.0;
        yVec[9] = 0.0;
      }


      // Sample over detector time-bin:

      if (m_mcOptVec[mcDetectorTimeBin])
      {
        yVec[10] = detTimeBin * (ranvec[imc++] - 0.5);
      }
      else
        yVec[10] = 0.0;


      // Sample over crystal mosaic:

      if (m_mcOptVec[mcMosaic] )
      {
        run->getEta23(ranvec[imc],ranvec[imc+1], eta2, eta3 );
      }
      else {
        eta2 = 0.0;
        eta3 = 0.0;
      }
    }

    /**
     * Map from yVec to modified qE - null implementation for now
     * Vectors "y_vec" and "x_vec" are defined on pg. 112 of T.Perring's thesis (1991):
     * --------------------------------------------------------------------------------
     * Input:
     *       wi                      incident wavevector (Ang^-1)
     *       wf                      final wavevector (Ang^-1)
     *       q0                      nominal Q and energy in laboratory frame (Ang^-1, meV)
     *       b_mat                   matrix defined on pg. 112 of T.Perring's thesis (1991)
     *       dinv_mat                matrix to convery components of vector in detector coord frame to laboratory coord frame
     *       yVec(0)  = t_m         deviation in departure time from moderator surface
     *       yVec(1)  = y_a         y-coordinate of neutron at apperture
     *       yVec(2)  = z_a         z-coordinate of neutron at apperture
     *       yVec(3)  = t_ch'       deviation in time of arrival at chopper
     *       yVec(4)  = x_s         x-coordinate of point of scattering in sample frame
     *       yVec(5)  = y_s         y-coordinate of point of scattering in sample frame
     *       yVec(6)  = z_s         z-coordinate of point of scattering in sample frame
     *       yVec(7)  = x_d         x-coordinate of point of detection in detector frame
     *       yVec(8)  = y_d         y-coordinate of point of detection in detector frame
     *       yVec(9)  = z_d         z-coordinate of point of detection in detector frame
     *       yVec(10) = t_d         deviation in detection time of neutron
     *       eta2                   standard deviation for in-plane mosaic
     *       eta3                   standard deviation for out-of-plane mosaic
     *
     * Output:
     *       perturbQE(4)
     *
     */
    void SimulateResolution::mcMapYtoQEVec(const double wi, const double wf, const std::vector<double> & q0, const Kernel::Matrix<double> & bMat,
                               const Kernel::Matrix<double> & dInvMat, const std::vector<double> & yVec,
                               const double eta2, const double eta3,
                               std::vector<double> & perturbQE) const
    {

      std::vector<double> xVec(6);
      if(perturbQE.size()<4)
          perturbQE.resize(4);
      xVec = bMat * yVec; // About half of terms are zero, might be more efficient to explicit expressions
      std::vector<double> xVecTop,dq(3);
      xVecTop.push_back(xVec[3]);
      xVecTop.push_back(xVec[4]);
      xVecTop.push_back(xVec[5]);
      dq=dInvMat.Tprime()*xVecTop;
      perturbQE[0]=xVec[0]-dq[0];
      perturbQE[1]=xVec[1]-dq[1];
      perturbQE[2]=xVec[2]-dq[2];
      perturbQE[3]=(4.1442836 * ( wi*xVec[2] - wf*xVecTop[2] )); // beam components
      //TODO add eta terms here
      // Include crystal mosaic if required:
      if ((eta2 != 0.0) || (eta3 != 0.0))
      {
        std::vector<double> q(q0); q[0] += perturbQE[0]; q[1] += perturbQE[1]; q[2] += perturbQE[2];
        const double small=1e-10;
        double qmod = sqrt(q[0]*q[0]+q[1]*q[1]+q[2]*q[2]);
        if (qmod > small)
        {
          double qipmod = sqrt(q[0]*q[0] + q[1]*q[1]);
          if (qipmod > small)
          {
            perturbQE[2] -= qipmod*eta2;
            perturbQE[1] += ( (q[2]*q[1])*eta2 - (q[3]*qmod)*eta3 )/qipmod; // TODO check signs here in transpose from TF to MTF axes
            perturbQE[0] += ( (q[2]*q[0])*eta2 + (q[2]*qmod)*eta3 )/qipmod;
          }
          else
          {
            perturbQE[0] += qmod*eta2;
            perturbQE[1] += qmod*eta3;
          }
        }
      }


    }
    /**
     * sampleAreaTable function from tobyfit -
     */
    double SimulateResolution::sampleAreaTable(const double area ) const
    {
      int i = static_cast<int>((area*static_cast<double>(m_xtab.size())));
      double da = (area*static_cast<double>(m_xtab.size()-1)) - static_cast<double>(i);
      return m_xtab[i+1]*(1.0-da) + m_xtab[i+2]*da;

    }

    /**
     * rlatt function from tobyfit -
        RLATT calculates the matrix which is used to give the components
        of a vector in an orthonormal basis defined below, given the
        components in reciprocal lattice units (see Busing & Levy
        Acta Cryst vol 22 457-464 (1967))

        A       contains the three lattice spacings a,b,c (ang-1)
        a(i) > 0.0d0

        ANG     contains the lattice angles alpha, beta, gamma (deg)
        any value permitted

        ARLU     reciprocal lattice parameters (ang-1)

        ANGRLU   reciprocal lattice angles (deg)
        angrlu(i) chosen so that sign(sin(angrlu(i))) =
        sign(sin(ang(i)))

        BMAT     matrix that gives the components of a vector in the
        orthonormal basis [Ec(i)] given the vector in the
        basis [rlu]:
        if vc(i) are components in [Ec(i)] and v(i) those in
        [rlu] then
        vc(i) = BMAT (i,j) * v(j)

        The orthonormal axes are defined as
        Ec1  parallel to a*
        Ec2  in the plane of a* and b*, defined so that
        b* has a positive component along Ec2
        Ec3  forms a right hand coordinate set with Ec1
        and Ec2


        Entry
        -----
        A, ANG must be given
        The values of the elements of ARLU, ANGRLU, BMAT and IERR are
        irrelevant

        Exit
        ----
        A, ANG are unchanged
        ARLU, ANGRLU, BMAT filled if IERR=0; otherwise unchanged
     */
    int SimulateResolution::rlatt(const std::vector<double> & a, const std::vector<double> & ang,
        std::vector<double> & arlu, std::vector<double> & angrlu,
        Kernel::Matrix<double> & dMat )
    {
      UNUSED_ARG(ang);
      UNUSED_ARG(arlu);
      UNUSED_ARG(angrlu);
      UNUSED_ARG(dMat);
      double tol=1e-10;
      if( a[0]< tol || a[1]<tol || a[2]<tol )
        return 1;

      return 0;
    }

    /**
     * @return A list of attribute names
     */
    std::vector<std::string> SimulateResolution::getAttributeNames()const
    {
      std::vector<std::string> res;
      res.push_back("MCLoopMin");
      res.push_back("MCLoopMax");
      res.push_back("MCTol");
      return res;
    }

    /**
     * @param attName :: Attribute name. If it is not know exception is thrown.
     * @return a value of attribute attName
     */
    API::IFunction::Attribute SimulateResolution::getAttribute(const std::string& attName)const
    {
      if (attName == "MCLoopMin")
      {
        return Attribute(m_mcLoopMin);
      }
      else if (attName == "MCLoopMax")
      {
        return Attribute(m_mcLoopMax);
      }
      else if (attName == "MCTol")
      {
        return Attribute(m_mcTol);
      }
      throw std::invalid_argument("SimulateResolution: Unknown attribute " + attName);
    }

    /**
     * @param attName :: The attribute name. If it is not "n" exception is thrown.
     * @param att :: An int attribute containing the new value. The value cannot be negative.
     */
    void SimulateResolution::setAttribute(const std::string& attName,const API::IFunction::Attribute& att)
    {
      if (attName == "MCLoopMin")
      {
        int mcLoopMin = att.asInt();
        if ( mcLoopMin < 1)
        {
          throw std::invalid_argument("SimulateResolution: Must have MCLoopMin>=1.");
        }
        m_mcLoopMin = mcLoopMin;
      }
      else if (attName == "MCLoopMax")
      {
        int mcLoopMax = att.asInt();
        if ( mcLoopMax < 1)
        {
          throw std::invalid_argument("SimulateResolution: Must have MCLoopMax>=1.");
        }
        m_mcLoopMax = mcLoopMax;
      }
      else if (attName == "MCTol")
      {
        double mcTol = att.asDouble();
        if ( mcTol < 1.e-20 || mcTol > 1.0 )
        {
          throw std::invalid_argument("SimulateResolution: Must have 1e-20 < MCTol < 1");
        }
        m_mcTol = mcTol;
      }
    }

    /// Check if attribute attName exists
    bool SimulateResolution::hasAttribute(const std::string& attName)const
    {
      return attName == "MCLoopMin" || attName == "MCLoopMax" || attName == "MCTol";
    }

  } // namespace MDAlgorithms
} // namespace Mantid
