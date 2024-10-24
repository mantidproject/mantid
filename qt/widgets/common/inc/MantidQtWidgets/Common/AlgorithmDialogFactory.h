// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//------------------------
// Includes
//------------------------
#include "DllOption.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"
#include <QHash>
#include <QSetIterator>
#include <QStringList>
#include <set>

namespace MantidQt {

namespace API {

//-------------------------------
// MantidQt forward declarations
//-------------------------------
class AlgorithmDialog;
class UserSubWindow;

/**
    The AlgorithmDialogFactory is responsible for creating concrete instances of
    AlgorithmDialog classes. It is implemented as a singleton class.

    @author Martyn Gigg, Tessella plc
    @date 24/02/2009
*/
class EXPORT_OPT_MANTIDQT_COMMON AlgorithmDialogFactoryImpl : public Mantid::Kernel::DynamicFactory<AlgorithmDialog> {

public:
  // Unhide the inherited create method
  using Mantid::Kernel::DynamicFactory<AlgorithmDialog>::createUnwrapped;
  AlgorithmDialogFactoryImpl(const AlgorithmDialogFactoryImpl &) = delete;
  AlgorithmDialogFactoryImpl &operator=(const AlgorithmDialogFactoryImpl &) = delete;

private:
  friend struct Mantid::Kernel::CreateUsingNew<AlgorithmDialogFactoryImpl>;

  /// Private Constructor for singleton class
  AlgorithmDialogFactoryImpl() = default;

  /// Private Destructor
  ~AlgorithmDialogFactoryImpl() override = default;
};

/// The specific instantiation of the templated type
using AlgorithmDialogFactory = Mantid::Kernel::SingletonHolder<AlgorithmDialogFactoryImpl>;
} // namespace API
} // namespace MantidQt

namespace Mantid {
namespace Kernel {
EXTERN_MANTIDQT_COMMON template class EXPORT_OPT_MANTIDQT_COMMON
    Mantid::Kernel::SingletonHolder<MantidQt::API::AlgorithmDialogFactoryImpl>;
} // namespace Kernel
} // namespace Mantid
