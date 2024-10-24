// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/docstring_options.hpp>
#include <boost/python/make_function.hpp>
#include <boost/python/module.hpp>
#include <boost/python/type_id.hpp>

#include "MantidKernel/MantidVersion.h"

using namespace boost::python;
using Mantid::Kernel::MantidVersion;

namespace {

std::string getMajor(const MantidVersion::VersionInfo &self) { return self.major; }

std::string getMinor(const MantidVersion::VersionInfo &self) { return self.minor; }

std::string getPatch(const MantidVersion::VersionInfo &self) { return self.patch; }

std::string getTweak(const MantidVersion::VersionInfo &self) { return self.tweak; }

std::string getStr(const MantidVersion::VersionInfo &self) {
  return self.major + "." + self.minor + "." + self.patch + self.tweak;
}

} // namespace

void export_MantidVersion() {

  class_<MantidVersion::VersionInfo>("VersionInfo")
      .add_property("major", make_function(&getMajor), "The major release version")
      .add_property("minor", make_function(&getMinor), "The minor release version")
      .add_property("patch", make_function(&getPatch), "The patch release version")
      .add_property("tweak", make_function(&getTweak), "The tweak release version")
      .def("__str__", make_function(&getStr), "The version in the standard form: {Major}.{Minor}.{Patch}{Tweak}");

  def("version_str", &Mantid::Kernel::MantidVersion::version,
      "Returns the Mantid version string in the form \"{Major}.{Minor}.{Patch}{Tweak}\"");
  def("version", &Mantid::Kernel::MantidVersion::versionInfo,
      "Returns a data structure containing the major, minor, patch, and tweak parts of the version.");
  def("release_notes_url", &Mantid::Kernel::MantidVersion::releaseNotes,
      "Returns the url to the most applicable release notes");
  def("revision", &Mantid::Kernel::MantidVersion::revision, "Returns the abbreviated SHA-1 of the last commit");
  def("revision_full", &Mantid::Kernel::MantidVersion::revisionFull, "Returns the full SHA-1 of the last commit");
  def("release_date", &Mantid::Kernel::MantidVersion::releaseDate, "Returns the date of the last commit");
  def("doi", &Mantid::Kernel::MantidVersion::doi, "Returns the DOI for this release of Mantid");
  def("paper_citation", &Mantid::Kernel::MantidVersion::paperCitation, "Returns The citation for the Mantid paper");
}
