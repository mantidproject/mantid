// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_MODERATORTZEROLINEAR_H_
#define MANTID_ALGORITHMS_MODERATORTZEROLINEAR_H_

#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Instrument.h"

namespace Mantid {
namespace API {
class SpectrumInfo;
}
namespace Algorithms {
/* Corrects the time of flight (TOF) by a time offset that is dependent on the
 velocity of the neutron after passing through the moderator.
 * "The TOF measured by the BASIS data acquisition system (DAS) should be
 reduced by this moderator emission time. The DAS "erroneously"
 *  thinks that it takes longer for neutrons to reach the sample and detectors,
 because it does not "know" that the neutrons
 *  spend some time in the moderator before being emitted and starting flying" -
 E. Mamontov
 *
 *  A heuristic formula for the correction, stored in the instrument definition
 file, is taken as linear on the initial neutron wavelength lambda_i:
 *    t_0 = gradient * lambda_i + intercept,  [gradient]=microsec/Angstrom and
 [intercept]=microsec
 *
 *  Required Properties:
    <UL>
    <LI> InputWorkspace  - EventWorkSpace in TOF units. </LI>
    <LI> OutputWorkspace - EventWorkSpace in TOF units. </LI>
    <LI> Moderator.Tzero.gradient - Variation of the time offset with initial
 neutron wavelength (obtained from the instrument parameter file)
    <LI> Moderator.Tzero.intercept - time offset common to all neutrons
 (obtained from the instrument parameter file)
    </UL>

         The recorded TOF = t_0 + t_i + t_f with
                 t_0: moderator emission time
                 t_i: time from moderator to sample
                 t_f: time from sample to detector
         This algorithm will replace TOF with TOF' = TOF-t_0 = t_i+t_f

         For a direct geometry instrument, lambda_i is the same for all
 neutrons. Hence the moderator emission time is the same for all neutrons
         For an indirect geometry instrument, lambda_i is not known but the
 final energy, E_f, selected by the analyzers is known. For this geometry:
                 t_f = L_f/v_f   L_f: distance from sample to detector, v_f:
 final velocity derived from E_f
                 t_i = L_i/v_i   L_i: distance from moderator to sample, v_i:
 initial velocity unknown
                 t_0 = a/v_i+b   a and b are constants derived from the
 aforementioned heuristic formula.
                                 a=gradient*3.956E-03, [a]=meter,
 b=intercept, [b]=microsec
                 Putting all together:  TOF' = (L_i/(L_i+a))*(TOF-t_f-b) + t_i,
 [TOF']=microsec

    @author Jose Borreguero and Andrei Savici
    @date 12/12/2011
*/
class DLLExport ModeratorTzeroLinear : public API::Algorithm {
public:
  /// Default constructor
  ModeratorTzeroLinear();
  /// Algorithm's name
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override;
  /// Algorithm's version
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"ModeratorTzero"};
  }
  /// Algorithm's category for identification
  const std::string category() const override;

private:
  // conversion constants applicable to histogram and event workspaces
  double m_gradient;
  double m_intercept;
  Geometry::Instrument_const_sptr m_instrument;

  // Initialisation code
  void init() override;
  // Execution code for histogram workspace
  void exec() override;
  // Execution code for event workspace
  void execEvent();
  // Calculate time from sample to detector and initial flight path
  void calculateTfLi(const API::SpectrumInfo &spectrumInfo, size_t i,
                     double &t_f, double &L_i);
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_MODERATORTZEROLINEAR_H_*/
