#ifndef DATAHANDING_SAVEOPENGENIEASCII_H_
#define DATAHANDING_SAVEOPENGENIEASCII_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid {
namespace DatHandling {

class DLLExport SaveOpenGenieAscii : public Mantid::API::Algorithm {
public:
  /// (Empty) Constructor
  SaveOpenGenieAscii();

  //// Virtual destructor
  virtual ~SaveOpenGenieAscii() {}

  /// Algorithm's name
  virtual const std::string name() const { return "SaveOpenGenieAscii"; }

  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Saves a focused data set into an OpenGenie ASCII file "
           "ascii file of the given workspace . There is no "
           "instrument assosciated with the resulting workspace. ";
  }

  /// Algorithm's version
  virtual int version() const { return (1); }

  /// Algorithm's category for identification
  virtual const std::string category() const {
    return "Diffraction;DataHandling\\Text";
  }




private:
	
	/// Initialisation code
  void init();

  /// Execution code
  void exec();

  /// Get the focused position for the supplied spectrum
//  void getFocusedPos(MatrixWorkspace_const_sptr wksp, const int spectrum,
  //                 double &l1, double &l2, double &tth, double &difc);



  /*

  /// Write the header information in default "XYE" format
  void writeXYEHeaders(std::ostream &os,
                       API::MatrixWorkspace_const_sptr &workspace) const;

  /// Write spectra XYE header
  void writeXYESpectraHeader(std::ostream &os, size_t index1, size_t index2,
                             double flightPath, double tth,
                             const std::string &caption);

  /// Determine the focused position for the supplied spectrum.
  void getFocusedPos(API::MatrixWorkspace_const_sptr wksp,
                     const size_t spectrum, double &l1, double &l2,
                     double &tth);

  /// sets non workspace properties for the algorithm
  void setOtherProperties(IAlgorithm *alg, const std::string &propertyName,
                          const std::string &propertyValue, int perioidNum);

  /// Header type
  HeaderType m_headerType;  */

};
}
}
#endif // DATAHANDING_SAVEOPENGENIEASCII_H_ 
