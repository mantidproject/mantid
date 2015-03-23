#ifndef MANTID_MDEVENTS_INTEGRATE_ELLIPSOIDS_H_
#define MANTID_MDEVENTS_INTEGRATE_ELLIPSOIDS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidMDEvents/MDWSDescription.h"

namespace Mantid {
namespace MDEvents {

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

  MDWSDescription m_targWSDescr;

  void initTargetWSDescr(API::MatrixWorkspace_sptr& wksp);
};

} // namespace MDEvents
} // namespace Mantid

#endif /* MANTID_MDEVENTS_INTEGRATE_ELLIPSOIDS_H_ */
