#ifndef MANTID_MDALGORITHMS_INTEGRATEPEAKSCWSD_H_
#define MANTID_MDALGORITHMS_INTEGRATEPEAKSCWSD_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidAPI/CompositeFunction.h"

namespace Mantid {
namespace MDAlgorithms {

/** Integrate single-crystal peaks in reciprocal-space.
 *
 * @author Janik Zikovsky
 * @date 2011-04-13 18:11:53.496539
 */
class DLLExport IntegratePeaksCWSD : public API::Algorithm {
public:
  IntegratePeaksCWSD();
  ~IntegratePeaksCWSD();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "IntegratePeaksCWSD"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Integrate single-crystal peaks in reciprocal space, for "
           "MDEventWorkspaces.";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 1; };
  /// Algorithm's category for identification
  virtual const std::string category() const { return "MDAlgorithms\\Peaks"; }

  void simplePeakIntegration(const std::vector<detid_t> &vecMaskedDetID);

private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();

  template <typename MDE, size_t nd>
  void integrate(typename DataObjects::MDEventWorkspace<MDE, nd>::sptr ws);


  std::vector<detid_t> processMaskWorkspace(DataObjects::MaskWorkspace_const_sptr maskws);

  /// Input MDEventWorkspace
  Mantid::API::IMDEventWorkspace_sptr m_inputWS;

  /// Input PeaksWorkspace
  Mantid::DataObjects::PeaksWorkspace_sptr m_peaksWS;

  /// Mask
  bool m_maskDets;
  DataObjects::MaskWorkspace_sptr m_maskWS;
};

} // namespace Mantid
} // namespace DataObjects

#endif /* MANTID_MDALGORITHMS_INTEGRATEPEAKSCWSD_H_ */
