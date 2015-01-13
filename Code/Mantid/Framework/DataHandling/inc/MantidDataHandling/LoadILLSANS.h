#ifndef MANTID_DATAHANDLING_LOADILLSANS_H_
#define MANTID_DATAHANDLING_LOADILLSANS_H_

#include "MantidKernel/System.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidNexus/NexusClasses.h"
#include "MantidDataHandling/LoadHelper.h"

namespace Mantid {
namespace DataHandling {

/** LoadILLSANS

 To date this only supports ILL D33 SANS-TOF instrument

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

struct DetectorPosition {
  double distanceSampleRear;
  double distanceSampleBottomTop;
  double distanceSampleRightLeft;
  double shiftLeft;
  double shiftRight;
  double shiftUp;
  double shiftDown;
};

std::ostream &operator<<(std::ostream &strm, const DetectorPosition &p) {
  return strm << "DetectorPosition : "
              << "distanceSampleRear = " << p.distanceSampleRear << ", "
              << "distanceSampleBottomTop = " << p.distanceSampleBottomTop
              << ", "
              << "distanceSampleRightLeft = " << p.distanceSampleRightLeft
              << ", "
              << "shiftLeft = " << p.shiftLeft << ", "
              << "shiftRight = " << p.shiftRight << ", "
              << "shiftUp = " << p.shiftUp << ", "
              << "shiftDown = " << p.shiftDown << std::endl;
}

class DLLExport LoadILLSANS : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  LoadILLSANS();
  virtual ~LoadILLSANS();

  virtual const std::string name() const;
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Loads a ILL nexus files for SANS instruments.";
  }

  virtual int version() const;
  virtual const std::string category() const;
  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const;

private:
  void init();
  void exec();
  void setInstrumentName(const NeXus::NXEntry &, const std::string &);
  DetectorPosition getDetectorPosition(const NeXus::NXEntry &,
                                       const std::string &);
  void initWorkSpace(NeXus::NXEntry &, const std::string &);
  void createEmptyWorkspace(int, int);

  size_t loadDataIntoWorkspaceFromMonitors(NeXus::NXEntry &firstEntry,
                                           size_t firstIndex = 0);

  size_t loadDataIntoWorkspaceFromHorizontalTubes(NeXus::NXInt &,
                                                  const std::vector<double> &,
                                                  size_t);
  size_t loadDataIntoWorkspaceFromVerticalTubes(NeXus::NXInt &,
                                                const std::vector<double> &,
                                                size_t);
  void runLoadInstrument();
  void moveDetectors(const DetectorPosition &);
  void moveDetectorDistance(double, const std::string &);
  void moveDetectorHorizontal(double, const std::string &);
  void moveDetectorVertical(double, const std::string &);
  Kernel::V3D getComponentPosition(const std::string &componentName);
  void loadMetaData(const NeXus::NXEntry &, const std::string &);

  LoadHelper m_loader;
  std::string m_instrumentName; ///< Name of the instrument
  std::vector<std::string> m_supportedInstruments;
  API::MatrixWorkspace_sptr m_localWorkspace;
  std::vector<double> m_defaultBinning;

  double calculateQ(const double lambda, const double twoTheta) const;
  std::pair<double, double> calculateQMaxQMin();
  void setFinalProperties();
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADILLSANS_H_ */
