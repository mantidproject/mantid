//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidMDAlgorithms/CobaltSpinWaveDSHO.h"
#include <math.h>

#include "MantidKernel/Tolerance.h"
#include "MantidGeometry/Math/mathSupport.h"
#include "MantidKernel/Matrix.h"

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

        CobaltSpinWaveDSHO::CobaltSpinWaveDSHO(std::vector<std::string> extraParams)
        {
        	declareParameter("Amplitude", 0.0);
        	declareParameter("12SJ_AA", 0.0);
        	declareParameter("12SJ_AB", 0.0);
        	declareParameter("Gamma", 0.0);
        	for(size_t i=0;i<extraParams.size();i++)
        		declareParameter(extraParams[i],0.0);
        }

        //

        // This is a broad model
        bool CobaltSpinWaveDSHO::userModelIsBroad() const
        {return true;};
        //
        /**
        * sqw_broad model 601 from tobyfit
        */
        void CobaltSpinWaveDSHO::userSqw(const std::vector<double> & params, const std::vector<double> & qE, std::vector<double> & result) const
        //double CobaltSpinWaveDSHO::sqwBroad(const std::vector<double> & point, const std::vector<double> & fgParams,
        //    const double temp, const Kernel::Matrix<double> & ubinv) const
        {

            const double eps = qE[3];
            Mantid::Kernel::V3D qlab=Mantid::Kernel::V3D(qE[0],qE[1],qE[2]);
            const double qsqr = qlab.norm2();
            const double temp=0.0; //TODO set properly
            // Get Q in r.l.u. using the uBInv matrix for this run:
            //Mantid::Kernel::V3D qrlu = m_runData->uBInv()*qlab;
            const double qh = qlab[0]; //TODO Fix this by using correct transform from lab to hkl
            const double qk = qlab[1];
            const double ql = qlab[2];

            const double a1 = (2.*M_PI/3.);
            const double a2 = 2.*a1;

            const double amp = params[0];
            const double sj1 = params[1];
            const double sj2 = params[2];
            const double gam = params[3];

            const double alpha =cos(((a2*qh)+(a1*qk))) + cos(((a1*qk)-(a1*qh))) + cos((-(a1*qh)-(a2*qk)));
            const double beta  =sin(((a2*qh)+(a1*qk))) + sin(((a1*qk)-(a1*qh))) + sin((-(a1*qh)-(a2*qk)));
            double sfa;
            if(fabs(ql)<0.5)
            {
              sfa = 0.5+0.5 * alpha/(sqrt((alpha*alpha+beta*beta)));
            }
            else
            {
              sfa = 0.5-0.5 * alpha/(sqrt((alpha*alpha+beta*beta)));
            }
            const double sfo = 1.0-sfa;
            const double var1=(1./6.)*(alpha*alpha+beta*beta)-0.5;
            const double var2=(1./3.)*sj1*fabs(cos(M_PI*ql))*(sqrt((alpha*alpha+beta*beta)));
            const double wacous = (sj1 + sj2*(1.0-var1)) - var2;
            const double woptic = (sj1 + sj2*(1.0-var1)) + var2;
            const double epswacous = (eps*eps-wacous*wacous);
            const double epswoptic = (eps*eps-woptic*woptic);
            const double formtab = magneticForm(qsqr);
            //UNUSED_ARG(qsqr);

            const double weight =  amp * bose(eps,temp) * formtab*formtab *
                ( sfa * (4.0*gam*wacous)/(M_PI*(epswacous*epswacous+4.*(gam*eps)*(gam*eps))) +
                sfo * (4.0*gam*woptic)/(M_PI*(epswoptic*epswoptic+4.*(gam*eps)*(gam*eps))) );
            result.push_back(weight);
        }
        // read parameter values - don't call every time
        void CobaltSpinWaveDSHO::getParams() const
        {
            m_amplitude = getParameter("Amplitude");
            m_p12SJAA = getParameter("12SJ_AA");
            m_p12SJAB = getParameter("12SJ_AB");
            m_gamma = getParameter("Gamma");
        }
    }
}

