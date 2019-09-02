// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidKernel/ConfigService.h"

namespace Mantid {
namespace API {
namespace {
/// static logger object
Kernel::Logger g_log("WorkspaceFactory");
} // namespace

using std::size_t;

/// Private constructor for singleton class
WorkspaceFactoryImpl::WorkspaceFactoryImpl()
    : Mantid::Kernel::DynamicFactory<Workspace>() {
  g_log.debug() << "WorkspaceFactory created.\n";
}

/** Create a new instance of the same type of workspace as that given as
 * argument.
 *  If the optional size parameters are given, the workspace will be initialised
 * using
 *  those; otherwise it will be initialised to the same size as the parent.
 *  This method should be used when you want to carry over the Workspace data
 * members
 *  relating to the Instrument, Spectra-Detector Map, Sample & Axes to the new
 * workspace.
 *  If the workspace is the same size as its parent, then the X data, axes and
 * mask list are
 *  copied. If its a different size then they are not.
 *
 *  @deprecated Replaced by functions in MantidDataObjects/WorkspaceCreation.h
 *  @param  parent    A shared pointer to the parent workspace
 *  @param  NVectors  (Optional) The number of vectors/histograms/detectors in
 * the workspace
 *  @param  XLength   (Optional) The number of X data points/bin boundaries in
 * each vector (must all be the same)
 *  @param  YLength   (Optional) The number of data/error points in each vector
 * (must all be the same)
 *  @return A shared pointer to the newly created instance
 *  @throw  std::out_of_range If invalid (0 or less) size arguments are given
 *  @throw  NotFoundException If the class is not registered in the factory
 */
MatrixWorkspace_sptr
WorkspaceFactoryImpl::create(const MatrixWorkspace_const_sptr &parent,
                             size_t NVectors, size_t XLength,
                             size_t YLength) const {
  bool differentSize(true);
  // Use the parent sizes if new ones are not specified
  if (NVectors == size_t(-1))
    NVectors = parent->getNumberHistograms();
  if (XLength == size_t(-1))
    XLength = parent->dataX(0).size();
  if (YLength == size_t(-1)) {
    differentSize = false;
    YLength = parent->blocksize();
  }

  // If the parent is an EventWorkspace, we want it to spawn a Workspace2D (or
  // managed variant) as a child
  std::string id(parent->id());
  if (id == "EventWorkspace")
    id = "Workspace2D";

  // Create an 'empty' workspace of the appropriate type and size
  MatrixWorkspace_sptr ws = create(id, NVectors, XLength, YLength);

  // Copy over certain parent data members
  initializeFromParent(*parent, *ws, differentSize);

  return ws;
}

/** Initialize a workspace from its parent
 * This sets values such as title, instrument, units, sample, spectramap.
 * This does NOT copy any data.
 *
 * @deprecated Replaced by functions in MantidDataObjects/WorkspaceCreation.h
 * @param parent :: the parent workspace
 * @param child :: the child workspace
 * @param differentSize :: A flag to indicate if the two workspace will be
 *different sizes
 */
void WorkspaceFactoryImpl::initializeFromParent(
    const MatrixWorkspace &parent, MatrixWorkspace &child,
    const bool differentSize) const {
  child.setTitle(parent.getTitle());
  child.setComment(parent.getComment());
  child.copyExperimentInfoFrom(&parent);
  child.setYUnit(parent.m_YUnit);
  child.setYUnitLabel(parent.m_YUnitLabel);
  child.setDistribution(parent.isDistribution());

  // Only copy the axes over if new sizes are not given
  if (!differentSize) {
    // Only copy mask map if same size for now. Later will need to check
    // continued validity.
    child.m_masks = parent.m_masks;
  }

  // Same number of histograms = copy over the spectra data
  if (parent.getNumberHistograms() == child.getNumberHistograms()) {
    child.m_isInitialized = false;
    for (size_t i = 0; i < parent.getNumberHistograms(); ++i)
      child.getSpectrum(i).copyInfoFrom(parent.getSpectrum(i));
    child.m_isInitialized = true;
    // We use this variant without ISpectrum update to avoid costly rebuilds
    // triggered by setIndexInfo(). ISpectrum::copyInfoFrom sets invalid flags
    // for spectrum definitions, so it is important to call this *afterwards*,
    // since it clears the flags:
    child.setIndexInfoWithoutISpectrumUpdate(parent.indexInfo());
  }

  // deal with axis
  for (size_t i = 0; i < parent.m_axes.size(); ++i) {
    if (parent.m_axes[i]->isSpectra()) {
      // By default the child already has a spectra axis which
      // does not need to get cloned from the parent.
      continue;
    }
    const bool isBinEdge = dynamic_cast<const BinEdgeAxis *const>(
                               parent.m_axes[i].get()) != nullptr;
    const size_t newAxisLength =
        child.m_axes[i]->length() + (isBinEdge ? 1 : 0);
    const size_t oldAxisLength = parent.m_axes[i]->length();

    // Need to delete the existing axis created in init above

    child.m_axes[i] = nullptr;
    if (newAxisLength == oldAxisLength) {
      // Now set to a copy of the parent workspace's axis
      child.m_axes[i] = std::unique_ptr<Axis>(parent.m_axes[i]->clone(&child));
    } else {
      // Call the 'different length' clone variant
      child.m_axes[i] =
          std::unique_ptr<Axis>(parent.m_axes[i]->clone(newAxisLength, &child));
    }
  }
}

/** Creates a new instance of the class with the given name, and allocates
 * memory for the arrays
 *
 *  @deprecated Replaced by functions in MantidDataObjects/WorkspaceCreation.h
 *  @param  className The name of the class you wish to create
 *  @param  NVectors  The number of vectors/histograms/detectors in the
 * workspace
 *  @param  XLength   The number of X data points/bin boundaries in each vector
 * (must all be the same)
 *  @param  YLength   The number of data/error points in each vector (must all
 * be the same)
 *  @return A shared pointer to the newly created instance
 *  @throw  std::out_of_range If invalid (0 or less) size arguments are given
 *  @throw  NotFoundException If the class is not registered in the factory
 */
MatrixWorkspace_sptr WorkspaceFactoryImpl::create(const std::string &className,
                                                  const size_t &NVectors,
                                                  const size_t &XLength,
                                                  const size_t &YLength) const {
  MatrixWorkspace_sptr ws =
      boost::dynamic_pointer_cast<MatrixWorkspace>(this->create(className));

  if (!ws) {
    g_log.error("Workspace was not created");
    throw std::runtime_error("Workspace was not created");
  }

  ws->initialize(NVectors, XLength, YLength);
  return ws;
}

/// Create a ITableWorkspace
ITableWorkspace_sptr
WorkspaceFactoryImpl::createTable(const std::string &className) const {
  ITableWorkspace_sptr ws;
  try {
    ws = boost::dynamic_pointer_cast<ITableWorkspace>(this->create(className));
    if (!ws) {
      throw std::runtime_error("Class " + className +
                               " cannot be cast to ITableWorkspace");
    }
  } catch (Kernel::Exception::NotFoundError &) {
    throw;
  }
  return ws;
}

/// Create a IPeaksWorkspace
IPeaksWorkspace_sptr
WorkspaceFactoryImpl::createPeaks(const std::string &className) const {
  IPeaksWorkspace_sptr ws;
  try {
    ws = boost::dynamic_pointer_cast<IPeaksWorkspace>(this->create(className));
    if (!ws) {
      throw std::runtime_error("Class " + className +
                               " cannot be cast to IPeaksWorkspace");
    }
  } catch (Kernel::Exception::NotFoundError &) {
    throw;
  }
  return ws;
}

} // namespace API
} // Namespace Mantid
