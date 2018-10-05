// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidPythonInterface/api/PythonAlgorithm/DataProcessorAdapter.h"

//-----------------------------------------------------------------------------
// AlgorithmAdapter definition
//-----------------------------------------------------------------------------
namespace Mantid {
namespace PythonInterface {

/**
 * Construct the "wrapper" and stores the reference to the PyObject
 * @param self A reference to the calling Python object
 */
template <class Base>
DataProcessorAdapter<Base>::DataProcessorAdapter(PyObject *self)
    : AlgorithmAdapter<API::GenericDataProcessorAlgorithm<Base>>(self) {}

template class DataProcessorAdapter<API::Algorithm>;
template class DataProcessorAdapter<API::SerialAlgorithm>;
template class DataProcessorAdapter<API::ParallelAlgorithm>;
template class DataProcessorAdapter<API::DistributedAlgorithm>;
} // namespace PythonInterface
} // namespace Mantid
