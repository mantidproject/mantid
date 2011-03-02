#ifndef MANTID_ALGORITHMS_CALCULATEEFFICIENCY_H_
#define MANTID_ALGORITHMS_CALCULATEEFFICIENCY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid
{
namespace Algorithms
{
/**
 *
    Compute relative detector pixel efficiency from flood data as part of SANS reduction.
    Normalizes pixel counts to the sums up all unmasked pixel counts. If a minimum and/or maximum
    maximum efficiency is provided, the  pixels falling outside the limits will be taken out
    of the normalization and masked.

    For workspaces with more than one TOF bins, the bins are summed up before the calculation
    and the resulting efficiency has a single TOF bin.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace2D to take as input </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the result </LI>
    </UL>

    Optional Properties:
    <UL>

    <LI> MinEfficiency - Minimum efficiency for a pixel to be considered (default: no minimum)</LI>
    <LI> MaxEfficiency - Maximum efficiency for a pixel to be considered (default: no maximum)</LI>
    </UL>

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport CalculateEfficiency : public API::Algorithm
{
public:
  /// Default constructor
  CalculateEfficiency() : API::Algorithm() {};
  /// Destructor
  virtual ~CalculateEfficiency() {};
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "CalculateEfficiency";}
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return (1);}
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "SANS";}

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  // Overridden Algorithm methods
  void init();
  void exec();

  /// Sum all detectors, excluding monitors and masked detectors
  void sumUnmaskedDetectors(API::MatrixWorkspace_sptr rebinnedWS,
      double& sum, double& error, int& nPixels);

  /// Normalize all detectors to get the relative efficiency
  void normalizeDetectors(API::MatrixWorkspace_sptr rebinnedWS,
      API::MatrixWorkspace_sptr outputWS, double sum, double error, int nPixels,
      double min_eff, double max_eff);
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_CALCULATEEFFICIENCY_H_*/
