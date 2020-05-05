// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidIndexing/DllConfig.h"

namespace Mantid {
namespace Indexing {
class IndexInfo;

/** Scattering for IndexInfo, in particular changing its storage mode to
  Parallel::StorageMode::Distributed.

  @author Simon Heybrock
  @date 2017
*/
MANTID_INDEXING_DLL IndexInfo scatter(const IndexInfo &indexInfo);

} // namespace Indexing
} // namespace Mantid
