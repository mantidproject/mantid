#ifndef MANTID_CRYSTAL_SAVELauenorm_H_
#define MANTID_CRYSTAL_SAVELauenorm_H_
    
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h" 
#include "MantidDataObjects/PeaksWorkspace.h"

namespace Mantid
{
namespace Crystal
{

  /** Save a PeaksWorkspace to a lauenorm format
   * http://www.ccp4.ac.uk/cvs/viewvc.cgi/laue/doc/lauenorm.ptx?diff_format=s&revision=1.1.1.1&view=markup
   * 
   * @author Vickie Lynch, SNS
   * @date 2014-07-24
   */

  class DLLExport SaveLauenorm  : public API::Algorithm
  {
  public:
    SaveLauenorm();
    ~SaveLauenorm();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "SaveLauenorm";};
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Save a PeaksWorkspace to a ASCII file for each detector.";}

    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "Crystal;DataHandling\\Text";}
    
  private:
    
    /// Initialise the properties
    void init();
    /// Run the algorithm
    void exec();

    DataObjects::PeaksWorkspace_sptr ws;
    void sizeBanks(std::string bankName, int& nCols, int& nRows);
  };


} // namespace Mantid
} // namespace Crystal

#endif  /* MANTID_CRYSTAL_SAVELauenorm_H_ */
