#ifndef MANTID_MDEVENTS_LOAD_SQW_H_
#define MANTID_MDEVENTS_LOAD_SQW_H_

#include "MantidAPI/IFileLoader.h"
#include "MantidKernel/FileDescriptor.h"
#include "MantidAPI/Progress.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidGeometry/MDGeometry/MDHistoDimensionBuilder.h"
#include <fstream>
#include <string>

namespace Mantid {
namespace MDAlgorithms {

/** LoadSQW :
 * Load an SQW file and read observations in as events to generate a
 *IMDEventWorkspace, with events in reciprocal space (Qx, Qy, Qz)
 *
 * @author Owen Arnold, Tessella, ISIS
 * @date 12/July/2011
 */
/*==================================================================================
   Region: Declarations and Definitions in the following region are candidates
   for refactoring. Copied from MD_FileHoraceReader
   ==================================================================================*/
namespace LoadSQWHelper {
/*Helper type lifted from MD_FileHoraceReader,
  The structure describes the positions of the different sqw data parts in the
  total binary sqw data file
  TODO. Replace.*/
struct dataPositions {
  std::streamoff if_sqw_start;
  std::streamoff n_dims_start;
  std::streamoff sqw_header_start;
  std::vector<std::streamoff> component_headers_starts;
  std::streamoff detectors_start;
  std::streamoff data_start;
  std::streamoff geom_start;
  std::streamoff npax_start;
  std::streamoff s_start;
  std::streamoff err_start;
  std::streamoff n_cell_pix_start; // where data.npix  array starts
  std::streamoff min_max_start; // data range positions (uRange -- this is the
                                // data which describe the extents of the
                                // MDPixesl (events))
  std::streamoff pix_start;     //< event data positions
  size_t mdImageSize; //< the variable describes the size of Horace MD image
                      //(number of pixels in Horace DND object or number of bins
  // in Mantid Histo workspace)

  /// Default Constructor
  dataPositions()
      : if_sqw_start(18), n_dims_start(22), sqw_header_start(26),
        component_headers_starts(), detectors_start(0), data_start(0),
        geom_start(0), npax_start(0), s_start(0),
        // the following values have to be identified from the file
        // itself
        err_start(0), n_cell_pix_start(0), min_max_start(0), pix_start(0),
        mdImageSize(0){}; // the following values have to be identified from the
                          // file itself

  // the helper methods
  /// Block 1:  Main_header: Parse SQW main data header
  void parse_sqw_main_header(
      std::ifstream &stream); // Legacy - candidate for removal
  /// Block 2: Header: Parse header of single SPE file
  std::streamoff parse_component_header(
      std::ifstream &stream,
      std::streamoff start_location); // Legacy -candidate for removal
  /// Block 3: Detpar: parse positions of the contributed detectors. These
  /// detectors have to be the same for all contributing spe files
  std::streamoff parse_sqw_detpar(
      std::ifstream &stream,
      std::streamoff start_location); // Legacy - candidate for removal
  /// Block 4: Data: parse positions of the data fields
  void
  parse_data_locations(std::ifstream &stream, std::streamoff data_start,
                       std::vector<size_t> &nBins,
                       uint64_t &nDataPoints); // Legacy - candidate for removal
};
}

class DLLExport LoadSQW : public API::IFileLoader<Kernel::FileDescriptor> {
public:
  LoadSQW();
  ~LoadSQW();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "LoadSQW"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Create a MDEventWorkspace with events in reciprocal space (Qx, Qy, "
           "Qz, Energy) from a SQW file.";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 1; };
  /// Algorithm's category for identification
  virtual const std::string category() const {
    return "DataHandling;MDAlgorithms";
  }

  /// Returns a confidence value that this algorithm can load a file
  virtual int confidence(Kernel::FileDescriptor &descriptor) const;

private:
  void init();
  void exec();

protected: // for testing
  /// Read events onto the workspace.
  virtual void readEvents(
      Mantid::MDEvents::MDEventWorkspace<MDEvents::MDEvent<4>, 4> *ws);

  /// Read DND dimensions.
  void readDNDDimensions(
      std::vector<Mantid::Geometry::MDHistoDimensionBuilder> &DimVectorOut,
      bool arrangeByMDImage = true);
  /// Read SQW dimensions
  void readSQWDimensions(
      std::vector<Mantid::Geometry::MDHistoDimensionBuilder> &DimVectorOut);

  /// Extract lattice information
  virtual void addLattice(
      Mantid::MDEvents::MDEventWorkspace<MDEvents::MDEvent<4>, 4> *ws);

  /// Parse metadata from file.
  void parseMetadata(const std::string &fileName); // New controlling function
                                                   // over legacy ones.
  /// build an initial range of 4 dimensions
  void buildMDDimsBase(
      std::vector<Mantid::Geometry::MDHistoDimensionBuilder> &DimVector);
  /// add range of dimensions to the workspace;
  void addDimsToWs(
      Mantid::MDEvents::MDEventWorkspace<MDEvents::MDEvent<4>, 4> *ws,
      std::vector<Mantid::Geometry::MDHistoDimensionBuilder> &DimVector);

  /// read real box sizes for all Horace Boxes;
  void readBoxSizes();

  /// the name of the file to work with
  std::string m_fileName;
  /// File stream containing binary file data.
  std::ifstream m_fileStream;

  /// Progress bar
  Mantid::API::Progress *m_prog;

  /// OutputFilename param
  std::string m_outputFile;

  /// Instance of helper type, which describes the positions of the data within
  /// binary Horace file
  LoadSQWHelper::dataPositions m_dataPositions;
  // the array of horace boxe sizes;
  std::vector<uint64_t> m_boxSizes;

  uint64_t m_nDataPoints;
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

#endif /* MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACE_H_ */
