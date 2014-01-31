#ifndef MANTID_CURVEFITTING_ConvolveWorkspaces_H_
#define MANTID_CURVEFITTING_ConvolveWorkspaces_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IFunctionMW.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidCurveFitting/CubicSpline.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunction1D.h"

namespace Mantid
{
namespace CurveFitting
{
/** Convolution of two workspaces

*/

class DLLExport ConvolveWorkspaces : public API::Algorithm
{
public:
  ConvolveWorkspaces();
  virtual ~ConvolveWorkspaces();
  /// Algorithm's name
  virtual const std::string name() const { return "ConvolveWorkspaces"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Utility\\Workspaces"; }

private:
  virtual void initDocs();
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
  void convolve(MantidVec& xValues, const MantidVec& Y1, const MantidVec& Y2, MantidVec& out)const;
  API::Progress * prog;
};

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_ConvolveWorkspaces_H_*/
