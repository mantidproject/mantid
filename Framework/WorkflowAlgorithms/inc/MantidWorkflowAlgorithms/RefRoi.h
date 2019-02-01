// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_RefRoi_H_
#define MANTID_ALGORITHMS_RefRoi_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace WorkflowAlgorithms {
/**

    Workflow algorithm for reflectometry to sum up a region of interest on a 2D
   detector.
*/

class DLLExport RefRoi : public API::Algorithm {
public:
  /// Constructor
  RefRoi();
  /// Algorithm's name
  const std::string name() const override { return "RefRoi"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Workflow algorithm for reflectometry to sum up a region of "
           "interest on a 2D detector.";
  }
  /// Algorithm's version
  int version() const override { return (1); }

  /// Algorithm's category for identification
  const std::string category() const override {
    return "Workflow\\Reflectometry";
  }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  void extractReflectivity();
  void reverse(API::MatrixWorkspace_sptr WS);
  void extract2D();

  int m_xMin;
  int m_xMax;
  int m_yMin;
  int m_yMax;
  int m_nXPixel;
  int m_nYPixel;
};

} // namespace WorkflowAlgorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_RefRoi_H_*/
