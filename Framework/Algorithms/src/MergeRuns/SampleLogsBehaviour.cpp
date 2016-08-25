#include "MantidAlgorithms/MergeRuns/SampleLogsBehaviour.h"
#include "MantidGeometry/Instrument.h"

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;
// using namespace Geometry;

namespace {
std::string generateDifferenceMessage(std::string item, std::string wsName,
                                      std::string wsValue,
                                      std::string firstValue) {
  std::stringstream stringstream;
  stringstream << "Item \"" << item
               << "\" has different values in files! Found: " << wsValue
               << " in file " << wsName
               << " but value in first file value was: " << firstValue << "."
               << std::endl;
  return stringstream.str();
}
}

SampleLogsBehaviour::SampleLogsBehaviour(Logger &g_log) : m_logger(g_log) {}

void SampleLogsBehaviour::createSampleLogsMaps(const MatrixWorkspace_sptr &ws) {
  getSampleList(average, "sample_logs_average", ws);
  getSampleList(min, "sample_logs_min", ws);
  getSampleList(max, "sample_logs_max", ws);
  getSampleList(sum, "sample_logs_sum", ws);
  getSampleList(list, "sample_logs_list", ws);
  getSampleList(warn, "sample_logs_warn", ws, "sample_logs_warn_delta");
  getSampleList(fail, "sample_logs_fail", ws, "sample_logs_warn_delta");
}

void SampleLogsBehaviour::getSampleList(const MergeLogType &sampleLogBehaviour,
                                        const std::string &parameterName,
                                        const MatrixWorkspace_sptr &ws,
                                        const std::string sampleLogDeltas) {
  std::string params =
      ws->getInstrument()->getParameterAsString(parameterName, false);
  std::string paramsDelta =
      ws->getInstrument()->getParameterAsString(sampleLogDeltas, false);
  StringTokenizer tokenizer(params, ",", StringTokenizer::TOK_TRIM |
                                             StringTokenizer::TOK_IGNORE_EMPTY);
  StringTokenizer tokenizerDelta(params, ",",
                                 StringTokenizer::TOK_TRIM |
                                     StringTokenizer::TOK_IGNORE_EMPTY);

  size_t numberNames = tokenizer.count();
  size_t numberTolerances = tokenizer.count();

  if (numberNames == numberTolerances) {
    // pass
  } else if (numberTolerances == 0) {
    // pass
  } else if (numberTolerances == 1) {
    // pass
  } else {
    throw std::invalid_argument("Invalid length of tolerances, found " +
                                std::to_string(numberTolerances) +
                                " tolerance values but " +
                                std::to_string(numberNames) + " names.");
  }

  for (auto item : tokenizer.asVector()) {
    if (m_logMap.count(item) != 0) {
      throw std::invalid_argument(
          "Error when making list of merge items, sample log \"" + item +
          "\" defined more than once!");
    }

    Property *prop;
    try {
      prop = ws->getLog(item);
    } catch (std::invalid_argument e) {
      m_logger.warning() << "Could not merge sample log \"" << item
                         << "\", does not exist in workspace!" << std::endl;
      continue;
    }
    if (sampleLogBehaviour == list) {
      ws->mutableRun().addProperty(item + "_list", prop->value());
      prop = ws->getLog(item + "_list");
    }
    m_logMap[item] = std::make_pair(prop, sampleLogBehaviour);
  }
}

void SampleLogsBehaviour::calculateUpdatedSampleLogs(
    const MatrixWorkspace_sptr &ws, const MatrixWorkspace_sptr &outWS,
    const int numberOfWSsAdded) {
  for (auto item : m_logMap) {
    Property *wsProperty = ws->getLog(item.first);

    double wsNumber = 0;
    double outWSNumber = 0;

    try {
      wsNumber = ws->getLogAsSingleValue(item.first);
      outWSNumber = outWS->getLogAsSingleValue(item.first);
    } catch (std::invalid_argument) {
      if (item.second.second == average || item.second.second == min ||
          item.second.second == max || item.second.second == sum) {
        throw std::invalid_argument(
            item.first + " could not be converted to a numeric type");
      }
    }

    switch (item.second.second) {
    case average: {
      double value = (wsNumber + outWSNumber) *
                     ((numberOfWSsAdded) / (double)(numberOfWSsAdded + 1));
      item.second.first->setValue(std::to_string(value));
      break;
    }
    case min:
      item.second.first->setValue(
          std::to_string(std::min(wsNumber, outWSNumber)));
      break;
    case max:
      item.second.first->setValue(
          std::to_string(std::max(wsNumber, outWSNumber)));
      break;
    case sum:
      item.second.first->setValue(std::to_string(wsNumber + outWSNumber));
      break;
    case list:
      item.second.first->setValue(item.second.first->value() + ", " +
                                  wsProperty->value());
      outWS->getLog(item.first + "_list")
          ->setValueFromProperty(*item.second.first);
      break;
    case warn:
      if (item.second.first->value().compare(wsProperty->value()) != 0) {
        m_logger.warning() << generateDifferenceMessage(
            item.first, ws->name(), wsProperty->value(),
            item.second.first->value());
      }
      break;
    case fail:
      if (item.second.first->value().compare(wsProperty->value()) != 0) {
        throw std::invalid_argument(generateDifferenceMessage(
            item.first, ws->name(), wsProperty->value(),
            item.second.first->value()));
      }
      break;
    }
  }
}

void SampleLogsBehaviour::setUpdatedSampleLogs(const MatrixWorkspace_sptr &ws) {
  for (auto item : m_logMap) {
    Property *outWSProperty = ws->getLog(item.first)->clone();
    ws->mutableRun().addProperty(outWSProperty, true);
  }
}

void SampleLogsBehaviour::resetSampleLogs(const MatrixWorkspace_sptr &ws) {
  for (auto item : m_logMap) {
    Property *wsProperty = ws->getLog(item.first);
    item.second.first->setValue(wsProperty->value());
  }
}

} // namespace Algorithms
} // namespace Mantid
