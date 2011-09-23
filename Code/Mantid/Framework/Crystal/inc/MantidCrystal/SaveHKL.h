#ifndef MANTID_CRYSTAL_SAVEHKL_H_
#define MANTID_CRYSTAL_SAVEHKL_H_
/*WIKI* 

Save a PeaksWorkspace to a ISAW-style ASCII .peaks file.
*WIKI*/
    
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
  {    {0.9369, 0.9490, 0.9778, 1.0083, 1.0295, 1.0389, 1.0392, 1.0338,
        1.0261, 1.0180, 1.0107, 1.0046, 0.9997, 0.9957, 0.9929, 0.9909,
        0.9896, 0.9888, 0.9886},
       {2.1217, 2.0149, 1.7559, 1.4739, 1.2669, 1.1606, 1.1382, 1.1724,
        1.2328, 1.3032, 1.3706, 1.4300, 1.4804, 1.5213, 1.5524, 1.5755,
        1.5913, 1.6005, 1.6033},
       {-0.1304, 0.0423, 0.4664, 0.9427, 1.3112, 1.5201, 1.5844, 1.5411,
        1.4370, 1.2998, 1.1543, 1.0131, 0.8820, 0.7670, 0.6712, 0.5951,
        0.5398, 0.5063, 0.4955},
       {1.1717, 1.0872, 0.8715, 0.6068, 0.3643, 0.1757, 0.0446, -0.0375,
       -0.0853, -0.1088, -0.1176, -0.1177, -0.1123, -0.1051, -0.0978,
       -0.0914, -0.0868, -0.0840, -0.0833}};
  const double MAX_WAVELENGTH = 50.0;    // max in lamda_weight table

  const double STEPS_PER_ANGSTROM = 100;  // resolution of lamda table

  const int NUM_WAVELENGTHS = std::ceil( MAX_WAVELENGTH * STEPS_PER_ANGSTROM);

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
    virtual const std::string category() const { return "Crystal";}
    
  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    /// Initialise the properties
    void init();
    /// Run the algorithm
    void exec();

    double absor_sphere(double& twoth, double& wl, double& tbar) ;
    double smu; // in 1/cm
    double amu; // in 1/cm
    double radius; // in cm



  };


} // namespace Mantid
} // namespace Crystal

#endif  /* MANTID_CRYSTAL_SAVEHKL_H_ */
