//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidMDAlgorithms/TobyFitSimulate.h"
#include <math.h>

// @todo: This needs a factory (copied from MC Absorbtion algo)
#include "MantidKernel/MersenneTwister.h"

#include <gsl/gsl_errno.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_roots.h>

#include "MantidGeometry/Tolerance.h"
#include "MantidGeometry/Math/mathSupport.h"
#include "MantidGeometry/Math/Matrix.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid
{
    namespace MDAlgorithms
    {
        // Constructor
        TobyFitSimulate::TobyFitSimulate() : m_sobol(false), m_randSeed(12345678),m_randGen(NULL)
        {
        }

        TobyFitSimulate::~TobyFitSimulate()
        {
            delete m_randGen;
        }

        // simple test interface for fg model - will need vector of parameters
        void TobyFitSimulate::SimForeground(boost::shared_ptr<Mantid::API::IMDWorkspace> imdw,
              std::string fgmodel,const double fgparaP1, const double fgparaP2, const double fgparaP3)
        {
            imdwCut=imdw;
            initRandomNumbers();
            // currently maximum of 13 dimensions required
            std::vector<double> point;
            for(size_t i=0;i<13;i++)
                point.push_back(0.0);
            int ncell= imdwCut->getXDimension()->getNBins();
            // loop over MDCells in this cut
            for(int i=0; i<ncell ; i++ ){
                const Mantid::Geometry::SignalAggregate& newCell = imdwCut->getCell(i); // get cell data
                std::vector<boost::shared_ptr<Mantid::Geometry::MDPoint> > cellPoints = newCell.getContributingPoints();
                double answer=0.,error=0.;
                // loop over each MDPoint in current MDCell
                for(size_t j=0; j<cellPoints.size(); j++){
                    std::vector<Mantid::Geometry::coordinate> vertexes = cellPoints[j]->getVertexes();
                    double eps=vertexes[0].gett();
                    sqwConvolution(cellPoints[j],answer,error);
                }
                //pnt->setSignal(bgsum);
            }
        }


        // SQW convolution choice - currently just MonteCarlo
        void TobyFitSimulate::sqwConvolution(boost::shared_ptr<Mantid::Geometry::MDPoint> & point,
              double answer, double error) {
            sqwConvolutionMC(point,answer,error);
        }

        // SQW convolution MonteCarlo
        void TobyFitSimulate::sqwConvolutionMC(boost::shared_ptr<Mantid::Geometry::MDPoint> & point,
              double answer, double error) {
            
        }

        // Return next pseudo or quasi random point in the N dimensional space
        void TobyFitSimulate::getNextPoint(std::vector<double>& point, int count)
        {
            if(!m_sobol) {
                for(int i=0;i<count;i++)
                    point[i]=m_randGen->next();
            }
        }

        /**
        * Initialise the random number generator
        */
        void TobyFitSimulate::initRandomNumbers()
        {
            if( !m_randGen )
            {
                m_randGen = new Kernel::MersenneTwister;
                m_randGen->setSeed(m_randSeed);
            }
        }   

        /**
        * Reset the random number generator
        */
        void TobyFitSimulate::resetRandomNumbers()
        {
            if( m_randGen )
            {
                m_randGen->setSeed(m_randSeed);
            }
        }

        /**
        * sqw_broad model 601 from tobyfit
        */
        double TobyFitSimulate::sqwBroad601(const std::vector<double> & point, const std::vector<double> & fgParams,
            const double temp, const Geometry::Matrix<double> & ubinv)
        {
            double qx = point[0]; double qy = point[1]; double qz = point[2]; double eps = point[3];
            double qsqr = qx*qx+qy*qy+qz*qz;
            // Get Q in r.l.u.:
            double qh = ubinv[1][1]*qx + ubinv[1][2]*qy + ubinv[1][3]*qz;
            double qk = ubinv[2][1]*qx + ubinv[2][2]*qy + ubinv[2][3]*qz;
            double ql = ubinv[3][1]*qx + ubinv[3][2]*qy + ubinv[3][3]*qz;

            const double a1 = (2.*M_PI/3.);
            const double a2 = 2.*a1;

            const double amp = fgParams[0];
            const double sj1 = fgParams[1];
            const double sj2 = fgParams[2];
            const double gam = fgParams[6];
            double dint;

            const double alpha =cos(((a2*qh)+(a1*qk))) + cos(((a1*qk)-(a1*qh))) + cos((-(a1*qh)-(a2*qk)));
            const double beta  =sin(((a2*qh)+(a1*qk))) + sin(((a1*qk)-(a1*qh))) + sin((-(a1*qh)-(a2*qk)));
            const double sfa = 0.5+( (modf(ql+1.5,&dint)-1.0>0)?(0.5):(-0.5) ) * alpha/(sqrt((alpha*alpha+beta*beta)));
            const double sfo = 1.0-sfa;
            const double var1=(1./6.)*(alpha*alpha+beta*beta)-0.5;
            const double var2=(1./3.)*sj1*abs(cos(M_PI*ql))*(sqrt((alpha*alpha+beta*beta)));
            const double wacous = (sj1 + sj2*(1.0-var1)) - var2;
            const double woptic = (sj1 + sj2*(1.0-var1)) + var2;
            const double epswacous = (eps*eps-wacous*wacous);
            const double epswoptic = (eps*eps-woptic*woptic);
            const double formtab = formTable(qsqr);

            return( amp * bose(eps,temp) * formtab*formtab *
                ( sfa * (4.0*gam*wacous)/(M_PI*(epswacous*epswacous+4.*(gam*eps)*(gam*eps))) +
                sfo * (4.0*gam*woptic)/(M_PI*(epswoptic*epswoptic+4.*(gam*eps)*(gam*eps))) ) );
        }

        /**
        * bose function from tobyfit
        */
        double TobyFitSimulate::bose(const double eps, const double temp)
        {
            double kB = 0.08617347;
            if( temp<0. )
                return( (eps>=0)?(eps):(0.) );
            else
                return( (kB*temp)*pop(eps/(kB*temp)) );
        }

        /**
        * formTable function from tobyfit - not yet implemented
        */
        double TobyFitSimulate::formTable(const double qsqr)
        {
            return 0.0; //to do - implement this look up table
        }
        /**
        * gausdev function from tobyfit
        */
        void TobyFitSimulate::gasdev2d(const double ran1, const double ran2, double & gaus1, double & gaus2)
        {
             const double fac=sqrt(-2.*log(std::max(ran1,1e-20)));
             gaus1=fac*cos(2.*M_PI*ran2);
             gaus2=fac*sin(2.*M_PI*ran2);
        }
        /**
        * pop function from tobyfit
        */
        double TobyFitSimulate::pop(double y)
        {
            double by2=0.5, by6=1./6., by60=1./60., by42=1./42., by40=1./40.;
            if ( (abs(y) > 0.1 ) )
            {
                double ans = (abs(y)) / (1.0 - (exp(-(abs(y)))));
                return( (y<0)? ( ans*(exp(-abs(y)))) : (ans) );
            }
            else
                return( 1.0 + by2*y*( 1.0 + by6*y*( 1.0 - by60*(y*y)
                *(1.0-by42*(y*y)*(1.0-by40*(y*y) )))) );
        }
        /**
        * tridev function from tobyfit
        */
        double TobyFitSimulate::tridev(double a)
        {
            return( (a>0.5)? (1.0-sqrt(abs(1.0-2.0*abs(a-0.5)))):(sqrt(abs(1.0-2.0*abs(a-0.5))-1.0)) );
        }
        /**
        * monte_carlo_sample_volume function from tobyfit
        * note that sample bounding box dimensions assume that sample is axis aligned cuboid
        * if not the case then m_mcOptVec[m_mcSampleVolme] should be false.
        * The sample bound box is stored globally to make references moe efficient.
        */
        void TobyFitSimulate::monteCarloSampleVolume( const double ran1 , const double ran2 , const double ran3,
            double & dx , double & dy, double & dz )
        {
            dx = m_sampleBB[0] * (ran1-0.5);
            dy = m_sampleBB[1] * (ran2-0.5);
            dz = m_sampleBB[2] * (ran3-0.5);
        }

        /**
        * bMatrix function to determine the elements of matrix bMat - TGP thesis 1991 p112
        */
        void TobyFitSimulate::bMatrix(const double wi, const double wf, const double x0, const double xa, const double x1,
            const double x2, const double thetam, const double angvel,
            const Geometry::Matrix<double> & sMat, const Geometry::Matrix<double> & dMat,
            Geometry::Matrix<double> & bMat)
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

            bMat [1][1] =  cp_i;
            bMat [1][2] = -cp_i * gg1;
            bMat [1][3] =  0.0;
            bMat [1][4] = -cp_i;
            bMat [1][5] =  cp_i * gg2 * sMat[2][1];
            bMat [1][6] =  cp_i * gg2 * sMat[2][2];
            bMat [1][7] =  cp_i * gg2 * sMat[2][3];
            bMat [1][8] =  0.0;
            bMat [1][9] =  0.0;
            bMat [1][10]=  0.0;
            bMat [1][11]=  0.0;

            bMat [2][1] =  0.0;
            bMat [2][2] = -ct_i;
            bMat [2][3] =  0.0;
            bMat [2][4] =  0.0;
            bMat [2][5] =  ct_i * sMat[2][1];
            bMat [2][6] =  ct_i * sMat[2][2];
            bMat [2][7] =  ct_i * sMat[2][3];
            bMat [2][8] =  0.0;
            bMat [2][9] =  0.0;
            bMat [2][10]=  0.0;
            bMat [2][11]=  0.0;

            bMat [3][1] =  0.0;
            bMat [3][2] =  0.0;
            bMat [3][3] = -ct_i;
            bMat [3][4] =  0.0;
            bMat [3][5] =  ct_i * sMat[3][1];
            bMat [3][6] =  ct_i * sMat[3][2];
            bMat [3][7] =  ct_i * sMat[3][3];
            bMat [3][8] =  0.0;
            bMat [3][9] =  0.0;
            bMat [3][10]=  0.0;
            bMat [3][11]=  0.0;

            bMat [4][1] =  cp_f * (-x1/x0);
            bMat [4][2] =  cp_f *  ff1;
            bMat [4][3] =  0.0;
            bMat [4][4] =  cp_f * (x0+x1)/x0;
            bMat [4][5] =  cp_f * ( sMat[1][1]/veli - (dMat[1][1]*sMat[1][1]+dMat[1][2]*sMat[2][1]+dMat[1][3]*sMat[3][1])/velf
                - ff2*sMat[2][1] );
            bMat [4][6] =  cp_f * ( sMat[1][2]/veli - (dMat[1][1]*sMat[1][2]+dMat[1][2]*sMat[2][2]+dMat[1][3]*sMat[3][2])/velf
                - ff2*sMat[2][2] );
            bMat [4][7] =  cp_f * ( sMat[1][3]/veli - (dMat[1][1]*sMat[1][3]+dMat[1][2]*sMat[2][3]+dMat[1][3]*sMat[3][3])/velf
                - ff2*sMat[2][3] );
            bMat [4][8] =  cp_f/velf;
            bMat [4][9] =  0.0;
            bMat [4][10]=  0.0;
            bMat [4][11]= -cp_f;

            bMat [5][1] =  0.0;
            bMat [5][2] =  0.0;
            bMat [5][3] =  0.0;
            bMat [5][4] =  0.0;
            bMat [5][5] = -ct_f * ( dMat[2][1]*sMat[1][1] + dMat[2][2]*sMat[2][1] + dMat[2][3]*sMat[3][1] );
            bMat [5][6] = -ct_f * ( dMat[2][1]*sMat[1][2] + dMat[2][2]*sMat[2][2] + dMat[2][3]*sMat[3][2] );
            bMat [5][7] = -ct_f * ( dMat[2][1]*sMat[1][3] + dMat[2][2]*sMat[2][3] + dMat[2][3]*sMat[3][3] );
            bMat [5][8] =  0.0;
            bMat [5][9] =  ct_f;
            bMat [5][10]=  0.0;
            bMat [5][11]=  0.0;

            bMat [6][1] =  0.0;
            bMat [6][2] =  0.0;
            bMat [6][3] =  0.0;
            bMat [6][4] =  0.0;
            bMat [6][5] = -ct_f * ( dMat[3][1]*sMat[1][1] + dMat[3][2]*sMat[2][1] + dMat[3][3]*sMat[3][1] );
            bMat [6][6] = -ct_f * ( dMat[3][1]*sMat[1][2] + dMat[3][2]*sMat[2][2] + dMat[3][3]*sMat[3][2] );
            bMat [6][7] = -ct_f * ( dMat[3][1]*sMat[1][3] + dMat[3][2]*sMat[2][3] + dMat[3][3]*sMat[3][3] );
            bMat [6][8] =  0.0;
            bMat [6][9] =  0.0;
            bMat [6][10]=  ct_f;
            bMat [6][11]=  0.0;

        }

        /**
        * d_matrix function from tobyfit
        */

        void TobyFitSimulate::dMatrix(const double phi, const double beta, Geometry::Matrix<double> & dMat,
             Geometry::Matrix<double> & dinvMat)
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

            dMat[1][1] =  cp;
            dMat[1][2] =  sp*cb;
            dMat[1][3] =  sp*sb;
            dMat[2][1] = -sp;
            dMat[2][2] =  cp*cb;
            dMat[2][3] =  cp*sb;
            dMat[3][1] =  0.0;
            dMat[3][2] = -sb;
            dMat[3][3] =  cb;

            // Elements of matrix "dinvMat":
            dinvMat[1][1] =  cp;
            dinvMat[1][2] = -sp;
            dinvMat[1][3] =  0.0;
            dinvMat[2][1] =  sp*cb;
            dinvMat[2][2] =  cp*cb;
            dinvMat[2][3] = -sb;
            dinvMat[3][1] =  sp*sb;
            dinvMat[3][2] =  cp*sb;
            dinvMat[3][3] =  cb;

        }

        /**
        * pop function from tobyfit
        */
        double TobyFitSimulate::enResModChop(const double ei, const double eps, const double x0, const double x1,
            const double x2, const double taumod, const double tauchp )
        {
            /* subroutine energy_resolution_mod_chop (ei, eps, x0, x1, x2, taumod, tauchp, eps_sig, ierr)
            integer ierr
            double precision ei, eps, x0, x1, x2, taumod, tauchp, eps_sig
            !
            !  Simple minded energy resolution that includes only those components from the moderator and chopper
            ! Input:
            !     ei    incident energy (meV)
            !     eps   energy transfer (meV)
            !     x0    moderator-chopper distance (m)
            !     x1    chopper-sample distance    (m)
            !     x2    sample-detector distance   (m)
            !     taumod   standard deviation of moderator pulse width (s)
            !     tauchp   standard deviation of chopper pulse width (s)
            !
            ! Output:
            !     eps_sig standard deviation of energy resolution (meV)
            !
            */
            if (ei <= 0.0 || (ei-eps) <= 0.0)
                ; // should throw exception
            else
            {
                const double wi = sqrt(ei/2.0721418);
                const double wf = sqrt((ei-eps)/2.0721418);
                const double veli = 629.62237 * wi;
                const double ti = x0/veli;
                const double temp = pow( ((taumod/ti)*(1.0+(x1/x2)*pow((wf/wi),3))), 2 ) +
                    pow( ((tauchp/ti)*(1.0+((x0+x1)/x2)*pow((wf/wi),3))) , 2 );
                return( 2.0*ei*sqrt(temp) );
            }
            return(0.);
        }

        /**
        * mc_yvec function from tobyfit - generate a random point in up to 13 dimensional space
        */
        void TobyFitSimulate::mcYVec(const double detWidth, const double detHeight, const double detTimeBin,
            std::vector<double> & yVec, double & eta2, double & eta3 )
        {
            //(irun, det_width, det_height, det_timebin, mc_type, y_vec, eta_2, eta_3)

            /*


            ! Calculates the elements of the vector "Y" defined on pg. 112 of T.Perring's thesis (1991):
            ! ------------------------------------------------------------------------------------------
            ! Input:
            !     irun           run index
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

            ! Get vector of random deviates:
            ! ------------------------------
            */
            const double rt6 = 2.449489742783178098; // sqrt(6)
            int imc(0);

            // this may need to be optimised
            std::vector<double> ranvec;
            getNextPoint(ranvec,13);
            // Sample over moderator time distribution:
            if (m_mcOptVec[mcLineShape]) {
                //if (imoderator(irun) .eq. 1) then ! Ikeda-Carpenter model
                double xtemp1 = sampleAreaTable (ranvec[imc++]);

                const double xtemp = std::min (0.999, xtemp1);
                yVec[1] = 1.0e-6 * m_t_mod_av * (xtemp/(1.0-xtemp) - 1.0);
            }
            else
                yVec[1] = 0.0;

            // Sample over beam-defining aperture:
            if (m_mcOptVec[mcAperture]) {
                yVec[2] = m_wa * (ranvec[imc++] - 0.5);
                yVec[3] = m_ha * (ranvec[imc++] - 0.5);
            }
            else {
                yVec[2] = 0.0;
                yVec[3] = 0.0;
            }


            // Sample over chopper time distribution: Assume symmetric triangular distributions
            if (m_mcOptVec[mcChopper]) {
                yVec[4] = m_dt_chop_eff * tridev(ranvec[imc++]);
            }
            else
                yVec[4] = 0.0;

            if (m_mcOptVec[mcChopperJitter]) {
                yVec[4] += + m_tjit_sig * rt6 * tridev(ranvec[imc++]);
            }

            // Sample over crystal volume:
            if (m_mcOptVec[mcSample]) {
                // need to pass three ranvec values here and increment count
                // sample shape is implicitly used in the method
                monteCarloSampleVolume ( ranvec[imc], ranvec[imc+1], ranvec[imc+2], yVec[5], yVec[6], yVec[7]);
                imc += 3;
            }
            else {
                yVec[5] = 0.0;
                yVec[6] = 0.0;
                yVec[7] = 0.0;
            }


            // Sample over detector volume:
            if (m_mcOptVec[mcDetectorDepth]) { // a rough approximation
                //yVec[8] = 0.6 * det_width * (ranvec(imc) - 0.5); // Introduces error if use false detector parameters e.g.HET rings
                yVec[8] = 0.6 * 0.025   * (ranvec[imc++] - 0.5);   // Assume detectors are 25mm diameter
            }
            else
                yVec[8] = 0.0;

            if (m_mcOptVec[mcDetectorArea]) {
                yVec[9]  = m_detectorBB[0] * (ranvec[imc++] - 0.5); // need to check BB, need width, height
                yVec[10] = m_detectorBB[2] * (ranvec[imc++] - 0.5);
            }
            else {
                yVec[9]  = 0.0;
                yVec[10] = 0.0;
            }


            // Sample over detector time-bin:

            if (m_mcOptVec[mcDetectorTimeBin]) {
                yVec[11] = m_detectorTimebin * (ranvec[imc++] - 0.5);
            }
            else
                yVec[11] = 0.0;


            // Sample over crystal mosaic:

            if (m_mcOptVec[mcMosaic] ) {
                gasdev2d(ranvec[imc++], ranvec[imc++], eta2, eta3);
                eta2 = m_eta_sig * eta2;
                eta3 = m_eta_sig * eta3;
            }
            else {
                eta2 = 0.0;
                eta3 = 0.0;
            }
        }
      /**
        * sampleAreaTable function from tobyfit - 
        */
        double TobyFitSimulate::sampleAreaTable(const double area )
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
        int TobyFitSimulate::rlatt(const std::vector<double> & a, const std::vector<double> & ang,
                 std::vector<double> & arlu, std::vector<double> & angrlu,
                 Geometry::Matrix<double> & dMat )
        {
            int i;
	    double tol=1e-10;
	    if( a[0]< tol || a[1]<tol || a[2]<tol )
		    return 1;

	    return 0;
        }

    } // namespace Algorithms
} // namespace Mantid
