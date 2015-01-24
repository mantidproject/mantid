#ifndef MANTID_ALGORITHMS_CONVERTSPECTRUMAXIS_H_
#define MANTID_ALGORITHMS_CONVERTSPECTRUMAXIS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/IDetector.h"

namespace Mantid {
namespace Algorithms {
/** Converts the representation of the vertical axis (the one up the side of
    a matrix in MantidPlot) of a Workspace2D from its default of holding the
    spectrum number to the target unit given - theta, elastic Q and elastic Q^2.

    The spectra will be reordered by increasing theta and duplicates will not be
   aggregated.

    Required Properties:
    <UL>
    <LI> InputWorkspace  - The name of the input workspace. </LI>
    <LI> OutputWorkspace - The name of the output workspace. </LI>
    <LI> Target          - The unit to which the spectrum axis should be
   converted. </LI>
    </UL>

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport ConvertSpectrumAxis2 : public API::Algorithm {
public:
  /// (Empty) Constructor
  ConvertSpectrumAxis2() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~ConvertSpectrumAxis2() {}
  /// Algorithm's name
  virtual const std::string name() const { return "ConvertSpectrumAxis"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Converts the axis of a Workspace2D which normally holds spectrum "
           "numbers to one of Q, Q^2 or theta.  'Note': After running this "
           "algorithm, some features will be unavailable on the workspace as "
           "it will have lost all connection to the instrument. This includes "
           "things like the 3D Instrument Display.";
  }

  /// Algorithm's version
  virtual int version() const { return (2); }
  /// Algorithm's category for identification
  virtual const std::string category() const {
    return "Transforms\\Units;Transforms\\Axes";
  }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
  /// Converting to theta.
  void createThetaMap(const std::string &target);
  /// Converting to Q and QSquared
  void createElasticQMap(const std::string &target);
  /// Creates an output workspace.
  API::MatrixWorkspace_sptr createOutputWorkspace(const std::string &target);

  // Stores the input workspace.
  API::MatrixWorkspace_const_sptr m_inputWS;
  // Map to which the conversion to the unit is stored.
  std::multimap<double, size_t> m_indexMap;
  // Stores the number of bins.
  size_t m_nBins;
  // Stores the number of x bins.
  size_t m_nxBins;
  // Stores the number of histograms.
  size_t m_nHist;

  /// Getting Efixed
  double getEfixed(Geometry::IDetector_const_sptr detector,
                   API::MatrixWorkspace_const_sptr inputWS, int emode) const;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_CONVERTSPECTRUMAXIS_H_*/
