// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <sstream>

#include "MantidAPI/ArchiveSearchFactory.h"
#include "MantidDataHandling/ORNLDataArchive.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/InternetHelper.h"
#include "MantidKernel/Logger.h"

#include <boost/regex.hpp>

using Mantid::Catalog::Exception::CatalogError;
using Mantid::Catalog::ONCat::ONCat;
using Mantid::Catalog::ONCat::ONCat_uptr;
using Mantid::Catalog::ONCat::ONCatEntity;
using Mantid::Catalog::ONCat::QueryParameter;
using Mantid::Catalog::ONCat::QueryParameters;

namespace {

std::string toUpperCase(const std::string &s) {
  std::string result(s);
  std::transform(s.begin(), s.end(), result.begin(), toupper);
  return result;
}

const static boost::regex FILE_REGEX("^(.*?)_(\\d+).*$");
const static std::string NOT_FOUND("");

Mantid::Kernel::Logger g_log("ORNLDataArchive");
} // namespace

namespace Mantid::DataHandling {

DECLARE_ARCHIVESEARCH(ORNLDataArchive, ORNLDataSearch)
DECLARE_ARCHIVESEARCH(ORNLDataArchive, SNSDataSearch)

/**
 * ****************
 * PLEASE READ THIS
 * ****************
 *
 * This archive searcher retrieves SNS / HFIR run locations from ONCat.
 *
 * Something to bear in mind here, however, is that the signature of
 * IArchiveSearch's getArchivePath is quite counter-intuitive, and so probably
 * shouldn't be used as an aid to understanding.  This is because:
 *
 * 1) It claims to deal in "filenames" and "exts", but in reality "basenames"
 *    and "suffixes" would be more accurate terms.  (In general, "[INST]_[RUN]"
 *    is the format of expected "filenames", and "_event.nxs" is one example
 *    of a possible "extension".)  I've just gone ahead and started using the
 *    more accurate terms here.
 *
 * 2) It accepts a *collection* of basenames, but can only ever output a
 *    *single* file path.  An inspection of the surrounding code in FileFinder
 *    will show that this has been done as a workaround to accomodate caseless-
 *    searching of directories on platforms where case make a difference.
 *
 *    (So, when it says "getArchivePath", it really does mean *path* -- if you
 *    want to search for a range of runs then you will either have to extend
 *    the interface, or make multiple calls to the existing one.)
 *
 * 3) The implementation for SNS / ORNL has never (and will never) require all
 *    the basenames passed to it -- it just discards all but the first and then
 *    uses that.
 *
 * 4) In the cases where multiple versions of a raw run file exist in the
 *    archive, we will have only ever ingested *one* of them into ONCat.
 *    (Where, for example, "*_event.nxs" takes precedence over "*_histo.nxs".)
 *    For this reason, a collection of suffixes is not exactly necessary,
 *    either.
 *
 * What we're *actually* doing here then is a best-effort with the information
 * we're given', and returning only the location of the files we know about.
 * We'll parse the run number and the instrument, and then make sure the
 * location ends in one of the expected suffixes.
 *
 * @param basenames :
 *     A set of basenames to check against.  Only the first will be used.
 * @param suffixes : List of extensions to check against.
 * @return The first matching location of an archived raw datafile, else an
 *     empty string.
 */
const API::Result<std::string> ORNLDataArchive::getArchivePath(const std::set<std::string> &basenames,
                                                               const std::vector<std::string> &suffixes) const {
  if (basenames.size() == 0) {
    return API::Result<std::string>(NOT_FOUND, "Not found.");
  }

  // Mimic previous functionality by only using the first basename.
  const auto basename = *basenames.cbegin();

  // Validate and parse the basename.
  boost::smatch result;
  if (!boost::regex_match(basename, result, FILE_REGEX)) {
    g_log.debug() << "Unexpected input passed to getArchivePath():" << std::endl << basename << std::endl;
    return API::Result<std::string>(NOT_FOUND, "Not found.");
    ;
  }

  assert(result.size() == 3);
  const std::string instrument = toUpperCase(result[1]);
  const std::string run = result[2];

  const auto &config = Mantid::Kernel::ConfigService::Instance();
  std::string facility;
  try {
    facility = config.getInstrument(instrument).facility().name();

    if (facility != "HFIR" && facility != "SNS") {
      return API::Result<std::string>(NOT_FOUND, "Not found.");
    }
  } catch (Mantid::Kernel::Exception::NotFoundError &) {
    g_log.debug() << "\"" << instrument << "\" is not an instrument known to Mantid." << std::endl;
    return API::Result<std::string>(NOT_FOUND, "Not found.");
  }

  // Note that we will only be asking for raw files with the given instrument
  // and run number, and *not* filtering by suffix at this point.  (ONCat has
  // a strict definition of what a file "extension" is, and has no way of
  // filtering by, for example, "_event.nxs".)
  const QueryParameters params{{"facility", facility},
                               {"instrument", instrument},
                               {"projection", "location"},
                               {"tags", "type/raw"},
                               {"sort_by", "ingested"},
                               {"sort_direction", "DESCENDING"},
                               {"ranges_q", "indexed.run_number:" + run}};

  // If we've not manually set up an ONCat instance (presumably for testing
  // purposes) then we must instead create one using the settings in the
  // currently-running instance of Mantid, making sure to run it in an
  // "unauthenticated" mode.  If we were to authenticate we'd be able to see
  // more information, but that would require users logging in and publically
  // available information is more than enough for our purposes here, anyway.
  auto defaultOncat = ONCat::fromMantidSettings();
  auto *oncat = m_oncat ? m_oncat.get() : defaultOncat.get();

  const auto datafiles = [&]() {
    try {
      return oncat->list("api", "datafiles", params);
    } catch (CatalogError &ce) {
      g_log.debug() << "Error while calling ONCat:" << std::endl << ce.what() << std::endl;
      return std::vector<ONCatEntity>();
    }
  }();

  if (datafiles.size() == 0) {
    g_log.debug() << "ONCat does not know the location of run \"" << run << "\" for \"" << instrument << "\"."
                  << std::endl;
    return API::Result<std::string>(NOT_FOUND, "Not found.");
  }

  g_log.debug() << "All datafiles returned from ONCat:" << std::endl;
  for (const auto &datafile : datafiles) {
    g_log.debug() << datafile.toString() << std::endl;
  }

  // It's technically possible to have been given multiple locations for a
  // single run, since runs are occasionally written out to the wrong IPTS and
  // therefore need to be "re-translated", leaving us with duplicates in the
  // catalog.  Duplicates require manual intervention to be removed, and so in
  // the meantime, since we have asked for locations to be returned to us in
  // descending order of the time at which they were ingested, we can take the
  // first one and be (quite) sure we end up with the correct run location.
  const auto location = *datafiles.cbegin()->get<std::string>("location");

  // Mimic the previous ICAT-calling functionality by taking "full"
  // suffixes into account.
  for (const auto &suffix : suffixes) {
    const std::string fullSuffix = basename + suffix;
    if (toUpperCase(location).ends_with(toUpperCase(fullSuffix))) {
      return API::Result<std::string>(location);
    }
  }

  if (toUpperCase(location).ends_with(toUpperCase(basename))) {
    return API::Result<std::string>(location);
  }

  return API::Result<std::string>(NOT_FOUND, "Not found.");
}

void ORNLDataArchive::setONCat(ONCat_uptr oncat) { m_oncat = std::move(oncat); }

} // namespace Mantid::DataHandling
