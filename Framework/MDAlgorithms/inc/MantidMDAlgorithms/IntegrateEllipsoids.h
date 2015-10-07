#ifndef MANTID_MDALGORITHMS_INTEGRATE_ELLIPSOIDS_H_
#define MANTID_MDALGORITHMS_INTEGRATE_ELLIPSOIDS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidMDAlgorithms/MDWSDescription.h"
#include "MantidMDAlgorithms/Integrate3DEvents.h"
#include "MantidMDAlgorithms/UnitsConversionHelper.h"
#include "MantidMDAlgorithms/MDTransfInterface.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"

namespace Mantid {
namespace MDAlgorithms {

class DLLExport IntegrateEllipsoids : public API::Algorithm {
public:
  IntegrateEllipsoids();
  virtual ~IntegrateEllipsoids();
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
  void qListFromEventWS(Integrate3DEvents &integrator, API::Progress &prog,
                        DataObjects::EventWorkspace_sptr &wksp,
                        Kernel::DblMatrix const &UBinv, bool hkl_integ);
  void qListFromHistoWS(Integrate3DEvents &integrator, API::Progress &prog,
                        DataObjects::Workspace2D_sptr &wksp,
                        Kernel::DblMatrix const &UBinv, bool hkl_integ);

  /// Calculate if this Q is on a detector
  void calculateE1(Geometry::Instrument_const_sptr inst);

  void runMaskDetectors(Mantid::DataObjects::PeaksWorkspace_sptr peakWS,
                        std::string property, std::string values);

  /// save for all detector pixels
  std::vector<Kernel::V3D> E1Vec;

  MDWSDescription m_targWSDescr;

  void initTargetWSDescr(API::MatrixWorkspace_sptr &wksp);
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_INTEGRATE_ELLIPSOIDS_H_ */
