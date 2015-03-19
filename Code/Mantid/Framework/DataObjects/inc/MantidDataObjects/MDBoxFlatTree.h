#ifndef MDBOX_FLAT_TREE_H_
#define MDBOX_FLAT_TREE_H_

#include "MantidKernel/Matrix.h"
#include "MantidAPI/BoxController.h"
#include "MantidDataObjects/MDBox.h"
#include "MantidDataObjects/MDGridBox.h"
#include "MantidDataObjects/MDEventWorkspace.h"

namespace Mantid {
namespace DataObjects {
//===============================================================================================
/** The class responsible for saving/loading MD boxes structure to/from HDD and
 for flattening/restoring
 *  the interconnected box structure (customized linked list) of  MD workspace
 *
 * @date March 21, 2013
 *
   Copyright &copy; 2007-2013 ISIS Rutherford Appleton Laboratory, NScD Oak
 Ridge National Laboratory & European Spallation Source

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

  File change history is stored at: <https://github.com/mantidproject/mantid>.
  Code Documentation is available at: <http://doxygen.mantidproject.org>
 * */

class DLLExport MDBoxFlatTree {
public:
  /**The constructor of the flat box tree    */
  MDBoxFlatTree();

  /**@return XML description of the workspace box controller */
  const std::string &getBCXMLdescr() const { return m_bcXMLDescr; }

  //---------------------------------------------------------------------------------------------------------------------
  /**@return internal linearized box structure of md workspace. Defined only
   * when the class is properly initiated*/
  std::vector<API::IMDNode *> &getBoxes() { return m_Boxes; }
  /**@return number of boxes */
  size_t getNBoxes() const { return m_BoxType.size(); }
  /**@return the vector of data which describes signals and errors over boxes */
  std::vector<double> &getSigErrData() { return m_BoxSignalErrorsquared; }
  /**@return the vector of data which describes signals and errors locations on
   * file */
  std::vector<uint64_t> &getEventIndex() { return m_BoxEventIndex; }
  const std::vector<int> &getBoxType() const { return m_BoxType; }

  //---------------------------------------------------------------------------------------------------------------------
  /// convert MDWS box structure into flat structure used for saving/loading on
  /// hdd
  void initFlatStructure(API::IMDEventWorkspace_sptr pws,
                         const std::string &fileName);

  uint64_t restoreBoxTree(std::vector<API::IMDNode *> &Boxes,
                          API::BoxController_sptr &bc, bool FileBackEnd,
                          bool NoFileInfo = false);

  /*** this function tries to set file positions of the boxes to
        make data physically located close to each other to be as close as
     possible on the HDD */
  void setBoxesFilePositions(bool setFileBacked);

  /**Save flat box structure into a file, defined by the file name*/
  void saveBoxStructure(const std::string &fileName);
  void loadBoxStructure(const std::string &fileName, int &nDim,
                        const std::string &EventType,
                        bool onlyEventInfo = false,
                        bool restoreExperimentInfo = false);

  /**Export existing experiment info defined in the box structure to target
   * workspace (or other experiment info) */
  void exportExperiment(Mantid::API::IMDEventWorkspace_sptr &targetWS);

  /// Return number of dimensions this class is initiated for (or not initiated
  /// if -1)
  int getNDims() const { return m_nDim; }

protected: // for testing
private:
  /**Load flat box structure from a nexus file*/
  void loadBoxStructure(::NeXus::File *hFile, bool onlyEventInfo = false);
  /**Load the part of the box structure, responsible for locating events only*/
  /**Save flat box structure into properly open nexus file*/
  void saveBoxStructure(::NeXus::File *hFile);
  //----------------------------------------------------------------------------------------------
  int m_nDim;
  // The name of the file the class will be working with
  std::string m_FileName;
  /// Box type (0=None, 1=MDBox, 2=MDGridBox
  std::vector<int> m_BoxType;
  /// Recursion depth
  std::vector<int> m_Depth;
  /// Start/end indices into the list of events; 2*i -- filePosition, 2*i+1
  /// number of events in the block
  std::vector<uint64_t> m_BoxEventIndex;
  /// Min/Max extents in each dimension
  std::vector<double> m_Extents;
  /// Inverse of the volume of the cell
  std::vector<double> m_InverseVolume;
  /// Box cached signal/error squared
  std::vector<double> m_BoxSignalErrorsquared;
  /// Start/end children IDs
  std::vector<int> m_BoxChildren;
  /// linear vector of boxes;
  std::vector<API::IMDNode *> m_Boxes;
  /// XML representation of the box controller
  std::string m_bcXMLDescr;
  /// name of the event type
  std::string m_eventType;
  /// shared pointer to multiple experiment info stored within the workspace
  boost::shared_ptr<API::MultipleExperimentInfos> m_mEI;

public:
  static ::NeXus::File *createOrOpenMDWSgroup(const std::string &fileName,
                                              int &nDims,
                                              const std::string &WSEventType,
                                              bool readOnly, bool &exist);
  // save each experiment info into its own NeXus group within an existing
  // opened group
  static void saveExperimentInfos(::NeXus::File *const file,
                                  API::IMDEventWorkspace_const_sptr ws);
  // load experiment infos, previously saved through the the saveExperimentInfo
  // function
  static void loadExperimentInfos(
      ::NeXus::File *const file,
      const std::string & filename,
      boost::shared_ptr<API::MultipleExperimentInfos> ei,
      bool lazy = false);

  static void saveAffineTransformMatricies(::NeXus::File *const file,
                                           API::IMDWorkspace_const_sptr ws);
  static void saveAffineTransformMatrix(::NeXus::File *const file,
                                        API::CoordTransform *transform,
                                        std::string entry_name);

  static void saveWSGenericInfo(::NeXus::File *const file,
                                API::IMDWorkspace_const_sptr ws);
};

template <typename T>
void saveMatrix(::NeXus::File *const file, std::string name,
                Kernel::Matrix<T> &m, ::NeXus::NXnumtype type, std::string tag);
}
}
#endif
