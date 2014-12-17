#ifndef MANTID_ALGORITHMS_EQSANSTOFSTRUCTURE_H_
#define MANTID_ALGORITHMS_EQSANSTOFSTRUCTURE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace Algorithms {
/**

    Apply correction to EQSANS data to account for its TOF structure. The
   algorithm modifies the
    TOF values to correct for the fact that T_0 is not properly recorded by the
   DAS.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
// Pulse widge (micro sec per angstrom)
const double PULSEWIDTH = 20.0;
// Chopper phase offset (micro sec)
const double CHOPPER_PHASE_OFFSET[2][4] = {{9507., 9471., 9829.7, 9584.3},
                                           {19024., 18820., 19714., 19360.}};
// Chopper angles (degree)
const double CHOPPER_ANGLE[4] = {129.605, 179.989, 230.010, 230.007};
// Chopper location (mm)
const double CHOPPER_LOCATION[4] = {5700., 7800., 9497., 9507.};

class DLLExport EQSANSTofStructure : public API::Algorithm {
public:
  /// (Empty) Constructor
  EQSANSTofStructure() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~EQSANSTofStructure() {}
  /// Algorithm's name
  virtual const std::string name() const { return "EQSANSTofStructure"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Corrects the TOF of raw EQSANS data. This algorithm needs to be "
           "run once on every data set.";
  }

  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "SANS"; }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
  // void execEvent(Mantid::DataObjects::EventWorkspace_sptr inputWS, bool
  // frame_skipping);
  void execEvent(Mantid::DataObjects::EventWorkspace_sptr inputWS,
                 double threshold, double frame_offset, double tof_frame_width,
                 double tmp_frame_width, bool frame_skipping);

  /// Compute TOF offset
  double getTofOffset(DataObjects::EventWorkspace_const_sptr inputWS,
                      bool frame_skipping);
  double frame_tof0;
  bool flight_path_correction;
  double low_tof_cut;
  double high_tof_cut;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_EQSANSTOFSTRUCTURE_H_*/
