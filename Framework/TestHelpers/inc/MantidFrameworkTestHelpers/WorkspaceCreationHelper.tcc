// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/*********************************************************************************
 *  PLEASE READ THIS!!!!!!!
 *
 *  This header MAY NOT be included in any test from a package below DataObjects
 *    (e.g. Kernel, Geometry, API).
 *  Conversely, this file (and its cpp) MAY NOT be modified to use anything from
 *a
 *  package higher than DataObjects (e.g. any algorithm), even if via the
 *factory.
 *********************************************************************************/

#pragma once

#include <functional>
#include <tuple>

namespace WorkspaceCreationHelper {

namespace impl {

using Mantid::HistogramData::Histogram;
using Histogram_sptr = std::shared_ptr<Histogram>;
// A generalized function type returning a histogram.
template <typename... Args> using HistogramFunc = std::function<Histogram_sptr(Args...)>;

template <typename... Args, std::size_t... I>
Histogram_sptr call_function_impl(HistogramFunc<Args...> f, std::tuple<Args...> args, std::index_sequence<I...>) {
  return f(std::get<I>(args)...);
}

/*
 * Unpack a std::tuple and present it as an argument pack to a function.
 */
template <typename... Args> Histogram_sptr call_function(HistogramFunc<Args...> f, std::tuple<Args...> args) {
  return call_function_impl(f, args, std::make_index_sequence<sizeof...(Args)>());
}
} // namespace impl

/**
 * Creates a 2D workspace from a function object, and a list of args instantiations.
 * The number of spectra in the resulting workspace corresponds to the number of entries in the provided list.
 * @param spectrumFunc :: A function to use to calculate the histogram of each spectrum
 * @param argss :: a list of argument instantiations to the function,
 *    each element of the list provides the required arguments to produce a single spectrum
 */
template <typename... Args>
Mantid::DataObjects::Workspace2D_sptr
create2DWorkspaceFromFunctionAndArgsList(impl::HistogramFunc<Args...> spectrumFunc,
                                         std::initializer_list<std::tuple<Args...>> argss) {
  auto ws = Mantid::API::createWorkspace<Mantid::DataObjects::Workspace2D>(
      argss.size(), *impl::call_function(spectrumFunc, *argss.begin()));

  // spectrum(0) has already been initialized
  auto itArgs = argss.begin();
  ++itArgs;
  for (size_t n = 1; itArgs != argss.end(); ++itArgs, ++n) {
    auto args = *itArgs;
    auto test = impl::call_function(spectrumFunc, args);
    ws->getSpectrum(n).setHistogram(*impl::call_function(spectrumFunc, args));
  }

  return ws;
}

template <typename... Args>
Mantid::DataObjects::Workspace2D_sptr
create2DWorkspaceFromFunctionAndArgsList_(impl::HistogramFunc<Args...> spectrumFunc,
                                          const std::vector<std::tuple<Args...>> &argss) {
  auto ws = Mantid::API::createWorkspace<Mantid::DataObjects::Workspace2D>(
      argss.size(), *impl::call_function(spectrumFunc, argss.front()));

  // spectrum(0) has already been initialized
  auto itArgs = argss.cbegin();
  ++itArgs;
  for (size_t n = 1; itArgs != argss.cend(); ++itArgs, ++n) {
    auto args = *itArgs;
    auto test = impl::call_function(spectrumFunc, args);
    ws->getSpectrum(n).setHistogram(*impl::call_function(spectrumFunc, args));
  }

  return ws;
}

} // namespace WorkspaceCreationHelper
