#ifndef MANTID_DATAHANDLING_LOADILLREFLECTOMETRY_H_
#define MANTID_DATAHANDLING_LOADILLREFLECTOMETRY_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

#include "MantidAPI/IFileLoader.h"
#include "MantidNexus/NexusClasses.h"
#include "MantidDataHandling/LoadHelper.h"

namespace Mantid {
namespace DataHandling {

/** LoadILLReflectometry : Loads a ILL Reflectometry data file.

 Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
 National Laboratory & European Spallation Source

 This file is part of Mantid.

 Mantid is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 Mantid is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 File change history is stored at: <https://github.com/mantidproject/mantid>
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport LoadILLReflectometry
    : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  LoadILLReflectometry();
  virtual ~LoadILLReflectometry();
  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const;

  virtual const std::string name() const;
  virtual int version() const;
  virtual const std::string category() const;

  virtual const std::string summary() const {
    return "Loads a ILL/D17 nexus file.";
  }

private:
  void init();
  void exec();

  void initWorkSpace(NeXus::NXEntry &entry,
                     std::vector<std::vector<int>> monitorsData);
  void setInstrumentName(const NeXus::NXEntry &firstEntry,
                         const std::string &instrumentNamePath);
  void loadDataDetails(NeXus::NXEntry &entry);
  void loadDataIntoTheWorkSpace(NeXus::NXEntry &entry,
                                std::vector<std::vector<int>> monitorsData);
  void loadNexusEntriesIntoProperties(std::string nexusfilename);
  std::vector<std::vector<int>> loadMonitors(NeXus::NXEntry &entry);
  void runLoadInstrument();
  void centerDetector(double);
  void placeDetector(double, double);

  API::MatrixWorkspace_sptr m_localWorkspace;

  std::string m_instrumentName; ///< Name of the instrument

  size_t m_numberOfTubes;         // number of tubes - X
  size_t m_numberOfPixelsPerTube; // number of pixels per tube - Y
  size_t m_numberOfChannels;      // time channels - Z

  size_t m_numberOfHistograms;

  /* Values parsed from the nexus file */
  double m_wavelength;
  double m_channelWidth;

  std::vector<std::string> m_supportedInstruments;
  LoadHelper m_loader;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADILLREFLECTOMETRY_H_ */
