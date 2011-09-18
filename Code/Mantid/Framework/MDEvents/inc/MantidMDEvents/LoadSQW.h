#ifndef MANTID_MDEVENTS_LOAD_SQW_H_
#define MANTID_MDEVENTS_LOAD_SQW_H_
/*WIKI* 

The algorithm takes every pixel defined in the SQW horace file and converts it into an event. SQW DND/Image data is used to format dimension, with which to work.After the algorithm completes a fully formed [[MDEventWorkspace]] is provided.
If the OutputWorkspace does NOT already exist, a default one is created. This is not the only route to generating MDEventWorkspaces.
*WIKI*/

#include "MantidAPI/Algorithm.h"
#include "MantidMDEvents/MDEventFactory.h"
#include <fstream>

namespace Mantid
{
namespace MDEvents
{

  /** LoadSQW :
   * Load an SQW file and read observations in as events to generate a IMDEventWorkspace, with events in reciprocal space (Qx, Qy, Qz) 
   * 
   * @author Owen Arnold, Tessella, ISIS
   * @date 12/July/2011
   */
  class DLLExport LoadSQW  : public API::Algorithm
  {
  public:
    LoadSQW();
    ~LoadSQW();
    
    /// Algorithm's name for identification 
    virtual const std::string name() const { return "LoadSQW";};
    /// Algorithm's version for identification 
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "DataHandling";}
  
  protected:

    /// Add events onto the workspace.
    virtual void addEvents(Mantid::MDEvents::MDEventWorkspace<MDLeanEvent<4>,4>* ws);

    /// Add dimensions onto the workspace.
    virtual void addDimensions(Mantid::MDEvents::MDEventWorkspace<MDLeanEvent<4>,4>* ws);

    /// Parse metadata from file.
    void parseMetadata(); // New controlling function over legacy ones.
    
    /// File stream containing binary file data.
    std::ifstream m_fileStream;
  
  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    void init();
    void exec();
    

  private:

    /*==================================================================================
    Region: Declarations and Definitions in the following region are candidates for refactoring. Copied from MD_FileHoraceReader
    ==================================================================================*/
    
    
    void parse_sqw_main_header(); //Legacy - candidate for removal
    std::streamoff parse_sqw_detpar(std::streamoff start_location); //Legacy - candidate for removal
    std::streamoff parse_component_header(std::streamoff start_location); //Legacy -candidate for removal
    void parse_data_locations(std::streamoff data_start); //Legacy - candidate for removal

    
    /*Helper type lifted from MD_FileHoraceReader, TODO. Replace.*/
    struct data_positions
    {
      std::streamoff  if_sqw_start;
      std::streamoff  n_dims_start;
      std::streamoff  sqw_header_start;
      std::vector<std::streamoff> component_headers_starts;
      std::streamoff detectors_start;
      std::streamoff   data_start;
      std::streamoff   geom_start;
      std::streamoff   npax_start;
      std::streamoff   s_start;
      std::streamoff   err_start;
      std::streamoff   n_cell_pix_start;
      std::streamoff   min_max_start;
      std::streamoff   pix_start;
      /// Default Constructor
      data_positions():if_sqw_start(18),n_dims_start(22),sqw_header_start(26),
        detectors_start(0),data_start(0),geom_start(0),s_start(0), // the following values have to be identified from the file itself
        err_start(0),
        n_cell_pix_start(0),min_max_start(0),pix_start(0){}; // the following values have to be identified from the file itself
    };

    /// Instance of helper type.
    data_positions m_dataPositions;

    size_t m_nDataPoints;
    size_t m_mdImageSize;
    size_t m_nDims;
    /// number of bins in every non-integrated dimension
    std::vector<size_t> m_nBins;

    /*==================================================================================
    End Region
    ==================================================================================*/
  };

} // namespace Mantid
} // namespace MDEvents

#endif  /* MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACE_H_ */
