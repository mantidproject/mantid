//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidMDAlgorithms/CobaltSpinWaveDSHO.h"
#include <math.h>

#include "MantidGeometry/Tolerance.h"
#include "MantidGeometry/Math/mathSupport.h"
#include "MantidGeometry/Math/Matrix.h"

namespace Mantid
{
    namespace MDAlgorithms
    {

        using namespace Mantid::Kernel;
        using namespace Mantid::API;

        DECLARE_FUNCTION(CobaltSpinWaveDSHO)

        CobaltSpinWaveDSHO::CobaltSpinWaveDSHO()
        {
            declareParameter("Amplitude", 0.0);
            declareParameter("12SJ_AA", 0.0);
            declareParameter("12SJ_AB", 0.0);
            declareParameter("Gamma", 0.0);
        }

        //

        /**
        * sqw_broad model 601 from tobyfit
        */
        double CobaltSpinWaveDSHO::sqwBroad(const std::vector<double> & point, const std::vector<double> & fgParams,
            const double temp, const Geometry::Matrix<double> & ubinv) const
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
            const double var2=(1./3.)*sj1*fabs(cos(M_PI*ql))*(sqrt((alpha*alpha+beta*beta)));
            const double wacous = (sj1 + sj2*(1.0-var1)) - var2;
            const double woptic = (sj1 + sj2*(1.0-var1)) + var2;
            const double epswacous = (eps*eps-wacous*wacous);
            const double epswoptic = (eps*eps-woptic*woptic);
            const double formtab = formTable(qsqr);

            return( amp * bose(eps,temp) * formtab*formtab *
                ( sfa * (4.0*gam*wacous)/(M_PI*(epswacous*epswacous+4.*(gam*eps)*(gam*eps))) +
                sfo * (4.0*gam*woptic)/(M_PI*(epswoptic*epswoptic+4.*(gam*eps)*(gam*eps))) ) );
        }
        void CobaltSpinWaveDSHO::getParams() const
        {
            m_amplitude = getParameter("Amplitude");
            m_p12SJAA = getParameter("12SJ_AA");
            m_p12SJAB = getParameter("12SJ_AB");
            m_gamma = getParameter("Gamma");
        }
    }
}

