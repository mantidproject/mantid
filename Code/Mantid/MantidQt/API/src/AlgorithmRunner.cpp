#include "MantidQtAPI/AlgorithmRunner.h"

#include <Poco/ActiveResult.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace MantidQt
{
namespace API
{

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  AlgorithmRunner::AlgorithmRunner(QObject * parent) : AbstractAsyncAlgorithmRunner(parent)
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  AlgorithmRunner::~AlgorithmRunner()
  {
  }
  

  //--------------------------------------------------------------------------------------
  /** If an algorithm is already running, cancel it.
   * Does nothing if no algorithm is running. This blocks
   * for up to 1 second to wait for the algorithm to finish cancelling.
   */
  void AlgorithmRunner::cancelRunningAlgorithm()
  {
    AbstractAsyncAlgorithmRunner::cancelRunningAlgorithm();
  }

  //--------------------------------------------------------------------------------------
  /** Begin asynchronous execution of an algorithm and observe its execution
   *
   * @param alg :: algorithm to execute. All properties should have been set properly.
   */
  void AlgorithmRunner::startAlgorithm(Mantid::API::IAlgorithm_sptr alg)
  {
    AbstractAsyncAlgorithmRunner::startAlgorithm(alg);
  }

  /// Get back a pointer to the running algorithm
  Mantid::API::IAlgorithm_sptr AlgorithmRunner::getAlgorithm() const
  {
    return getCurrentAlgorithm();
  }

  //--------------------------------------------------------------------------------------
  /** Observer called when the asynchronous algorithm has completed.
   *
   * Emits a signal for the GUI widget
   *
   * This is called in a separate (non-GUI) thread and so
   * CANNOT directly change the gui.
   *
   * @param pNf :: finished notification object.
   */
  void AlgorithmRunner::handleAlgorithmFinish()
  {
    emit algorithmComplete(false);
  }

  //--------------------------------------------------------------------------------------
  /** Observer called when the async algorithm has progress to report
   *
   * @param pNf :: notification object
   */
  void AlgorithmRunner::handleAlgorithmProgress(double p, const std::string msg)
  {
    emit algorithmProgress(p, msg);
  }

  //--------------------------------------------------------------------------------------
  /** Observer called when the async algorithm has encountered an error.
   * Emits a signal for the GUI widget
   *
   * @param pNf :: notification object
   */
  void AlgorithmRunner::handleAlgorithmError()
  {
    emit algorithmComplete(true);
  }


} // namespace Mantid
} // namespace API
