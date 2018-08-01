#ifndef MANTID_ALGORITHMS_ConvertToPoleFigure_H_
#define MANTID_ALGORITHMS_ConvertToPoleFigure_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidTypes/Core/DateAndTime.h"
#include "MantidTypes/Event/TofEvent.h"

namespace Mantid {
namespace Algorithms {

using Types::Core::DateAndTime;
using Types::Event::TofEvent;

/** ConvertToPoleFigure : Calcualte Pole Figure for engineering material
 */
class DLLExport ConvertToPoleFigure : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "function (PDF). G(r) will be stored in another named workspace.";
  }

  /// Algorithm's version for identification
  int version() const override;
  /// Algorithm's category for identification
  const std::string category() const override;

private:
  /// Initialize the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  /// process inputs suite
  void processInputs();
  void processEventModeInputs();

  /// general methods used by convertToPoleFigureHistogramMode and
  /// convertToPoleFigureEventMode
  void retrieveInstrumentInfo(Kernel::V3D &sample_pos,
                              Kernel::V3D &sample_src_unit_k);
  Kernel::V3D calculateUnitQ(size_t iws, Kernel::V3D samplepos,
                             Kernel::V3D k_sample_src_unit);
  void rotateVectorQ(Kernel::V3D unitQ, const double &hrot, const double &omega,
                     double &r_td, double &r_nd);

  /// calculate pole figure in conventional histogram approach
  void convertToPoleFigureHistogramMode();

  /// calcualte pole figure in the event ode
  void convertToPoleFigureEventMode();
  void sortEventWS();
  void setupOutputVectors();
  Types::Event::TofEvent me;
  size_t findDRangeInEventList(std::vector<Types::Event::TofEvent> vector,
                               double tof, bool index_to_right);

  /// generatae output workspace and set output properties
  void generateOutputsHistogramMode();

  /// generate output MDEventWorkspace
  void generateMDEventWS();

  /// input workspace
  API::MatrixWorkspace_const_sptr m_inputWS;
  DataObjects::EventWorkspace_const_sptr
      m_eventWS; // same object as m_inputWS if inputWS is EventWorkspace
  /// input counts: only required for the histogram mode
  API::MatrixWorkspace_const_sptr m_peakIntensityWS;

  /// sample log name
  std::string m_nameHROT;
  std::string m_nameOmega;

  /// vector to record pole figure
  std::vector<double> m_poleFigureRTDVector;
  std::vector<double> m_poleFigureRNDVector;
  std::vector<double> m_poleFigurePeakIntensityVector;

  /// mode
  bool m_inEventMode;
  double m_minD;
  double m_maxD;

  ///
};

} // namespace Mantid
} // namespace Algorithms

#endif /* MANTID_ALGORITHMS_ConvertToPoleFigure_H_ */
