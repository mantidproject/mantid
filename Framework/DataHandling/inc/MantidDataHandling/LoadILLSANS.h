#ifndef MANTID_DATAHANDLING_LOADILLSANS_H_
#define MANTID_DATAHANDLING_LOADILLSANS_H_

#include "MantidKernel/System.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidNexus/NexusClasses.h"
#include "MantidDataHandling/LoadHelper.h"

namespace Mantid {
namespace DataHandling {

/** LoadILLSANS; supports D11, D22 and D33 (TOF/monochromatic)

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

class DLLExport LoadILLSANS : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  LoadILLSANS();
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads a ILL nexus files for SANS instruments D11, D22, D33.";
  }
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"LoadNexus"};
  }
  const std::string category() const override;
  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::NexusDescriptor &descriptor) const override;

private:
  struct DetectorPosition {
    double distanceSampleRear;
    double distanceSampleBottomTop;
    double distanceSampleRightLeft;
    double shiftLeft;
    double shiftRight;
    double shiftUp;
    double shiftDown;
    void operator>>(std::ostream &strm) {
      strm << "DetectorPosition : "
           << "distanceSampleRear = " << distanceSampleRear << ", "
           << "distanceSampleBottomTop = " << distanceSampleBottomTop << ", "
           << "distanceSampleRightLeft = " << distanceSampleRightLeft << ", "
           << "shiftLeft = " << shiftLeft << ", "
           << "shiftRight = " << shiftRight << ", "
           << "shiftUp = " << shiftUp << ", "
           << "shiftDown = " << shiftDown << '\n';
    }
  };

  void init() override;
  void exec() override;
  void setInstrumentName(const NeXus::NXEntry &, const std::string &);
  DetectorPosition getDetectorPositionD33(const NeXus::NXEntry &,
                                          const std::string &);

  void initWorkSpace(NeXus::NXEntry &, const std::string &);
  void initWorkSpaceD33(NeXus::NXEntry &, const std::string &);
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
  void moveDetectorsD33(const DetectorPosition &);
  void moveDetectorDistance(double, const std::string &);
  void moveDetectorHorizontal(double, const std::string &);
  void moveDetectorVertical(double, const std::string &);
  Kernel::V3D getComponentPosition(const std::string &componentName);
  void loadMetaData(const NeXus::NXEntry &, const std::string &);
  std::string getInstrumentFilePath(const std::string &) const;

  LoadHelper m_loader;  
  std::string m_instrumentName; ///< Name of the instrument
  std::vector<std::string> m_supportedInstruments;
  API::MatrixWorkspace_sptr m_localWorkspace;
  std::vector<double> m_defaultBinning;
  std::string m_resMode; ///< Resolution mode for D11

  double calculateQ(const double lambda, const double twoTheta) const;
  std::pair<double, double> calculateQMaxQMin();
  void setFinalProperties();
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADILLSANS_H_ */
