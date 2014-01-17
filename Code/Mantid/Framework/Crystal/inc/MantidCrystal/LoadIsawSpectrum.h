#ifndef MANTID_CRYSTAL_LoadIsawSpectrum_H_
#define MANTID_CRYSTAL_LoadIsawSpectrum_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 
#include "MantidDataObjects/PeaksWorkspace.h"

namespace Mantid
{
namespace Crystal
{

  /** Loads a Spectrum file
   * 
   * @author Vickie Lynch, SNS
   * @date 2014-01-16
   */

  const double radtodeg_half = 180.0/M_PI/2.;

  class DLLExport LoadIsawSpectrum  : public API::Algorithm
  {
  public:
    LoadIsawSpectrum();
    ~LoadIsawSpectrum();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "LoadIsawSpectrum";};
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

    double spectrumCalc(double TOF, int iSpec,std::vector<std::vector<double> > time, std::vector<std::vector<double> > spectra, size_t id);
  };


} // namespace Mantid
} // namespace Crystal

#endif  /* MANTID_CRYSTAL_LoadIsawSpectrum_H_ */
