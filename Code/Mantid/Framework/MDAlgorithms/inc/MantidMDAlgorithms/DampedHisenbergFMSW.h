#ifndef MANTID_MDALGORITHMS_DAMPEDHISENBERGFMSW_H_
#define MANTID_MDALGORITHMS_DAMPEDHISENBERGFMSW_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidMDAlgorithms/SimulateResolution.h"

namespace Mantid
{
  namespace MDAlgorithms
  {
    /**
        Damped Heisenberg ferromagnetic spin waves in simple cubic lattice
        ==================================================================
        Model has four flavours:
           (1) DSHO, uniform damping (TF model 111)
           (2) DSHO, Lovesey damping
           (3) Lorentzian, uniform damping
           (4) Lorentzian, Lovesey damping
         The attributes Type=DSHO|Lorentzian and Damping=Uniform|Lovesey will control this, initial just 111.

        Dispersion:
              Amp     intensity scale
              Gap     gap
              JS1     JS for nearest neighbour exchange
              JS2     JS for next nearest neighbour exchange
              JS3     JS for third neasrest neighbour exchange

        Damping:

          if uniform i.e. Q independent damping (icross=111 or 121):
              Gamma   inverse lifetime gamma (= energy half-width)

          else if Lovesey model for Q-dependent damping (interpolating between eqns 9.89 and 9.90
               in Lovesey vol.II):

              Gamma   inverse lifetime gamma_0 (= energy half-width)
              Spin    spin (i.e. 1/2, 1, 3/2, ...)
              Damp    damping pre-scale (fix to some power of 10, needed for p(9) to be O(1))

        Required properties:
        <UL>
        <LI> Amp       - scaling factor </LI>
        <LI> Gap       - gap </LI>
        <LI> JS1       - Exchange constant nearest neighbour </LI>
        <LI> JS2       - Exchange constant next nearest neighbour </LI>
        <LI> JS3       - Exchange constant third nearest neighbour </LI>
        <LI> Gamma     - inverse lifetime gamma (= energy half-width) </LI>
        <LI> Spin      - Spin for Lovesey only </LI>
        <LI> Damp      - Damping pre-scale for Lovesey only </LI>
        </UL>

        Copyright &copy; 2007-12 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport DampedHisenbergFMSW : public MDAlgorithms::SimulateResolution
    {
    public:
      /// Constructors
      DampedHisenbergFMSW();
      DampedHisenbergFMSW(std::vector<std::string> extraParams);
      /// Destructor
      virtual ~DampedHisenbergFMSW() {}

      /// overwrite IFunction base class methods
      std::string name()const{return "DampedHisenbergFMSW";}
      /// This function only for use in inelastic analysis
      virtual const std::string category() const { return "Inelastic";}

    protected:
      /// check if model is broad or sharp
      bool userModelIsBroad() const;
      /// function that returns expected scatter for given point using the defined model with parameters params
      /// and run parameters.
      void userSqw(const boost::shared_ptr<Mantid::MDAlgorithms::RunParam> run, const std::vector<double> & params,
          const std::vector<double> & qE, std::vector<double> & result) const;
      /// load parameter values into local memory
      void getParams(std::vector<double> & params) const;
      /// Lovesey damping
      double gamFM (const double e, const double t, const double s) const;
    private:
      /// model amplitude
      mutable double m_amp;
      mutable double m_gap;
      mutable double m_js1;
      mutable double m_js2;
      mutable double m_js3;
      mutable double m_gamma;
      mutable double m_spin;
      mutable double m_damp;
      // model options
      bool m_lorentzian;
      bool m_lovesey;
    };

  } // namespace MDAlgorithms
} // namespace Mantid

#endif /*MANTID_MDALGORITHMS_DAMPEDHISENBERGFMSW_H_*/
