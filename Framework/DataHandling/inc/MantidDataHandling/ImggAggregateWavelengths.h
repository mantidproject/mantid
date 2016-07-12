#ifndef MANTID_DATAHANDLING_IMGGAGGREGATEWAVELENGTHS_H_
#define MANTID_DATAHANDLING_IMGGAGGREGATEWAVELENGTHS_H_

#include "MantidDataHandling/DllConfig.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

namespace Poco {
class Path;
}

namespace Mantid {
namespace DataHandling {

/**
  ImggAggregateWavelengths : Aggregates images from multiple energy
  bands

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport ImggAggregateWavelengths final : public API::Algorithm {
public:
  const std::string name() const override final;
  int version() const override final;
  const std::string category() const override final;
  const std::string summary() const override final;

private:
  void init() override final;
  void exec() override final;

  std::map<std::string, std::string> validateInputs() override;

  void aggUniformBands(const std::string &inputPath,
                       const std::string &outputPath, size_t bands);

  void aggIndexBands(const std::string &inputPath,
                     const std::string &outputPath,
                     const std::string &rangesSpec);

  void aggToFBands(const std::string & /*inputPath*/,
                   const std::string & /*outputPath*/,
                   const std::string & /*ranges*/);

  void processDirectory(const Poco::Path &inDir, size_t bands,
                        const std::string &outDir,
                        const std::vector<std::string> &outSubdirs,
                        const std::string &prefix, size_t outImgIndex);

  void processDirectory(const Poco::Path &inDir,
                        const std::vector<std::pair<size_t, size_t>> &ranges,
                        const std::string outDir,
                        const std::vector<std::string> &outSubdirs,
                        const std::string &prefix, size_t outImgIndex);

  std::vector<std::pair<size_t, size_t>>
  rangesFromStringProperty(const std::string &rangesSpec,
                           const std::string &propName);

  std::vector<Poco::Path> findInputSubdirs(const Poco::Path &path);

  std::vector<Poco::Path> findInputImages(const Poco::Path &path);

  std::vector<std::pair<size_t, size_t>>
  splitSizeIntoRanges(size_t availableCount, size_t bands);

  std::vector<std::string> buildOutputSubdirNamesFromUniformBands(
      const std::vector<Poco::Path> &inputSubDirs, size_t bands);

  std::vector<std::string> buildOutputSubdirNamesFromIndexRangesBands(
      const std::vector<std::pair<size_t, size_t>> &outRanges);

  bool isSupportedExtension(const std::string &extShort,
                            const std::string &extLong);

  void aggImage(API::MatrixWorkspace_sptr accum,
                const API::MatrixWorkspace_sptr toAdd);

  void saveAggImage(const API::MatrixWorkspace_sptr accum,
                    const std::string &outDir, const std::string &prefix,
                    size_t outImgIndex);

  API::MatrixWorkspace_sptr loadFITS(const Poco::Path &imgPath,
                                     const std::string &outName);

  void saveFITS(const API::MatrixWorkspace_sptr accum,
                const std::string &filename);

  static const std::string outPrefixProjections;
  static const std::string outPrefixBands;
  static const std::string indexRangesPrefix;
  static const std::string tofRangesPrefix;
  static const std::string outSubdirsPrefixUniformBands;
  static const std::string outSubdirsPrefixIndexBands;
  static const std::string outSubdirsPrefixToFBands;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_IMGGAGGREGATEWAVELENGTHS_H_ */
