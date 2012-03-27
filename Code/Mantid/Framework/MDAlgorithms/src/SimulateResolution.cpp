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
      m_mcOptVec.resize(9,true);; // 9 MC effects are modelled in current implementation
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
    }

    SimulateResolution::~SimulateResolution()
    {
      delete m_randGen;
    }

    void SimulateResolution::setWorkspaceMD(WorkspaceGroup_sptr wsGroup)
    {
      m_mdWorkspaces = wsGroup;
    }

    double SimulateResolution::functionMD(Mantid::API::IMDIterator& it) const
    {
      getParams();

      double fgSignal = 0.;
      double fgError = 0.;
      // loop over each MDPoint in current MDBox
      for(size_t j=0; j < it.getNumEvents(); j++)
      {
        // Calculate convolution
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
      boost::shared_ptr<Mantid::MDAlgorithms::RunParam> runData = m_runData[runID];
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
      int mcLoopLimit=100,mcLoopMin=10; // test values
      double mcTol=1e-3;

      // MC loop over sample points until accuracy or loop limit reached
      if(mcLoopMin<1)
        mcLoopMin=1;
      if(mcLoopLimit<1)
        mcLoopLimit=1;

      const int sizeRanvec=13;
      boost::shared_array<double> ranvec( new double[sizeRanvec]);
      gsl_qrng *qRvec;
      if(m_random==sobol)
        qRvec =gsl_qrng_alloc(gsl_qrng_sobol, sizeRanvec);
      else
        throw std::invalid_argument("Invalid random method");

      for( int mcStep=0; mcStep<mcLoopLimit; mcStep++)
      {
        gsl_qrng_get( qRvec, ranvec.get() );
        mcYVec(ranvec.get() ,runData, yVec, eta2, eta3);
        mcMapYtoQEVec(yVec,qE,eta2,eta3,perturbQE);
        userSqw(params,perturbQE,result);
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
        if( (mcStep % mcLoopMin) ==0 || mcStep==mcLoopLimit-1 )
        {
          simSig=sum/mcStep;
          error=sqrt(fabs( ( sumSqr/mcStep - simSig*simSig ))/mcStep );

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
        Kernel::Matrix<double> & bMat)
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


      // Calculate velocities and times:
      // -------------------------------

      const double veli = 629.62237 * wi;
      const double velf = 629.62237 * wf;
      const double ti = x0/veli;
      const double tf = x2/velf;


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


      // Calculate the matrix elements:
      //-------------------------------

      bMat [0][0] =  cp_i;
      bMat [0][1] = -cp_i * gg1;
      bMat [0][2] =  0.0;
      bMat [0][3] = -cp_i;
      bMat [0][4] =  cp_i * gg2 * sMat[1][0];
      bMat [0][5] =  cp_i * gg2 * sMat[1][1];
      bMat [0][6] =  cp_i * gg2 * sMat[1][2];
      bMat [0][7] =  0.0;
      bMat [0][8] =  0.0;
      bMat [0][9]=  0.0;
      bMat [0][10]=  0.0;

      bMat [1][0] =  0.0;
      bMat [1][1] = -ct_i;
      bMat [1][2] =  0.0;
      bMat [1][3] =  0.0;
      bMat [1][4] =  ct_i * sMat[1][0];
      bMat [1][5] =  ct_i * sMat[1][1];
      bMat [1][6] =  ct_i * sMat[1][2];
      bMat [1][7] =  0.0;
      bMat [1][8] =  0.0;
      bMat [1][9]=  0.0;
      bMat [1][10]=  0.0;

      bMat [2][0] =  0.0;
      bMat [2][1] =  0.0;
      bMat [2][2] = -ct_i;
      bMat [2][3] =  0.0;
      bMat [2][4] =  ct_i * sMat[2][0];
      bMat [2][5] =  ct_i * sMat[2][1];
      bMat [2][6] =  ct_i * sMat[2][2];
      bMat [2][7] =  0.0;
      bMat [2][8] =  0.0;
      bMat [2][9]=  0.0;
      bMat [2][11]=  0.0;

      bMat [3][0] =  cp_f * (-x1/x0);
      bMat [3][1] =  cp_f *  ff1;
      bMat [3][2] =  0.0;
      bMat [3][3] =  cp_f * (x0+x1)/x0;
      bMat [3][4] =  cp_f * ( sMat[0][0]/veli - (dMat[0][0]*sMat[0][0]+dMat[0][1]*sMat[1][0]+dMat[0][2]*sMat[2][0])/velf
          - ff2*sMat[1][0] );
      bMat [3][5] =  cp_f * ( sMat[0][1]/veli - (dMat[0][0]*sMat[0][1]+dMat[0][1]*sMat[1][1]+dMat[0][2]*sMat[2][1])/velf
          - ff2*sMat[1][1] );
      bMat [3][6] =  cp_f * ( sMat[0][2]/veli - (dMat[0][0]*sMat[0][2]+dMat[0][1]*sMat[1][2]+dMat[0][2]*sMat[2][2])/velf
          - ff2*sMat[1][2] );
      bMat [3][7] =  cp_f/velf;
      bMat [3][8] =  0.0;
      bMat [3][9]=  0.0;
      bMat [3][10]= -cp_f;

      bMat [4][0] =  0.0;
      bMat [4][1] =  0.0;
      bMat [4][2] =  0.0;
      bMat [4][3] =  0.0;
      bMat [4][4] = -ct_f * ( dMat[1][0]*sMat[0][0] + dMat[1][1]*sMat[1][0] + dMat[1][2]*sMat[2][0] );
      bMat [4][5] = -ct_f * ( dMat[1][0]*sMat[0][1] + dMat[1][1]*sMat[1][1] + dMat[1][2]*sMat[2][1] );
      bMat [4][6] = -ct_f * ( dMat[1][0]*sMat[0][2] + dMat[1][1]*sMat[1][2] + dMat[1][2]*sMat[2][2] );
      bMat [4][7] =  0.0;
      bMat [4][8] =  ct_f;
      bMat [4][9]=  0.0;
      bMat [4][10]=  0.0;

      bMat [5][0] =  0.0;
      bMat [5][1] =  0.0;
      bMat [5][2] =  0.0;
      bMat [5][3] =  0.0;
      bMat [5][4] = -ct_f * ( dMat[2][0]*sMat[0][0] + dMat[2][1]*sMat[1][0] + dMat[2][2]*sMat[2][0] );
      bMat [5][5] = -ct_f * ( dMat[2][0]*sMat[0][1] + dMat[2][1]*sMat[1][1] + dMat[2][2]*sMat[2][1] );
      bMat [5][6] = -ct_f * ( dMat[2][0]*sMat[0][2] + dMat[2][1]*sMat[1][2] + dMat[2][2]*sMat[2][2] );
      bMat [5][7] =  0.0;
      bMat [5][8] =  0.0;
      bMat [5][9]=  ct_f;
      bMat [5][10]=  0.0;

    }

    /**
     * d_matrix function from tobyfit
     */

    void SimulateResolution::dMatrix(const double phi, const double beta, Kernel::Matrix<double> & dMat,
        Kernel::Matrix<double> & dinvMat)
    {

      /*
            ! Input:
            !     phi       scattering angle of centre of detector
            !     beta      azimuthal  angle of centre of detector
            ! Output:
            !     dMat    matrix for converting laboratory coordinates to detector coordinates
            !     dinv_mat corresponding inverse matrix
       */
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


    }

    /**
     * mc_yvec function from tobyfit - generate a random point in up to 13 dimensional space
     */
    void SimulateResolution::mcYVec(const double ranvec[], const boost::shared_ptr<Mantid::MDAlgorithms::RunParam> run,
        std::vector<double> & yVec, double & eta2, double & eta3 ) const
    {
      //(irun, det_width, det_height, det_timebin, mc_type, y_vec, eta_2, eta_3)

      /*


            ! Calculates the elements of the vector "Y" defined on pg. 112 of T.Perring's thesis (1991):
            ! ------------------------------------------------------------------------------------------
            ! Input:
            !     ranvec            double[13] set of random/quasi random sample points on unit hypercube
            !     irun              run index
            !     det_width         detector width    (m)
            !     det_height        detector height    (m)
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

      int imc(0);

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
        yVec[7] = 0.6 * 0.025   * (ranvec[imc++] - 0.5);   // Assume detectors are 25mm diameter
      }
      else
        yVec[7] = 0.0;

      if (m_mcOptVec[mcDetectorArea])
      {
        yVec[8]  = m_detectorBB[0] * (ranvec[imc++] - 0.5); // need to check BB, need width, height
        yVec[9] = m_detectorBB[2] * (ranvec[imc++] - 0.5);
      }
      else {
        yVec[8]  = 0.0;
        yVec[9] = 0.0;
      }


      // Sample over detector time-bin:

      if (m_mcOptVec[mcDetectorTimeBin])
      {
        yVec[10] = m_detectorTimebin * (ranvec[imc++] - 0.5);
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
     */
    void SimulateResolution::mcMapYtoQEVec(const std::vector<double> & yVec,const std::vector<double> & qE,const double eta2, const double eta3,
        std::vector<double> & perturbQE) const
    {
      UNUSED_ARG(yVec); UNUSED_ARG(qE); UNUSED_ARG(eta2); UNUSED_ARG(eta3);
      perturbQE.push_back(0.0);
      perturbQE.push_back(0.0);
      perturbQE.push_back(0.0);
      perturbQE.push_back(0.0);
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

  } // namespace MDAlgorithms
} // namespace Mantid
