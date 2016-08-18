#ifndef SCDCALIBRATEPANELS_H_
#define SCDCALIBRATEPANELS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/Quat.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include <boost/lexical_cast.hpp>

namespace Mantid {
namespace Crystal {
/** SCDCalibratePanels calibrates instrument parameters for Rectangular
 Detectors

 *  @author Ruth Mikkelson(adapted from Isaw's Calibration code)
 *  @date   Mar 12, 2012
 *
 *  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory &
 *                   NScD Oak Ridge National Laboratory
 *
 *  This file is part of Mantid.
 *
 *  Mantid is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Mantid is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  File change history is stored at:
 *   <https://github.com/mantidproject/mantid>
 *   Code Documentation is available at: <http://doxygen.mantidproject.org>
 */

class SCDCalibratePanels : public Mantid::API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Panel parameters and L0 are optimized to "
           "minimize errors between theoretical and actual q values for the "
           "peaks";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override;

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override;

  /**
   *  Refactors a rotation Q as a Rotation in x dir by Rotx * a Rotation in the
   * y dir by Roty
   *                                     * a rotation in the z direction by Rotz
   *  @param Q  A rotation( a copy will be normalized)
   *  @param Rotx       The angle in degrees for the rotation in x direction
   *  @param Roty       The angle in degrees for the rotation in y direction
   *  @param Rotz      The angle in degrees for the rotation in z direction
   */
  static void Quat2RotxRotyRotz(const Kernel::Quat Q, double &Rotx,
                                double &Roty, double &Rotz);

  /**
   *  Updates the ParameterMap for NewInstrument to reflect the changes in the
   *  associated panel information
   *
   *  @param bankNames      The names of the banks(panels) that will be updated
   *
   *  @param NewInstrument  The instrument whose parameter map will be changed
   *                         to reflect the new values below
   *
   *  @param  pos           The quantity to be added to the current relative
   *                        position, from old NewInstrument, of the banks in
   *bankNames.
   *  @param rot            The quantity to be added to the current relative
   *                        rotations, from old NewInstrument, of the banks in
   *bankNames.
   *
   *  @param  DetWScale     The factor to multiply the current detector width,
   *                        from old NewInstrument, by to get the new detector
   *width
   *                        for the banks in bankNames.
   *
   *  @param   DetHtScale  The factor to multiply the current detector height,
   *                        from old NewInstrument, by to get the new detector
   *height
   *                        for the banks in bankNames.
   *
   *   @param  pmapOld      The Parameter map from the original instrument( not
   *                        NewInstrument). "Clones" relevant information into
   *the
   *                        NewInstrument's parameter map.
   *
   *   @param RotCenters Rotate the centers of the panels(the same amount)
   *with the
   *                        rotation of panels around their center
   */
  static void FixUpBankParameterMap(
      std::vector<std::string> const bankNames,
      boost::shared_ptr<const Geometry::Instrument> NewInstrument,
      Kernel::V3D const pos, Kernel::Quat const rot, double const DetWScale,
      double const DetHtScale,
      boost::shared_ptr<const Geometry::ParameterMap> const pmapOld,
      bool RotCenters);

  /**
   * *  Updates the ParameterMap for NewInstrument to reflect the position of
  *the
   * source.
   *
   * @param NewInstrument  The instrument whose parameter map will be changed
  *                         to reflect the new source position
  *
   * @param L0             The distance from source to sample( should be
  *positive)
   *
   * @param newSampPos     The  new sample position
   *
   * @param  pmapOld     The Parameter map from the original instrument( not
  *                        NewInstrument). "Clones" relevant information into
  *the
  *                        NewInstrument's parameter map.
   */
  static void FixUpSourceParameterMap(
      boost::shared_ptr<const Geometry::Instrument> NewInstrument,
      double const L0, Kernel::V3D const newSampPos,
      boost::shared_ptr<const Geometry::ParameterMap> const pmapOld);

  /**
   *  Copies positional entries in pmapSv to pmap starting at bank_const
   *  and parents.
   *
   *  @param  bank_const  the starting component for copying entries.
   *
   *  @param pmap         the Parameter Map to be updated
   *
   *  @param pmapSv       the original Parameter Map
   *
   */
  static void
  updateBankParams(boost::shared_ptr<const Geometry::IComponent> bank_const,
                   boost::shared_ptr<Geometry::ParameterMap> pmap,
                   boost::shared_ptr<const Geometry::ParameterMap> pmapSv);

  /**
   *  Copies positional entries in pmapSv to pmap starting at bank_const
   *  and parents.
   *  @param  bank_const  the starting component for copying entries.
   *  @param pmap        the Parameter Map to be updated
   *  @param pmapSv       the original Parameter Map
   *
   */
  static void
  updateSourceParams(boost::shared_ptr<const Geometry::IComponent> bank_const,
                     boost::shared_ptr<Geometry::ParameterMap> pmap,
                     boost::shared_ptr<const Geometry::ParameterMap> pmapSv);

  void LoadISawDetCal(boost::shared_ptr<const Geometry::Instrument> &instrument,
                      boost::container::flat_set<std::string> &AllBankName,
                      double &T0, double &L0, std::string filename,
                      std::string bankPrefixName);

private:
  void saveIsawDetCal(boost::shared_ptr<Geometry::Instrument> &instrument,
                      boost::container::flat_set<std::string> &AllBankName,
                      double T0, std::string filename);

  void createResultWorkspace(const int numGroups, const int colNum,
                             const std::vector<std::string> &names,
                             const std::vector<double> &params,
                             const std::vector<double> &errs);
  /// Function to find peaks near detector edge
  bool edgePixel(DataObjects::PeaksWorkspace_sptr ws, std::string bankName,
                 int col, int row, int Edge);
  /// Function to calculate U
  void findU(DataObjects::PeaksWorkspace_sptr peaksWs);
  /// save workspaces
  void saveNexus(std::string outputFile, API::MatrixWorkspace_sptr outputWS);
  /// Function to optimize L1
  void findL1(int nPeaks, DataObjects::PeaksWorkspace_sptr peaksWs);

  void exec() override;

  void init() override;

  API::ITableWorkspace_sptr Result;

  /**
   * Saves the new instrument to an xml file that can be used with the
   * LoadParameterFile Algorithm. If the filename is empty, nothing gets done.
   *
   * @param FileName     The filename to save this information to
   *
   * @param AllBankNames The names of the banks in each group whose values are
   *                         to be saved to the file
   *
   * @param instrument   The instrument with the new values for the banks in
   *Groups
   */
  void saveXmlFile(std::string const FileName,
                   boost::container::flat_set<std::string> const AllBankNames,
                   Geometry::Instrument_const_sptr const instrument) const;
};

} // namespace Crystal
} // namespace Mantid

#endif /* SCDCALIBRATEPANELS_H_ */
