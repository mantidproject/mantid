#ifndef MANTID_ALGORITHMS_MONITOREFFICIENCYCORUSER_H_
#define MANTID_ALGORITHMS_MONITOREFFICIENCYCORUSER_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid {

namespace HistogramData {
class HistogramX;
class HistogramY;
class HistogramE;
} // namespace HistogramData
namespace Algorithms {

class DLLExport MonitorEfficiencyCorUser : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "MonitorEfficiencyCorUser"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "This algorithm normalises the counts by the monitor counts with "
           "additional efficiency correction according to the formula set in "
           "the instrument parameters file.";
  }

  /// Algorithm's version
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"NormaliseToMonitor"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "CorrectionFunctions\\NormalisationCorrections";
  }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  double calculateFormulaValue(const std::string &, double);
  std::string getValFromInstrumentDef(const std::string &);

  /// The user selected (input) workspace
  API::MatrixWorkspace_const_sptr m_inputWS;
  /// The output workspace, maybe the same as the input one
  API::MatrixWorkspace_sptr m_outputWS;
  /// stores the incident energy of the neutrons
  double m_Ei = 0.0;
  /// stores the total count of neutrons from the monitor
  double m_monitorCounts = 0;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MONITOREFFICIENCYCORUSER_H_*/
