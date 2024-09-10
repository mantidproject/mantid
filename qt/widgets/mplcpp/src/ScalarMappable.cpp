// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/MplCpp/ScalarMappable.h"
#include "MantidQtWidgets/MplCpp/Colormap.h"

#include "MantidPythonInterface/core/CallMethod.h"
#include "MantidPythonInterface/core/Converters/VectorToNDArray.h"
#include "MantidPythonInterface/core/Converters/WrapWithNDArray.h"
#include "MantidPythonInterface/core/NDArray.h"

using Mantid::PythonInterface::callMethodNoCheck;
using Mantid::PythonInterface::GlobalInterpreterLock;
using Mantid::PythonInterface::NDArray;
using Mantid::PythonInterface::Converters::VectorToNDArray;
using Mantid::PythonInterface::Converters::WrapReadOnly;
using namespace MantidQt::Widgets::Common;

namespace MantidQt::Widgets::MplCpp {

namespace {

Python::Object createScalarMappable(const NormalizeBase &norm, const Colormap &cmap) {
  GlobalInterpreterLock lock;
  return cmModule().attr("ScalarMappable")(norm.pyobj(), cmap.pyobj());
}
} // namespace

/**
 * @brief Construct a ScalarMappable instance with the given normalization
 * type and colormap
 * @param norm Instance use to define the mapping from data to [0,1]
 * @param cmap A Colormap defining the RGBA values to use for drawing
 */
ScalarMappable::ScalarMappable(const NormalizeBase &norm, const Colormap &cmap)
    : Python::InstanceHolder(createScalarMappable(norm, cmap), "to_rgba") {
  // The internal array needs to be set to some iterable but apparently
  // it is not used:
  // https://stackoverflow.com/questions/28801803/matplotlib-scalarmappable-why-need-to-set-array-if-norm-set
  GlobalInterpreterLock lock;
  pyobj().attr("set_array")(Python::NewRef(Py_BuildValue("()")));
}

/**
 * @brief Construct a ScalarMappable instance with the given normalization
 * type and colormap
 * @param norm Instance use to define the mapping from data to [0,1]
 * @param cmap A string name for a Colormap
 */
ScalarMappable::ScalarMappable(const NormalizeBase &norm, const QString &cmap) : ScalarMappable(norm, getCMap(cmap)) {}

/// @return A reference to the colormap instance
Colormap ScalarMappable::cmap() const {
  GlobalInterpreterLock lock;
  return Colormap(pyobj().attr("cmap"));
}

/**
 * Reset the underlying colormap
 * @param cmap An instance of a Colormap
 */
void ScalarMappable::setCmap(const Colormap &cmap) {
  callMethodNoCheck<void, Python::Object>(pyobj(), "set_cmap", cmap.pyobj());
}

/**
 * Reset the underlying colormap
 * @param cmap The name of a colormap
 */
void ScalarMappable::setCmap(const QString &cmap) {
  callMethodNoCheck<void, const char *>(pyobj(), "set_cmap", cmap.toLatin1().constData());
}

/**
 * @brief Reset the normalization instance
 * @param norm A normalization type
 */
void ScalarMappable::setNorm(const NormalizeBase &norm) {
  callMethodNoCheck<void, Python::Object>(pyobj(), "set_norm", norm.pyobj());
}

/**
 * Reset the mappable limits
 * @param vmin An optional new minmum value
 * @param vmax An optional new maximum value
 */
void ScalarMappable::setClim(std::optional<double> vmin, std::optional<double> vmax) {
  GlobalInterpreterLock lock;
  Python::Object none;
  auto setClimAttr = pyobj().attr("set_clim");
  if (vmin.has_value() && vmax.has_value()) {
    setClimAttr(vmin.value(), vmax.value());
  } else if (vmin.has_value()) {
    setClimAttr(vmin.value(), none);
  } else if (vmax.has_value()) {
    setClimAttr(none, vmax.value());
  }
}

/**
 * @brief Convert a data value to an RGBA value
 * @param x The data value within the
 * @param alpha The alpha value (default = 1)
 * @return A QRgb value corresponding to this data point
 */
QRgb ScalarMappable::toRGBA(double x, double alpha) const {
  GlobalInterpreterLock lock;
  return toRGBA(std::vector<double>(1, x), alpha)[0];
}

/**
 * @brief Convert an array of data values to a set of RGBA values
 * @param x The data array of values
 * @param alpha The alpha value (default = 1)
 * @return An array of QRgb values corresponding to the data points
 */
std::vector<QRgb> ScalarMappable::toRGBA(const std::vector<double> &x, double alpha) const {
  std::vector<QRgb> rgbaVector(x.size());
  GlobalInterpreterLock lock;
  auto ndarrayView = Python::NewRef(VectorToNDArray<double, WrapReadOnly>()(x));
  // The final argument (bytes=true) forces the return value to be 0->255
  NDArray bytes{pyobj().attr("to_rgba")(ndarrayView, alpha, true)};
  // sanity check
  auto shape = bytes.get_shape();
  if (bytes.get_typecode() == 'B' && bytes.get_nd() == 2 && static_cast<size_t>(shape[0]) == x.size() &&
      shape[1] == 4) {
    // The returned array is of shape (x.size(), 4)
    auto bytesRaw = reinterpret_cast<std::uint8_t *>(bytes.get_data());
    size_t bytesIndex{0};
    for (auto &rgb : rgbaVector) {
      rgb = qRgba(bytesRaw[bytesIndex], bytesRaw[bytesIndex + 1], bytesRaw[bytesIndex + 2], bytesRaw[bytesIndex + 3]);
      bytesIndex += 4;
    }
  } else {
    QString msg = QString("Unexpected return type from "
                          "ScalarMappable.to_rgba. Expected "
                          "np.array(dtype=B) with shape (%1, 4) "
                          "but found np.array(dtype=%2) with shape (%3, %4). "
                          "Cannot continue. Please contact development team.")
                      .arg(QString::number(x.size()), QString::number(bytes.get_typecode()), QString::number(shape[0]),
                           QString::number(shape[1]));
    throw std::runtime_error(msg.toLatin1().constData());
  }
  return rgbaVector;
}

} // namespace MantidQt::Widgets::MplCpp
