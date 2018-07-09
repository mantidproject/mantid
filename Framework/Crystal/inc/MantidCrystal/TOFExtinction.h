#ifndef MANTID_CRYSTAL_TOFEXTINCTION_H_
#define MANTID_CRYSTAL_TOFEXTINCTION_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Crystal {

/** Save a PeaksWorkspace to a Gsas-style ASCII .hkl file.
 *
 * @author Vickie Lynch, SNS
 * @date 2012-01-20
 */
class DLLExport TOFExtinction : public API::Algorithm {
public:
  TOFExtinction();

  /// Algorithm's name for identification
  const std::string name() const override { return "TOFExtinction"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Extinction correction for single crystal peaks.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  /// Algorithm's category for identification
  const std::string category() const override {
    return "Crystal;DataHandling\\Text";
  }

private:
  /// Initialise the properties;
  void init() override;
  /// Run the algorithm;
  void exec() override;
  double getEg(double mosaic);
  double getEgLaue(double Eg, double twoth, double wl, double divBeam,
                   double betaBeam);
  double getXqt(double Eg, double cellV, double wl, double twoth, double tbar,
                double fsq);
  double getZachariasen(double Xqt);
  double getGaussian(double Xqt, double twoth);
  double getLorentzian(double Xqt, double twoth);
  double getEsLaue(double r, double twoth, double wl);
  double getRg(double EgLaue, double EsLaue, double wl, double twoth);
  double getRgGaussian(double EgLaue, double r_crystallite, double wl,
                       double twoth);
  double getRgLorentzian(double EgLaue, double r_crystallite, double wl,
                         double twoth);
  double getXqtII(double Rg, double cellV, double wl, double twoth, double tbar,
                  double fsq);
  double getTypeIIZachariasen(double XqtII);
  double getTypeIIGaussian(double XqtII, double twoth);
  double getTypeIILorentzian(double XqtII, double twoth);
  double getSigFsqr(double Rg, double cellV, double wl, double twoth,
                    double tbar, double fsq, double sigfsq,
                    double relSigRg = 0.03);
  double absor_sphere(double &twoth, double &wl);
  double m_smu = 0.0;    ///< linear scattering coefficient in 1/cm
  double m_amu = 0.0;    ///< linear absoprtion coefficient in 1/cm
  double m_radius = 0.0; ///< sample radius in cm
};
} // namespace Crystal
} // namespace Mantid
#endif /* MANTID_CRYSTAL_TOFEXTINCTION_H_ */
