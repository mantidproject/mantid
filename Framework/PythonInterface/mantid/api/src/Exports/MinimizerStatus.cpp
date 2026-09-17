// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/IFuncMinimizer.h"

#include <boost/python/args.hpp>
#include <boost/python/class.hpp>

using namespace boost::python;

namespace {
/// A namespace cannot be exported directly, so the canonical status strings are hung off a
/// non-instantiable class that stands in for one on the Python side.
struct MinimizerStatusNamespace {};
} // namespace

void export_MinimizerStatus() {
  auto pythonClass = class_<MinimizerStatusNamespace, boost::noncopyable>(
      "MinimizerStatus",
      "The status strings a function minimizer reports through a fit's OutputStatus, and the test for "
      "whether one of them means the fit converged. Mirrors Mantid::API::MinimizerStatus so Python "
      "callers do not have to hard code the wording.",
      no_init);

  pythonClass.def("isConverged", &Mantid::API::MinimizerStatus::isConverged, (arg("status"), arg("strict") = false),
                  "Whether a fit OutputStatus means the minimizer converged. With strict=True only 'success' "
                  "counts; otherwise the tolerance-limited stopping conditions do too, because a minimizer "
                  "started from an already-optimal set of parameters stops that way routinely.");
  pythonClass.staticmethod("isConverged");

  pythonClass.attr("SUCCESS") = Mantid::API::MinimizerStatus::SUCCESS;
  pythonClass.attr("CHANGES_IN_FUNCTION_TOO_SMALL") = Mantid::API::MinimizerStatus::CHANGES_IN_FUNCTION_TOO_SMALL;
  pythonClass.attr("CHANGES_IN_PARAMETER_TOO_SMALL") = Mantid::API::MinimizerStatus::CHANGES_IN_PARAMETER_TOO_SMALL;
}
