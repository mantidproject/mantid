#ifndef MANTID_CRYSTAL_SAVEHKL_H_
#define MANTID_CRYSTAL_SAVEHKL_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 

namespace Mantid
{
namespace Crystal
{

  /** Save a PeaksWorkspace to a Gsas-style ASCII .hkl file.
   * 
   * @author Vickie Lynch, SNS
   * @date 2011-09-28
   */
  const double pc[4][19] =
  {{1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000,
    1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000},
   {1.9368, 1.8653, 1.6908, 1.4981, 1.3532, 1.2746, 1.2530, 1.2714, 1.3093, 1.3559,
    1.4019, 1.4434, 1.4794, 1.5088, 1.5317, 1.5489, 1.5608, 1.5677, 1.5700},
   {0.0145, 0.1596, 0.5175, 0.9237, 1.2436, 1.4308, 1.4944, 1.4635, 1.3770, 1.2585,
    1.1297, 1.0026, 0.8828, 0.7768, 0.6875, 0.6159, 0.5637, 0.5320, 0.5216},
   {1.1386, 1.0604, 0.8598, 0.6111, 0.3798, 0.1962, 0.0652, -0.0198, -0.0716, -0.0993,
    -0.1176, -0.1153, -0.1125, -0.1073, -0.1016, -0.0962, -0.0922, -0.0898, -0.0892}};

  const double MAX_WAVELENGTH = 50.0;    // max in lamda_weight table

  const double STEPS_PER_ANGSTROM = 100;  // resolution of lamda table

  const int NUM_WAVELENGTHS = static_cast<int>(std::ceil( MAX_WAVELENGTH * STEPS_PER_ANGSTROM));

  const double radtodeg_half = 180.0/M_PI/2.;

  class DLLExport SaveHKL  : public API::Algorithm
  {
  public:
    SaveHKL();
    ~SaveHKL();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "SaveHKL";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "Crystal;DataHandling\\Text";}
    
  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    /// Initialise the properties
    void init();
    /// Run the algorithm
    void exec();

    double absor_sphere(double& twoth, double& wl, double& tbar);
    double smu; // in 1/cm
    double amu; // in 1/cm
    double radius; // in cm
    double power_th;
    double spectrumCalc(double TOF, int iSpec,std::vector<std::vector<double> > time, std::vector<std::vector<double> > spectra, size_t id);

  };


} // namespace Mantid
} // namespace Crystal

#endif  /* MANTID_CRYSTAL_SAVEHKL_H_ */
