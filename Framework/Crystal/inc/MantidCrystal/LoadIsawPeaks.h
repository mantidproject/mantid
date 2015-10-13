#ifndef MANTID_CRYSTAL_LOADISAWPEAKS_H_
#define MANTID_CRYSTAL_LOADISAWPEAKS_H_

#include "MantidKernel/System.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidDataObjects/PeaksWorkspace.h"

namespace Mantid {
namespace Crystal {

/** LoadIsawPeaks : Load an ISAW-style .peaks file
 * into a PeaksWorkspace
 *
 * @author Janik Zikovsky, SNS
 * @date 2011-03-07 15:22:11.897153
 */
class DLLExport LoadIsawPeaks
    : public API::IFileLoader<Kernel::FileDescriptor> {
public:
  LoadIsawPeaks();
  virtual ~LoadIsawPeaks();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "LoadIsawPeaks"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Load an ISAW-style .peaks file into a PeaksWorkspace.";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 1; };
  /// Algorithm's category for identification
  virtual const std::string category() const {
    return "Crystal;DataHandling\\Isaw";
  }

  /// Returns a confidence value that this algorithm can load a file
  virtual int confidence(Kernel::FileDescriptor &descriptor) const;

private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();

  std::string ApplyCalibInfo(std::ifstream &in, std::string startChar,
                             Geometry::Instrument_const_sptr instr_old,
                             Geometry::Instrument_const_sptr instr, double &T0);

  std::string readHeader(Mantid::DataObjects::PeaksWorkspace_sptr outWS,
                         std::ifstream &in, double &T0);

  DataObjects::Peak readPeak(DataObjects::PeaksWorkspace_sptr outWS,
                             std::string &lastStr, std::ifstream &in,
                             int &seqNum, std::string bankName);

  int findPixelID(Geometry::Instrument_const_sptr inst, std::string bankName,
                  int col, int row);

  std::string readPeakBlockHeader(std::string lastStr, std::ifstream &in,
                                  int &run, int &detName, double &chi,
                                  double &phi, double &omega, double &monCount);

  void appendFile(Mantid::DataObjects::PeaksWorkspace_sptr outWS,
                  std::string filename);
  void checkNumberPeaks(Mantid::DataObjects::PeaksWorkspace_sptr outWS,
                        std::string filename);

  /// Local cache of bank IComponents used in file
  std::map<std::string, boost::shared_ptr<const Geometry::IComponent>> m_banks;

  /// Retrieve cached bank (or load and cache for next time)
  boost::shared_ptr<const Geometry::IComponent> getCachedBankByName(std::string bankname, const boost::shared_ptr<const Geometry::Instrument>& inst);

};

} // namespace Mantid
} // namespace Crystal

#endif /* MANTID_CRYSTAL_LOADISAWPEAKS_H_ */
