// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cmath>

// NAN is not defined in Visual C++ < 2013
#if (defined(_MSC_VER) && (_MSC_VER <= 1800))
#define INFINITY (DBL_MAX + DBL_MAX)
#define NAN (INFINITY - INFINITY)
#endif

namespace Mantid {

/** A namespace containing physical constants that are required by algorithms
   and unit routines.
*/
namespace PhysicalConstants {
/** Avagodro constant in mol<SUP>-1</SUP>. Taken from
    <http://physics.nist.gov/cuu/Constants> on 2010-12-28. */
static constexpr double N_A = 6.02214179e23;

/** Planck constant in J*s. Taken from <http://physics.nist.gov/cuu/Constants>
    on 2007-10-31 and confirmed again on 2010-12-28. */
static constexpr double h = 6.62606896e-34;

/** Planck constant in J*s, divided by 2 PI. */
static constexpr double h_bar = h / (2 * M_PI);

/** Standard acceleration due to gravity. Precise value in ms<SUP>-2</SUP>. */
static constexpr double g = 9.80665;

/** Mass of the neutron in kg. Taken from
 * <http://physics.nist.gov/cuu/Constants> on 30/10/2007. */
static constexpr double NeutronMass = 1.674927211e-27;

/** Mass of the neutron in AMU. Taken from
 * <http://physics.nist.gov/cuu/Constants> on 02/01/2013. */
static constexpr double NeutronMassAMU = 1.008664916;

/** AMU in kg. Taken from <http://physics.nist.gov/cuu/Constants> on 10/09/2014.
 */
static constexpr double AtomicMassUnit = 1.660538921e-27;

/** 1 meV in Joules. Taken from <http://physics.nist.gov/cuu/Constants> on
 * 28/03/2008. */
static constexpr double meV = 1.602176487e-22;

/** 1 meV in wavenumber (cm<SUP>-1</SUP>). Taken from
 * <http://physics.nist.gov/cuu/Constants> on 02/04/2008. */
static constexpr double meVtoWavenumber = 8.06554465;

/** 1 meV in frequency (GHz). Division of energy by Plank's constant. Taken from
 * <http://physics.nist.gov/cuu/Constants> on 03/11/2017. */
static constexpr double meVtoFrequency = 0.2417989262;

/** 1 meV in Kelvin. Taken from <http://physics.nist.gov/cuu/Constants> on
 * 09/09/2011. */
static constexpr double meVtoKelvin = 11.604519;

/** Transformation coefficient to transform neutron energy into neutron
 * wavevector: K-neutron[m^-10] = sqrt(E_neutron[meV]/E_transtormation[mEv]);*/
static constexpr double E_mev_toNeutronWavenumberSq =
    1.0e20 * h_bar * h_bar / (2 * NeutronMass * meV); //[meV*Angstrom^2]

/**  Muon lifetime. Taken from Particle Data Group on 15/1/2016
 * <http://pdg.lbl.gov/2015/tables/rpp2015-sum-leptons.pdf>. */
static constexpr double MuonLifetime = 2.1969811e-6;

/** Standard atmospheric pressure in kPa.
 * Taken from
 * <http://physics.nist.gov/cgi-bin/cuu/Value?stdatm|search_for=adopted_in!> on
 * 01/12/2010 **/
static constexpr double StandardAtmosphere = 101.325;

/** Boltzmann Constant in meV/K
 * Taken from <http://physics.nist.gov/cuu/Constants> on 10/07/2012
 */
static constexpr double BoltzmannConstant = 8.6173324e-02;

/** Muon gyromagnetic ratio in MHz/G
 * Taken from CalMuonDetectorPhases and DynamicKuboToyabe on 02/02/2016
 */
static constexpr double MuonGyromagneticRatio = 0.01355342;

} // namespace PhysicalConstants
} // namespace Mantid
