#ifndef MANTID_ALGORITHMS_MODERATORTZERO_H_
#define MANTID_ALGORITHMS_MODERATORTZERO_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidGeometry/muParser_Silent.h"

namespace Mantid
{
namespace Algorithms
{
/* Corrects the time of flight (TOF) by a time offset that is dependent on the energy of the neutron after passing through the moderator.
  A heuristic formula for the correction is stored in the instrument definition file. Below is shown the entry in the instrument file for the VISION beamline:
  <!--  formula for t0 calculation. See http://muparser.sourceforge.net/mup_features.html#idDef2 for available operators-->
  <parameter name="t0_formula" type="string">
   <value val="34.746 - 0.166672*incidentEnergy + 0.00020538*incidentEnergy^(2.0)" />
  </parameter>

  The recorded TOF = t_0 + t_i + t_f with
  t_0: emission time from the moderator
  t_1: time from moderator to sample
  t_2: time from sample to detector
  This algorithm will replace TOF with TOF' = TOF-t_0 = t_i+t_f

  For a direct geometry instrument, the incidente energy E_1 is the same for all neutrons. Hence, the moderator emission time is the same for all neutrons.
  For an indirect geometry instrument, E_1 is different for each neutron and is not known. However, the final energy E_2 selected by the analyzers is known.
  t_0 = func(E_1) , a function of the incident energy
  t_1 = L_1/v_1 with L_1 the distance from moderator to sample, and v_1 the initial unknown velocity ( E_1=1/2*m*v_1^2)
  t_2 = L_2/v_2 with L_2 the distance from sample to detector, and v_2 is the final fixed velocity ( E_2=1/2*m*v_2^2)

  We obtain TOF' in an iterative process, taking into account the fact that the correction t_0 is much smaller than t_i+t_f. Thus
  TOF-t_0^(n) = L_1/v_1^(n) + L_2/v_2 , n=0, 1, 2,..
  Set t_0^(0)=0 and obtain v_1^(0) from the previous formula. From v_1^(0) we obtain E_1^(0)
  Set t_0^(1)=func( E_1^(0) ) and repeat the steps until |t_0^(n+1) - t_0^(n+1)| < 1 microsec. Typically, three to four iterations are needed for convergence.

  @author Jose Borreguero
  @date 03/04/2013

  Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport ModeratorTzero: public Mantid::API::Algorithm
{
public:
  /// (Empty) Constructor
  ModeratorTzero() : Mantid::API::Algorithm(), m_convfactor(0.5e+12*Mantid::PhysicalConstants::NeutronMass/Mantid::PhysicalConstants::meV), m_niter(1), m_t1min(200.0) {}
  /// Virtual destructor
  virtual ~ModeratorTzero() {}
  /// Algorithm's name
  virtual const std::string name() const { return "ModeratorTzero"; }
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Corrects the time of flight of an indirect geometry instrument by a time offset that is dependent on the energy of the neutron after passing through the moderator.";}

  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "CorrectionFunctions\\InstrumentCorrections"; }
  /// set attribute m_formula
  void setFormula(const std::string &formula);
  /// output m_t1min
  double gett1min();

private:
  Mantid::Geometry::Instrument_const_sptr m_instrument;
  
  // Initialisation code
  void init();
  /// Execution code for histogram workspace
  void exec();
  /// Execution code for event workspace
  void execEvent();
  /// Calculate distance from source to sample or monitor
  double CalculateL1(Mantid::API::MatrixWorkspace_sptr inputWS, size_t i);
  /// Calculate time from sample to detector
  double CalculateT2(Mantid::API::MatrixWorkspace_sptr inputWS, size_t i);
  /// Calculate emission time from the moderator for a given detector (L1, t2) and TOF
  double CalculateT0(const double &tof, const double &L1, const double &t2, double &E1, mu::Parser &parser);
  const double m_convfactor;
  /// Maximum number of iterations when calculating the emission time from the moderator
  size_t m_niter;
  /// tolerance for calculating E1, in micro-seconds
  double m_tolTOF;
  /// string containing the heuristic regression for the moderator emission time versus neutron energy
  std::string m_formula;
  /// tof limit for fast neutrons
  const double m_t1min;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_MODERATORTZERO_H_*/
