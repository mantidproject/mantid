#ifndef MANTID_MDEVENTS_FINDPEAKSMD_H_
#define MANTID_MDEVENTS_FINDPEAKSMD_H_
    
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/V3D.h"

namespace Mantid
{
namespace MDEvents
{

  /** FindPeaksMD : TODO: DESCRIPTION
   * 
   * @author
   * @date 2011-06-02
   */
  class DLLExport FindPeaksMD  : public API::Algorithm
  {
  public:
    FindPeaksMD();
    ~FindPeaksMD();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "FindPeaksMD";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "Optimization\\PeakFinding;MDAlgorithms";}
    
  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    /// Initialise the properties
    void init();
    /// Run the algorithm
    void exec();

    void readExperimentInfo(Mantid::API::ExperimentInfo_sptr ei, Mantid::API::IMDWorkspace_sptr ws);

    void addPeak(Mantid::Kernel::V3D Q, double binCount);

    template<typename MDE, size_t nd>
    void findPeaks(typename MDEventWorkspace<MDE, nd>::sptr ws);

    void findPeaksHisto(Mantid::MDEvents::MDHistoWorkspace_sptr ws);


    /// Output PeaksWorkspace
    Mantid::DataObjects::PeaksWorkspace_sptr peakWS;

    /// Estimated radius of peaks. Boxes closer than this are rejected
    coord_t peakRadiusSquared;

    /// Thresholding factor
    double DensityThresholdFactor;

    /// Max # of peaks
    int64_t MaxPeaks;

    /// Progress reporter.
    Mantid::API::Progress * prog;


    /** Enum describing which type of dimensions in the MDEventWorkspace */
    enum eDimensionType
    {
      HKL, QLAB, QSAMPLE
    };

    /// Instrument
    Mantid::Geometry::Instrument_const_sptr inst;
    /// Run number of the peaks
    int runNumber;
    /// Dimension type
    eDimensionType dimType;
    /// Goniometer matrix
    Mantid::Kernel::Matrix<double> goniometer;
  };


} // namespace Mantid
} // namespace MDEvents

#endif  /* MANTID_MDEVENTS_FINDPEAKSMD_H_ */
