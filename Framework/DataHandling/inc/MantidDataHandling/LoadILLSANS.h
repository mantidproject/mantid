#ifndef MANTID_DATAHANDLING_LOADILLSANS_H_
#define MANTID_DATAHANDLING_LOADILLSANS_H_

#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/LoadHelper.h"
#include "MantidKernel/System.h"
#include "MantidNexus/NexusClasses.h"

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
  const std::string summary() const override;
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
  void createEmptyWorkspace(const size_t, const size_t);

  size_t loadDataIntoWorkspaceFromMonitors(NeXus::NXEntry &firstEntry,
                                           size_t firstIndex = 0);
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
  void rotateD22(double, const std::string &);
  void adjustTOF();
  void moveSource();

  LoadHelper m_loader;          ///< Load helper for metadata
  std::string m_instrumentName; ///< Name of the instrument
  std::vector<std::string>
      m_supportedInstruments;                 ///< List of supported instruments
  API::MatrixWorkspace_sptr m_localWorkspace; ///< to-be output workspace
  std::vector<double> m_defaultBinning;       ///< the default x-axis binning
  std::string m_resMode; ///< Resolution mode for D11 and D22
  bool m_isTOF;          ///< TOF or monochromatic flag
  double m_sourcePos;    ///< Source Z (for D33 TOF)

  void setFinalProperties(const std::string &filename);
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADILLSANS_H_ */
