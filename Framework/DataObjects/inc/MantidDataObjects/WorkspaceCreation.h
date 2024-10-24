// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/DllConfig.h"
#include "MantidHistogramData/Histogram.h"

#include <memory>
#include <type_traits>

namespace Mantid {
namespace Indexing {
class IndexInfo;
}
namespace Geometry {
class Instrument;
}
namespace API {
class MatrixWorkspace;
class HistoWorkspace;
} // namespace API
namespace DataObjects {
class EventWorkspace;
class Workspace2D;

/** Factory methods for creating MatrixWorkspaces. A template parameter T
  specifies the type of (or a base type of) the created workspace:
  - The type of the output workspace is identical to T for the variants without
    parent.
  - The type of the output workspace is the dynamic type of the parent if T is a
    base of the parents dynamic type.
  - The type of the output workspace is T if the dynamic type of the parent is a
    base of T.
  - If T is not a base of the parents dynamic type, a conversion is attempted.
    Currently this is the case only for EventWorkspace:
    - If the dynamic type of the parent is EventWorkspace but T is not, either a
      Workspace2D or T is created, whichever is more derived. For example, a
      typical use-case is to drop events and create a Workspace2D from an
      EventWorkspace.  This can be achieved as follows:
      ~~~{.cpp}
      auto ws = create<HistoWorkspace>(parent);
      ~~~
      This is equivalent to the old way of using
      WorkspaceFactory::create(parent). In this case, Workspace2D is more
      derived than HistoWorkspace, so a Workspace2D is created.
    - If the dynamic type of the parent is derived from HistoWorkspace, and
      EventWorkspace can be created from it.

  Other arguments can include:
  - The instrument.
  - The desired number of spectra (NumSpectra) to be created in the output
    workspace.
  - A reference to an IndexInfo object, defining the number of spectra and
    spectrum number and detector IDs.
  - A reference to a Histogram object (or alternatively BinEdges or Points),
    defining the size of the histograms as well as whether the workspace stores
    point data or histogram data. This is a replacement for an independent
    specification of the X and Y lengths. This is also used to initialize the
    workspace with X data and (optionally) Y and E data.

  Available variants are:

  ~~~{.cpp}
  create<T>(NumSpectra, Histogram)
  create<T>(IndexInfo,  Histogram)
  create<T>(Instrument, NumSpectra, Histogram)
  create<T>(Instrument, IndexInfo,  Histogram)
  create<T>(ParentWS)
  create<T>(ParentWS, Histogram)
  create<T>(ParentWS, NumSpectra, Histogram)
  create<T>(ParentWS, IndexInfo, Histogram)
  ~~~

  - If neither NumSpectra nor IndexInfo is given, or if the new size is
    identical to the size of the parent, the created workspace has the same
    number of spectra as the parent workspace and spectrum number as well as
    detector ID information is copied from the parent.
  - If only ParentWS is given, the created workspace has X identical to the
    parent workspace and Y and E are initialized to 0.
  - If a Histogram with 'NULL' Y and E is given, Y and E are initialized to 0.

  In all cases a (smart) pointer to T is returned.

  @author Simon Heybrock
  @date 2016
*/

namespace detail {
MANTID_DATAOBJECTS_DLL HistogramData::Histogram stripData(HistogramData::Histogram histogram);

template <class T> std::unique_ptr<T> createHelper() { return std::make_unique<T>(); }

template <class T> std::unique_ptr<T> createConcreteHelper() { return std::make_unique<T>(); }

template <> MANTID_DATAOBJECTS_DLL std::unique_ptr<API::HistoWorkspace> createHelper();

// Dummy specialization, should never be called, must exist for compilation.
template <> MANTID_DATAOBJECTS_DLL std::unique_ptr<EventWorkspace> createHelper();
// Dummy specialization, should never be called, must exist for compilation.
template <> MANTID_DATAOBJECTS_DLL std::unique_ptr<API::MatrixWorkspace> createHelper();

// Dummy specialization, should never be called, must exist for compilation.
template <> MANTID_DATAOBJECTS_DLL std::unique_ptr<API::MatrixWorkspace> createConcreteHelper();
// Dummy specialization, should never be called, must exist for compilation.
template <> MANTID_DATAOBJECTS_DLL std::unique_ptr<API::HistoWorkspace> createConcreteHelper();

template <class HistArg> void fixDistributionFlag(API::MatrixWorkspace &, const HistArg &) {}

template <>
MANTID_DATAOBJECTS_DLL void fixDistributionFlag(API::MatrixWorkspace &workspace,
                                                const HistogramData::Histogram &histArg);

template <class T> struct IsIndexInfo {
  using type = std::false_type;
};
template <> struct IsIndexInfo<Indexing::IndexInfo> {
  using type = std::true_type;
};
template <class UseIndexInfo>
void initializeFromParent(const API::MatrixWorkspace &parent, API::MatrixWorkspace &workspace);
} // namespace detail

/** This is the create() method that all the other create() methods call.
 *  And it is also called directly.
 */
template <class T, class P, class IndexArg, class HistArg,
          class = typename std::enable_if<std::is_base_of<API::MatrixWorkspace, P>::value>::type>
std::unique_ptr<T> create(const P &parent, const IndexArg &indexArg, const HistArg &histArg) {
  // Figure out (dynamic) target type:
  // - Type is same as parent if T is base of parent
  // - If T is not base of parent, conversion may occur. Currently only
  //   supported for EventWorkspace
  std::unique_ptr<T> ws;
  if (std::is_base_of<API::HistoWorkspace, T>::value && parent.id() == "EventWorkspace") {
    // Drop events, create Workspace2D or T whichever is more derived.
    ws = detail::createHelper<T>();
  } else {
    try {
      // If parent is more derived than T: create type(parent)
      ws = dynamic_cast<const T &>(parent).cloneEmpty();
    } catch (std::bad_cast &) {
      // If T is more derived than parent: create T
      ws = detail::createConcreteHelper<T>();
    }
  }

  // The instrument is also copied by initializeFromParent, but if indexArg is
  // IndexInfo and contains non-empty spectrum definitions the initialize call
  // will fail due to invalid indices in the spectrum definitions. Therefore, we
  // copy the instrument first. This should be cleaned up once we figure out the
  // future of WorkspaceFactory.
  ws->setInstrument(parent.getInstrument());
  ws->initialize(indexArg, HistogramData::Histogram(histArg));
  detail::initializeFromParent<typename detail::IsIndexInfo<IndexArg>::type>(parent, *ws);
  // initializeFromParent sets the distribution flag to the same value as
  // parent. In case histArg is an actual Histogram that is not the correct
  // behavior so we have to set it back to the value given by histArg.
  detail::fixDistributionFlag(*ws, histArg);
  return ws;
}

template <class T, class IndexArg, class HistArg,
          typename std::enable_if<!std::is_base_of<API::MatrixWorkspace, IndexArg>::value>::type * = nullptr>
std::unique_ptr<T> create(const IndexArg &indexArg, const HistArg &histArg) {
  auto ws = std::make_unique<T>();
  ws->initialize(indexArg, HistogramData::Histogram(histArg));
  return ws;
}

template <class T, class IndexArg, class HistArg,
          typename std::enable_if<!std::is_base_of<API::MatrixWorkspace, IndexArg>::value>::type * = nullptr>
std::unique_ptr<T> create(const std::shared_ptr<const Geometry::Instrument> &instrument, const IndexArg &indexArg,
                          const HistArg &histArg) {
  auto ws = std::make_unique<T>();
  ws->setInstrument(std::move(instrument));
  ws->initialize(indexArg, HistogramData::Histogram(histArg));
  return ws;
}

template <class T, class P, typename std::enable_if<std::is_base_of<API::MatrixWorkspace, P>::value>::type * = nullptr>
std::unique_ptr<T> createRagged(const P &parent) {
  const auto numHistograms = parent.getNumberHistograms();

  // make a temporary histogram that will be used for initialization. Can't be 0 size so resize.
  auto histArg = HistogramData::Histogram(parent.histogram(0).xMode(), parent.histogram(0).yMode());
  histArg.resize(1);
  auto ws = create<T>(parent, numHistograms, histArg);
  for (size_t i = 0; i < numHistograms; ++i) {
    ws->resizeHistogram(i, parent.histogramSize(i));
    ws->setSharedX(i, parent.sharedX(i));
  }
  return ws;
}

template <class T, class P, typename std::enable_if<std::is_base_of<API::MatrixWorkspace, P>::value>::type * = nullptr>
std::unique_ptr<T> create(const P &parent) {
  if (parent.isRaggedWorkspace())
    return createRagged<T>(parent);

  const auto numHistograms = parent.getNumberHistograms();
  auto ws = create<T>(parent, numHistograms, detail::stripData(parent.histogram(0)));
  for (size_t i = 0; i < numHistograms; ++i) {
    ws->setSharedX(i, parent.sharedX(i));
  }
  return ws;
}

// Templating with HistArg clashes with the IndexArg template above. Could be
// fixed with many enable_if cases, but for now we simply provide 3 variants
// (Histogram, BinEdges, Points) by hand.
template <class T, class P, typename std::enable_if<std::is_base_of<API::MatrixWorkspace, P>::value>::type * = nullptr>
std::unique_ptr<T> create(const P &parent, const HistogramData::Histogram &histogram) {
  return create<T>(parent, parent.getNumberHistograms(), histogram);
}

template <class T, class P, typename std::enable_if<std::is_base_of<API::MatrixWorkspace, P>::value>::type * = nullptr>
std::unique_ptr<T> create(const P &parent, const HistogramData::BinEdges &binEdges) {
  return create<T>(parent, parent.getNumberHistograms(), binEdges);
}

template <class T, class P, typename std::enable_if<std::is_base_of<API::MatrixWorkspace, P>::value>::type * = nullptr>
std::unique_ptr<T> create(const P &parent, const HistogramData::Points &points) {
  return create<T>(parent, parent.getNumberHistograms(), points);
}

} // namespace DataObjects
} // namespace Mantid
