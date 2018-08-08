#ifndef MANTID_DATAHANDLING_LOADLLB_H_
#define MANTID_DATAHANDLING_LOADLLB_H_

#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/LoadHelper.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/System.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid {
namespace DataHandling {

/** LoadLLB : Loads an LLB MIBEMOL TOF NeXus file into a Workspace2D with the
  given name.

  Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport LoadLLB : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  LoadLLB();

  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Loads LLB nexus file."; }

  int version() const override;
  const std::string category() const override;

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;

private:
  void init() override;
  void exec() override;
  void setInstrumentName(NeXus::NXEntry &entry);
  void initWorkSpace(NeXus::NXEntry &);
  void loadTimeDetails(NeXus::NXEntry &entry);
  void loadDataIntoTheWorkSpace(NeXus::NXEntry &);
  int getDetectorElasticPeakPosition(const NeXus::NXFloat &);
  void setTimeBinning(HistogramData::HistogramX &histX, int, double);
  /// Calculate error for y
  static double calculateError(double in) { return sqrt(in); }
  void loadExperimentDetails(NeXus::NXEntry &);
  void loadRunDetails(NeXus::NXEntry &);
  void runLoadInstrument();

  std::vector<std::string> m_supportedInstruments;
  std::string m_instrumentName;
  std::string m_instrumentPath; ///< Name of the instrument path

  API::MatrixWorkspace_sptr m_localWorkspace;
  size_t m_numberOfTubes;         // number of tubes - X
  size_t m_numberOfPixelsPerTube; // number of pixels per tube - Y
  size_t m_numberOfChannels;      // time channels - Z
  size_t m_numberOfHistograms;
  double m_wavelength;
  double m_channelWidth;

  LoadHelper m_loader;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADLLB_H_ */
