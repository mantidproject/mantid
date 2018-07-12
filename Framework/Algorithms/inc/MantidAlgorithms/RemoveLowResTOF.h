#ifndef REMOVELOWRESTOF_H_
#define REMOVELOWRESTOF_H_

#include "MantidAPI/DistributedAlgorithm.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/IComponent.h"

namespace Mantid {

namespace Algorithms {
class DLLExport RemoveLowResTOF : public API::DistributedAlgorithm {
public:
  RemoveLowResTOF();
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Removes low resolution Time of Flight data.";
  }

private:
  void init() override;
  void exec() override;
  void execEvent(const API::SpectrumInfo &spectrumInfo);

  /// Pointer to the input workspace
  API::MatrixWorkspace_const_sptr m_inputWS;
  /// Pointer to the input event workspace
  DataObjects::EventWorkspace_const_sptr m_inputEvWS;
  double calcTofMin(const std::size_t, const API::SpectrumInfo &spectrumInfo);
  void getTminData(const bool);
  /// The reference value for DIFC to filter with
  double m_DIFCref;
  /// Mystery variable that I'm not sure what it is for
  double m_K;
  /// The start of the time-of-flight frame
  double m_Tmin;
  /// The minimum wavelength accessible in the frame
  double m_wavelengthMin;
  /// The number of spectra in the workspace
  std::size_t m_numberOfSpectra;
  /// Progress reporting
  std::unique_ptr<API::Progress> m_progress = nullptr;
  /// Flag to generate low resolution TOF workspace
  bool m_outputLowResTOF;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* REMOVELOWRESTOF_H_ */
