#ifndef MANTID_ALGORITHMS_EQSANSUNFOLDFRAME_H_
#define MANTID_ALGORITHMS_EQSANSUNFOLDFRAME_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/Run.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid {
namespace Algorithms {
/**
Apply correction to EQSANS data to account for its TOF structure. The algorithm
modifies the TOF values to correct for the fact that T_0 is not properly
recorded by the DAS.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

// Pulse width (micro sec per angstrom)
const double PULSEWIDTH = 20.0;
const double MAXWL = 35.0;  // wavelength of fastest considered neutrons
const size_t NCHOPPERS = 4; // there are four choppers
// Chopper phase offset (micro sec). Second set if operating in frame-skipping
// mode
const double CHOPPER_PHASE_OFFSET[2][NCHOPPERS] = {
    {9507., 9471., 9829.7, 9584.3}, {19024., 18820., 19714., 19360.}};
// Chopper opening angles (degree)
const double CHOPPER_ANGLE[4] = {129.605, 179.989, 230.010, 230.007};
// Chopper location from moderator(mm)
const double CHOPPER_LOCATION[4] = {5700., 7800., 9497., 9507.};

/// Wavelength band
class DLLExport EQSANSWBand {
public:
  EQSANSWBand();
  EQSANSWBand(const double &lMin, const double &lMax);
  double width();
  EQSANSWBand intersect(const EQSANSWBand &otherBand) const;
  inline bool operator==(const EQSANSWBand &b) const {
    return (m_min == b.m_min && m_max == b.m_max);
  }
  double m_min;
  double m_max;
};

/// Set of wavelength bands transmitted by a chopper
class DLLExport EQSANSTransWBands {
public:
  EQSANSTransWBands();
  size_t size();
  EQSANSTransWBands intersect(const EQSANSWBand &otherBand) const;
  EQSANSTransWBands intersect(const EQSANSTransWBands &otherGates) const;
  inline bool operator==(const EQSANSTransWBands &b) const {
    return (m_bands == b.m_bands);
  }

  std::vector<EQSANSWBand> m_bands;
};

class DLLExport EQSANSDiskChopper {
public:
  EQSANSDiskChopper();
  double period() const;
  double transmissionDuration() const;
  double openingPhase() const;
  double closingPhase() const;
  double rewind() const;
  double tofToWavelength(double tof, double delay = 0.0,
                         bool pulsed = false) const;
  void setSpeed(const API::Run &run);
  void setPhase(const API::Run &run, double offset);
  EQSANSTransWBands transmissionBands(double maxWl, double delay = 0.0,
                                      bool pulsed = true) const;

  /// chopper index, from zero to (NCHOPPERS - 1)
  size_t m_index;
  /// distance to the moderator in meters
  double m_location;
  /// transmission window aperture, in degrees
  double m_aperture;
  /// time for the middle of transmission window to hit the beam, in
  /// microseconds
  double m_phase;
  /// rotational speed, in Hz
  double m_speed;
};

class DLLExport EQSANSUnfoldFrame : public API::Algorithm {
public:
  /// Default constructor
  EQSANSUnfoldFrame();
  /// Algorithm's namef
  const std::string name() const override { return "EQSANSUnfoldFrame"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Corrects the TOF of raw EQSANS data. This algorithm needs to be "
           "run once on every data set.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override { return "SANS"; }
  double getPulseFrequency();
  void setPulsePeriod();
  double getPulsePeriod() const { return m_pulsePeriod; }
  void setFrameSkippingMode();
  bool getFrameSkippingMode() const { return m_frameSkippingMode; }
  void setFrameWidth();
  double getFrameWidth() const { return m_frameWidth; }
  void initializeChoppers();

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  EQSANSWBand transmittedBand(double delay = 0.0) const;
  double travelTime(double wavelength, double flightPath,
                    bool pulsed = false) const;
  double tof_to_wavelength(double tof, double flightPath, double delay = 0.0,
                           bool pulsed = false) const;
  double nominalDetectorDistance() const;
  size_t frameOperating(double wavelength) const;
  void execEvent();
  /// Minimal allowed TOF
  double m_lowTOFcut;
  /// Maximal allowed TOF under one pulse period
  double m_highTOFcut;
  /// 1.0e6/60 micro-seconds
  double m_pulsePeriod;
  /// 1.0e6/30 ms if in frame skipping mode, otherwise 1.0e6/60 ms
  double m_frameWidth;
  /// frame_mode_operation - 1 (e.g. m_frameOffset==0 when operating in the
  /// first frame)
  double m_frameOffset;
  /// signals choppers operating at 30Hz
  bool m_frameSkippingMode;
  std::vector<EQSANSDiskChopper> m_choppers;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_EQSANSUNFOLDFRAME_H_*/
