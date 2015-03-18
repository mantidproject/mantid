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
#include "MantidDataObjects/Workspace2D.h"

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
  SCDCalibratePanels();

  virtual ~SCDCalibratePanels();

  virtual const std::string name() const;
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Panel parameters, sample position,L0 and T0 are optimized to "
           "minimize errors between theoretical and actual q values for the "
           "peaks";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const;

  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const;

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
   *   @param RotateCenters Rotate the centers of the panels(the same amount)
   *with the
   *                        rotation of panels around their center
   */
  static void FixUpBankParameterMap(
      std::vector<std::string> const bankNames,
      boost::shared_ptr<const Geometry::Instrument> NewInstrument,
      Kernel::V3D const pos, Kernel::Quat const rot, double const DetWScale,
      double const DetHtScale,
      boost::shared_ptr<const Geometry::ParameterMap> const pmapOld,
      bool RotateCenters);

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
   * Given a string representation of a set of groups( [] separated list of
   * bank nums separated by commas or colons( for ranges) , this method will
   *  produce a vector of "groups"( vector of bank names). All bank names
   *  will be members of AllBankNames and only used one time.
   *
   * @param AllBankNames  -The list of all banks to use
   *
   * @param Grouping      -Grouping mode (  OnePanelPerGroup,
   *AllPanelsInOneGroup,
   *                        or SpecifyGroups)
   *
   * @param bankPrefix    -For SpecifyGroups,the prefix to be affixed to each
   *                             integer in the bankingCode
   *
   * @param bankingCode   -A [] separated list of banknums. Between the [..], *
   *                      the bank nums can be separted by commas or : for
   *lists.
   *
   * @param Groups       -Contains the result, a vector of vectors of bank
   *names.
   *
   */
  void CalculateGroups(std::set<std::string> &AllBankNames,
                       std::string Grouping, std::string bankPrefix,
                       std::string bankingCode,
                       std::vector<std::vector<std::string>> &Groups);

  /**
   * Calculate the Workspace2D associated with a Peaksworkspace for Composite
   *functions.
   * the elements of parameter bounds can be used to calculate Xstart and Xend
   *
   * @param pwks        The PeaksWorkspace
   *
   * @param  bankNames  The list of bank names( from Peak.getBankName())
   *
   * @param tolerance   The maximum distance the h value, k value, and l value
   *of a Peak is from
   *                    an integer, for a peak to be considered Indexed.
   *
   * @param  bounds     bounds[i] is the starting index for the xvalues from the
   *resultant workspace.
   *                    This can be used to determine startX and endX.
   */
  DataObjects::Workspace2D_sptr
  calcWorkspace(DataObjects::PeaksWorkspace_sptr &pwks,
                std::vector<std::string> &bankNames, double tolerance,
                std::vector<int> &bounds);

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
                      std::set<std::string> &AllBankName, double &T0,
                      double &L0, std::string filename,
                      std::string bankPrefixName);

private:
  void saveIsawDetCal(boost::shared_ptr<const Geometry::Instrument> &instrument,
                      std::set<std::string> &AllBankName, double T0,
                      std::string filename);

  void createResultWorkspace(const int numGroups,
                             const std::vector<std::string> &names,
                             const std::vector<double> &params,
                             const std::vector<double> &errs);

  void exec();

  void init();

  /**
   * Creates a new instrument when a calibration file( .xml or .detcal)
   * is loaded
   *
   * @param instrument   The old instrument
   * @param preprocessCommand  either "No PreProcessing",
   *                                  "Apply a ISAW.DetCal File",or
   *                                  "Apply a LoadParameter.xml type file"
   * @param preprocessFilename  Filename is one of the preprocessCommand
   *                            indicates use of  a file
   * @param timeOffset  The timeoffset to use
   * @param L0          The initial flight path
   * @param AllBankNames  The names of all the banks that wiil be processed.
   */
  boost::shared_ptr<const Geometry::Instrument> GetNewCalibInstrument(
      boost::shared_ptr<const Geometry::Instrument> instrument,
      std::string preprocessCommand, std::string preprocessFilename,
      double &timeOffset, double &L0, std::vector<std::string> &AllBankNames);

  /**
   * Calculates the initial values for all the parameters.  This is needed if
   * when preprocessing is done( load in a calibration file before starting)
   *
   * @param  bank_rect   a bank in the instrument
   * @param instrument   The old instrument
   * @param PreCalibinstrument  the precalibrated instrument
   * @param detWidthScale0  The initial scaling on the panel width
   * @param detHeightScale0  The initial scaling on the panel height
   * @param Xoffset0         The initial X offset of the center of the panel
   * @param Yoffset0         The initial Y offset of the center of the panel
   * @param Zoffset0         The initial Z offset of the center of the panel
   * @param Xrot0            The initial relative rotation about the  x-axis
   *                                  around the center of the panel
   * @param Yrot0            The initial relative rotation about the  y-axis
   *                                  around the center of the panel
   * @param Zrot0            The initial relative rotation about the  z-axis
   *                                  around the center of the panel
   *
   */
  void CalcInitParams(Geometry::RectangularDetector_const_sptr bank_rect,
                      Geometry::Instrument_const_sptr instrument,
                      Geometry::Instrument_const_sptr PreCalibinstrument,
                      double &detWidthScale0, double &detHeightScale0,
                      double &Xoffset0, double &Yoffset0, double &Zoffset0,
                      double &Xrot0, double &Yrot0, double &Zrot0);

  /**
   * Creates the function and gets values using the current  values for the
   * parameters and Attributes
   *
   * @param ws        The workspace with the predicted qx,qy, and qz values for
   *each
   *                   peak
   *
   * @param NGroups   The number of Groups-Sets of panels
   * @param names     The names of the variables that have been fit
   *
   * @param params    The values of the variables that have been fit
   * @param BankNameString  The list of all banks to be refined, separated by
   *                           !(Group separator) or /
   *
   * @param out        The result of this function call. It is the error in
   *                                  qx,qy,and qz for each peak
   *
   * @param xVals      The xVals from ws. Here it should be the peak index for
   *the
   *                    corresponding  qx, qy, or qz
   *
   * @param nData      The number of xVals and out values
   */
  void CreateFxnGetValues(DataObjects::Workspace2D_sptr const ws,
                          int const NGroups,
                          std::vector<std::string> const names,
                          std::vector<double> const params,
                          std::string const BankNameString, double *out,
                          const double *xVals, const size_t nData) const;

  /**
   * Saves the new instrument to an xml file that can be used with the
   * LoadParameterFile Algorithm. If the filename is empty, nothing gets done.
   *
   * @param FileName     The filename to save this information to
   *
   * @param Groups      The names of the banks in each group whose values are
   *                         to be saved to the file
   *
   * @param instrument   The instrument with the new values for the banks in
   *Groups
   */
  void saveXmlFile(std::string const FileName,
                   std::vector<std::vector<std::string>> const Groups,
                   Geometry::Instrument_const_sptr const instrument) const;
};

} // namespace Crystal
} // namespace Mantid

#endif /* SCDCALIBRATEPANELS_H_ */
