#include "MantidIndexing/Group.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidTypes/SpectrumDefinition.h"

namespace Mantid {
namespace Indexing {

/** Return IndexInfo with grouped spectra as specified in the arguments.
*
* @param source IndexInfo to use as starting point for grouping.
* @param specNums Vector of spectrum numbers to use for the ouput IndexInfo.
* @param grouping Vector for specifying the grouping. The i-th entry in this
* vector describes the group for the i-th entry in 'specNums'. Each entry is a
* vector of indices of spectra in 'source' that are to be grouped.
*/
IndexInfo group(const IndexInfo &source, std::vector<SpectrumNumber> &&specNums,
                const std::vector<std::vector<size_t>> &grouping) {
  if (specNums.size() != grouping.size())
    throw std::runtime_error("Indexing::group: Size mismatch between spectrum "
                             "number and grouping vectors");
  std::vector<SpectrumDefinition> specDefs;
  const auto &sourceDefs = source.spectrumDefinitions();
  for (const auto &group : grouping) {
    specDefs.emplace_back(SpectrumDefinition{});
    for (const auto &i : group) {
      auto &newSpecDef = specDefs.back();
      for (const auto &specDef : (*sourceDefs)[i]) {
        newSpecDef.add(specDef.first, specDef.second);
      }
    }
  }
  IndexInfo result(std::move(specNums));
  result.setSpectrumDefinitions(std::move(specDefs));
  return result;
}

} // namespace Indexing
} // namespace Mantid
