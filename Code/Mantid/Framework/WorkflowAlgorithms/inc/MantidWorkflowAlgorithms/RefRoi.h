#ifndef MANTID_ALGORITHMS_RefRoi_H_
#define MANTID_ALGORITHMS_RefRoi_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace WorkflowAlgorithms
{
/**

    Workflow algorithm for reflectometry to sum up a region of interest on a 2D detector.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

class DLLExport RefRoi : public API::Algorithm
{
public:
  /// (Empty) Constructor
  RefRoi() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~RefRoi() {}
  /// Algorithm's name
  virtual const std::string name() const { return "RefRoi"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Workflow\\Reflectometry"; }

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  /// Initialisation code
  void init();
  /// Execution code
  void exec();

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

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_RefRoi_H_*/
