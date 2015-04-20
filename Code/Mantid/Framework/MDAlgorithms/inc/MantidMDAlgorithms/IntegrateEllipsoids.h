#ifndef MANTID_MDALGORITHMS_INTEGRATE_ELLIPSOIDS_H_
#define MANTID_MDALGORITHMS_INTEGRATE_ELLIPSOIDS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidMDAlgorithms/MDWSDescription.h"
#include "MantidMDAlgorithms/Integrate3DEvents.h"
#include "MantidMDAlgorithms/UnitsConversionHelper.h"
#include "MantidMDAlgorithms/MDTransfInterface.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid {
namespace MDAlgorithms {

class DLLExport IntegrateEllipsoids : public API::Algorithm {
public:
  IntegrateEllipsoids();
  virtual ~IntegrateEllipsoids();
  void qListFromEventWS(Integrate3DEvents &integrator, API::Progress &prog,
                        DataObjects::EventWorkspace_sptr &wksp,
                        MDTransf_sptr &qConverter,
                        Kernel::DblMatrix const &UBinv, bool hkl_integ);
  void qListFromHistoWS(Integrate3DEvents &integrator, API::Progress &prog,
                        DataObjects::Workspace2D_sptr &wksp,
                        MDTransf_sptr &qConverter,
                        Kernel::DblMatrix const &UBinv, bool hkl_integ);
  virtual const std::string name() const;
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Integrate Single Crystal Diffraction Bragg peaks using 3D "
           "ellipsoids.";
  }

  virtual int version() const;
  virtual const std::string category() const;

private:
  void init();
  void exec();

  MDWSDescription m_targWSDescr;

  void initTargetWSDescr(API::MatrixWorkspace_sptr &wksp);
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_INTEGRATE_ELLIPSOIDS_H_ */
