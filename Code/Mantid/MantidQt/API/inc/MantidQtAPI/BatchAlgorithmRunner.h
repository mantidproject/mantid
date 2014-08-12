#ifndef MANTID_API_BATCHALGORITHMRUNNER_H_
#define MANTID_API_BATCHALGORITHMRUNNER_H_

#include "MantidQtAPI/AbstractAsyncAlgorithmRunner.h"

#include "DllOption.h"
#include "MantidAPI/Algorithm.h"

#include <QObject>

namespace MantidQt
{
namespace API
{
  /**
   * Algorithm runner for execution of a queue of algorithms
    
    @date 2014-08-10

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

  class EXPORT_OPT_MANTIDQT_API BatchAlgorithmRunner : public AbstractAsyncAlgorithmRunner
  {
    Q_OBJECT

  public:
    typedef std::map<std::string, std::string> AlgorithmRuntimeProps;
    typedef std::pair<Mantid::API::IAlgorithm_sptr, AlgorithmRuntimeProps> ConfiguredAlgorithm;

    explicit BatchAlgorithmRunner(QObject * parent = 0);
    virtual ~BatchAlgorithmRunner();
    
    void cancelAll();
    void addAlgorithm(Mantid::API::IAlgorithm_sptr algo, AlgorithmRuntimeProps props = AlgorithmRuntimeProps());

    void startBatch(bool stopOnFailure = true);
    bool isExecuting();

  signals:
    void batchComplete(bool error);
    void batchProgress(double p, const std::string& currentAlg, const std::string& algMsg);

  private:
    void startNextAlgo();

    std::deque<ConfiguredAlgorithm> m_algorithms;
    size_t m_batchSize;

    bool m_stopOnFailure;
    bool m_isExecuting;

    void handleAlgorithmFinish();
    void handleAlgorithmProgress(double p, const std::string msg);
    void handleAlgorithmError();
  };

} // namespace API
} // namespace Mantid

#endif  /* MANTID_API_BATCHALGORITHMRUNNER_H_ */
