#ifndef MANTID_MDALGORITHMS_MDWS_TRANSFORMATION_H
#define MANTID_MDALGORITHMS_MDWS_TRANSFORMATION_H

#include "MantidMDAlgorithms/MDTransfAxisNames.h"
#include "MantidMDAlgorithms//MDWSDescription.h"

namespace Mantid {
namespace MDAlgorithms {
/***  The class responsible for building Momentums Transformation Matrix for
   CnvrtToMD algorithm
   *  from the input parameters of the algorithm and parameters, retrieved from
   input and
   *  (if availible) output MD workspace
   *
   *  The parameters are mainly related to MDTransfQ3D though are partially
   applicable to MDTransfModQ (scaling)
   *  They are fully igonred for MDTransfNoQ which copies its data to
   MDworkspace and completely ignores
   *  the transformation matrix, defined by this class

   @date 2012-03-20

   Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
namespace CnvrtToMD {
/// enum descrines availble momentum scalings, interpreted by this class
enum CoordScaling {
  NoScaling,   //< momentums in A^-1
  SingleScale, //< momentuns divided by  2*Pi/Lattice -- equivalend to d-spacing
  // in some sence
  OrthogonalHKLScale, //< each momentum component divided by appropriate lattice
  // parameter; equivalent to hkl for reclenear lattice
  HKLScale, //< non-orthogonal system for non-reclenear lattice
  NCoordScalings
};
/// enum describes availible target coordinate systems for Q3D mode
enum TargetFrame {
  LabFrame, //< * '''Q (lab frame)''': this calculates the momentum transfer
            //(ki-kf) for each event is calculated in the experimental lab
  // frame.
  SampleFrame, //< * '''Q (sample frame)''': the goniometer rotation of the
  // sample is taken out, to give Q in the frame of the sample. See
  //[[SetGoniometer]] to specify the goniometer used in the
  // experiment.
  HKLFrame, //<* '''HKL''': uses the UB matrix (see [[SetUB]],
            //[[FindUBUsingFFT]] and others) to calculate the HKL Miller indices
  // of each event.
  AutoSelect, //<*  This tries to select one of above by analyzing the
  // goniometer and UB matrix parameters on the workspace and tries
  // to establish what coordinate system is actually defined/needed.
  NTargetFrames
};
}

class DLLExport MDWSTransform {
public:
  MDWSTransform();

  /** helper function which verifies if projection vectors are specified and if
   * their values are correct when present.
      * sets default values u and v to [1,0,0] and [0,1,0] if not present or any
   * error. */
  void setUVvectors(const std::vector<double> &ut,
                    const std::vector<double> &vt,
                    const std::vector<double> &wt);

  std::vector<double>
  getTransfMatrix(MDAlgorithms::MDWSDescription &TargWSDescription,
                  const std::string &FrameRequested,
                  const std::string &QScaleRequested) const;

  /// construct meaningful dimension names for Q3D case and different
  /// transformation types defined by the class
  void setQ3DDimensionsNames(MDAlgorithms::MDWSDescription &TargWSDescription,
                             CnvrtToMD::TargetFrame FrameID,
                             CnvrtToMD::CoordScaling scaling) const;
  /// construct meaningful dimension names for ModQ case and different
  /// transformation types defined by the class;
  void setModQDimensionsNames(MDAlgorithms::MDWSDescription &TargWSDescription,
                              const std::string &QScaleRequested) const;
  /// return the list of possible scalings for momentums
  std::vector<std::string> getQScalings() const { return m_QScalingID; }
  CnvrtToMD::CoordScaling getQScaling(const std::string &ScID) const;
  std::string getQScaling(const CnvrtToMD::CoordScaling ScaleID) const;
  /// returns the list of possible target frames to convert to
  std::vector<std::string> getTargetFrames() const { return m_TargFramesID; }
  /// converts the target frame string representation into the frame ID
  CnvrtToMD::TargetFrame getTargetFrame(const std::string &FrameID) const;
  std::string getTargetFrame(const CnvrtToMD::TargetFrame FrameID) const;

private:
  bool m_isUVdefault;
  /** vectors, which describe the projection plain the target ws is based on
   * (notional or cryst cartezian coordinate system). The transformation matrix
   * below
    * should bring the momentums from lab coordinate system into orthogonal,
   * related to u,v vectors, coordinate system */
  mutable Kernel::V3D m_UProj, m_VProj, m_WProj;

  /// string representation of QScaling ID, which would be known to user
  std::vector<std::string> m_QScalingID;
  /// string representation of Target frames, which would be exposed to user;
  std::vector<std::string> m_TargFramesID;
  bool v3DIsDefault(const std::vector<double> &vect,
                    const std::string &message) const;

protected: // for testing
  /// function generates "Kind of" W transformation matrix for different
  /// Q-conversion modes;
  Kernel::DblMatrix buildQTrahsf(MDAlgorithms::MDWSDescription &TargWSDescription,
                                 CnvrtToMD::CoordScaling scaling,
                                 bool UnitUB = false) const;
  /// build orthogonal coordinate around two input vecotors u and v expressed in
  /// rlu;
  // std::vector<Kernel::V3D> buildOrtho3D(const Kernel::DblMatrix &BM,const
  // Kernel::V3D &u, const Kernel::V3D &v)const;

  std::vector<double>
  getTransfMatrix(MDAlgorithms::MDWSDescription &TargWSDescription,
                  CnvrtToMD::TargetFrame FrameID,
                  CnvrtToMD::CoordScaling &scaling) const;

  CnvrtToMD::TargetFrame
  findTargetFrame(MDAlgorithms::MDWSDescription &TargWSDescription) const;
  // helper function which verifies, if the input information availble on the
  // workspace consistent with the frame requiested
  void checkTargetFrame(const MDAlgorithms::MDWSDescription &TargWSDescription,
                        const CnvrtToMD::TargetFrame CoordFrameID) const;
};
}
}

#endif
