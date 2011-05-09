#ifndef MANTID_DATAHANDLING_LOADCALFILE_H_
#define MANTID_DATAHANDLING_LOADCALFILE_H_
    
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidGeometry/IInstrument.h"
#include "MantidKernel/System.h"


namespace Mantid
{
namespace DataHandling
{

  /** LoadCalFile : TODO: DESCRIPTION
   * 
   * @author
   * @date 2011-05-09 11:56:58.393364
   */
  class DLLExport LoadCalFile  : public API::Algorithm
  {
  public:
    LoadCalFile();
    ~LoadCalFile();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "LoadCalFile";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "DataHandling";}

    static void getInstrument3WaysInit(Mantid::API::Algorithm * alg);

    static Mantid::Geometry::IInstrument_sptr getInstrument3Ways(Mantid::API::Algorithm * alg);

    static void readCalFile(const std::string& calFileName,
        Mantid::DataObjects::GroupingWorkspace_sptr groupWS, Mantid::DataObjects::OffsetsWorkspace_sptr offsetsWS, Mantid::API::MatrixWorkspace_sptr maskWS);

    
  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    /// Initialise the properties
    void init();
    /// Run the algorithm
    void exec();

    static Kernel::Logger& g_log;    ///< reference to the logger class

  };


} // namespace Mantid
} // namespace DataHandling

#endif  /* MANTID_DATAHANDLING_LOADCALFILE_H_ */
