#ifndef REMOVELOWRESTOF_H_
#define REMOVELOWRESTOF_H_

// includes
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/IComponent.h"

namespace Mantid {

namespace Algorithms {
class DLLExport RemoveLowResTOF : public API::Algorithm {
public:
  RemoveLowResTOF();
  ~RemoveLowResTOF() override;
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

  API::MatrixWorkspace_const_sptr m_inputWS; ///< Pointer to the input workspace
  DataObjects::EventWorkspace_const_sptr
      m_inputEvWS; ///< Pointer to the input event workspace
  double calcTofMin(const std::size_t, const API::SpectrumInfo &spectrumInfo);
  void runMaskDetectors();
  void getTminData(const bool);
  double m_DIFCref;       ///< The reference value for DIFC to filter with
  double m_K;             ///< Mystery variable that I'm not sure what it is for
  double m_Tmin;          ///< The start of the time-of-flight frame
  double m_wavelengthMin; ///< The minimum wavelength accessible in the frame
  std::size_t m_numberOfSpectra; ///< The number of spectra in the workspace
  API::Progress *m_progress;     ///< Progress reporting
  bool m_outputLowResTOF; /// Flag to generate low resolution TOF workspace
};

} // namespace Algorithms
} // namespace Mantid

#endif /* REMOVELOWRESTOF_H_ */
