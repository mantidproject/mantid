#ifndef MANTID_MDALGORITHMS_COBALTSPINWAVEDSHO_H_
#define MANTID_MDALGORITHMS_COBALTSPINWAVEDSHO_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidMDAlgorithms/SimulateResolution.h"

namespace Mantid
{
    namespace MDAlgorithms
    {
        /**
        Foreground simulation using a Cobalt Spin Wave model with DSHO damping
        Works out the cobalt acoustic and optic magnon dispersion relations given the momentum transfer
        in r.l.u. (note that Q can be unreduced or reduced) and the exchange constants SJ1, SJ2 (i.e.
        12*S*J1 and 12*S*J2) for nearest neighbour interaction for A-A and A-B respectively. Also
        calculates the optic and acoustic magnon structure factors.
 
        Required properties:
        <UL>
        <LI> Amplitude - scaling factor </LI>
        <LI> 12SJ_AA   - Exchange constant 12SJ for A-A </LI>
        <LI> 12SJ_AB   - Exchange constant 12SJ for A-B </LI>
        <LI> Gamma     - inverse lifetime gamma (= energy half-width) </LI>
        </UL>

        @author Ron Fowler
        @date 07/04/2011

        Copyright &copy; 2007-11 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

        This file is part of Mantid.

        Mantid is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation; either version 3 of the License, or
        (at your option) any later version.

        Mantid is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program.  If not, see <http://www.gnu.org/licenses/>.

        File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
        Code Documentation is available at: <http://doxygen.mantidproject.org>
        */
        class DLLExport CobaltSpinWaveDSHO : public MDAlgorithms::SimulateResolution
        {
        public:
            /// Constructor
            CobaltSpinWaveDSHO();
            /// Destructor
            virtual ~CobaltSpinWaveDSHO() {}

            /// overwrite IFunction base class methods
            std::string name()const{return "CobaltSpinWaveDSHO";}
            /// sqwBroad returns the expected scattering weight for the given Q,w point
            virtual double sqwBroad(const std::vector<double> & point, const std::vector<double> & fgParams,
                    const double temp, const Geometry::Matrix<double> & ubinv) const;
           
        protected:
            /// load parameter values into local memory
            virtual void getParams() const;
        private:
            /// model amplitude
            mutable double m_amplitude;
            mutable double m_p12SJAA;
            mutable double m_p12SJAB;
            mutable double m_gamma;
        };

    } // namespace MDAlgorithms
} // namespace Mantid

#endif /*MANTID_MDALGORITHMS_COBALTSPINWAVEDSHO_H_*/
