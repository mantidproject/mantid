/*WIKI* 


This algorithm is for use with white-beam instruments with choppers. The chopper cuts the range of wavelengths, so all detectors (including monitors) should be reduced to the same wavelength range. This is done using a “reference” flightpath, <math>L_{ref}</math>, which is (usually, see below) the flightpath of the farthest detectors.

If <math>T_{min}</math> and <math>T_{max}</math> are the beginning and end of the frame, for each detector <math>D</math> at total flightpath <math>L_d</math> the following times are defined:

:<math> T_1 = T_{max} - \left ( T_{min} \times \left ( 1 - \frac{L_d}{L_{ref}} \right ) \right ) </math>

:<math> T_2 = T_{max} \times \left ( \frac{L_d}{L_{ref}} \right ) </math>

Thus if <math>L_d < L_{ref}</math> then <math>T_1 > T_2</math>

Neutron velocities (and hence wavelengths) for the detector <math>D</math> are calculated in the following way:

* For <math>T_{min} < T < T_2</math>, velocities are calculated in the usual way, i.e. <math> v = \frac{L_d}{T}</math>

* Data in the range <math> T_2 < T < T_1 </math> are ignored (i.e. they are not used in the wavelength-converted histograms)

* For <math> T_1 < T < T_{max}</math>, velocities are calculated using the formula <math>v = \frac{L_d}{T - T_{max} + T_{min}}</math>


Note that the minimum and maximum velocities for the points that are actually ''used'' are:

:<math> v_{max} = \frac{L_d}{T - T_{max} + T_{min}} = \frac{L_{ref}}{T_{min}} </math> and <math>v_{min} = \frac{L_d}{T_2} = \frac{L_{ref}}{T_{max}} </math>

In other words, these velocities are the same for all detectors, so the wavelength range in the transformed histogram will correspondingly be the same and this algorithm rebins the data into common bins in this range.

Occasionally, it may be that some detectors (typically downstream monitors) may be at a *longer* flightpath than <math>L_{ref}</math>. This depends entirely on the chopper aperture/setting. These detectors are “frame-overlapped” – in other words, there is an ambiguity in the definition of the wavelength for certain points, which should therefore be excluded. These points are at the very beginning and at the very end of the frame for a range <math>Dt</math> (equal on both sides) given by

:<math>D_t = (T_{max} - T_{min}) \times \left (1 - \frac{L_{ref}}{L_d} \right)</math>

In other words, points between <math>T_{min}</math> and <math>T_{min} + D_t</math> and between <math>T_{max} - D_t</math> and <math>T_{max}</math> should be excluded. For all other points, velocities and wavelengths are calculated in the normal way.

Note that since we are dealing with histogrammed data, the cut-off values above will almost certainly not fall exactly on a bin boundary. The approach taken by this algorithm is that if any part of a bin has a value that should be excluded, then the entire bin is excluded. What this means in practice is that the edge bins will possibly have a reduced number of counts.

==== Restrictions on the input workspace ====
The input workspace must contain histogram data where the X unit is time-of-flight and the Y data is raw counts. The [[instrument]] associated with the workspace must be fully defined because detector, source & sample position are needed.

====Subalgorithms used====
If the input workspace contains more than a single spectrum, Unwrap makes use of the [[rebin]] algorithm to set the bins on the output workspace to common values which cover the maximum theoretically accessible wavelength range.


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Unwrap.h"

namespace Mantid
{
namespace Algorithms
{

DECLARE_ALGORITHM(Unwrap)

/// Default constructor
Unwrap::Unwrap()
{
  this->useAlgorithm("UnwrapMonitor");
  this->deprecatedDate("2011-03-17"); // Saint Patrick's day!!!!!
}

Unwrap::~Unwrap()
{}

const std::string Unwrap::name() const
{
  return "Unwrap";
}

int Unwrap::version() const
{
  return 1;
}

} // namespace Algorithm
} // namespace Mantid
