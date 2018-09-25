#include "MantidQtWidgets/MplCpp/ScalarMappable.h"
#include "MantidQtWidgets/MplCpp/Colormap.h"

#include "MantidPythonInterface/core/NDArray.h"

using Mantid::PythonInterface::NDArray;

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

namespace {

Python::Object createScalarMappable(const NormalizeBase &norm,
                                    const Colormap &cmap) {
  return cmModule().attr("ScalarMappable")(norm.pyobj(), cmap.pyobj());
}
} // namespace

/**
 * @brief Construct a ScalarMappable instance with the given normalization
 * type and colormap
 * @param norm Instance use to define the mapping from data to [0,1]
 * @param cmap A Colormap defning the RGBA values to use for drawing
 */
ScalarMappable::ScalarMappable(const NormalizeBase &norm, const Colormap &cmap)
    : Python::InstanceHolder(createScalarMappable(norm, cmap), "to_rgba") {
  // The internal array needs to be set to some iterable but apparently
  // it is not used:
  // https://stackoverflow.com/questions/28801803/matplotlib-scalarmappable-why-need-to-set-array-if-norm-set
  pyobj().attr("set_array")(Python::NewRef(Py_BuildValue("()")));
}

/**
 * @brief ScalarMappable::toRGBA
 * @param x
 * @param alpha
 * @return
 */
QRgb ScalarMappable::toRGBA(double x, double alpha) const {
  // Sending the first argument as an iterable gives a numpy array back.
  // The final argument (bytes=true) forces the return value to be 0->255
  NDArray rgba{pyobj().attr("to_rgba")(Python::NewRef(Py_BuildValue("(f)", x)),
                                       alpha, true)};
  // sanity check
  auto shape = rgba.get_shape();
  if (rgba.get_typecode() == 'B' && rgba.get_nd() == 2 && shape[0] == 1 &&
      shape[1] == 4) {
    auto bytes = reinterpret_cast<std::uint8_t *>(rgba.get_data());
    return qRgba(bytes[0], bytes[1], bytes[2], bytes[3]);
  } else {
    std::string msg = "Unexpected return type from "
                      "ScalarMappable.to_rgba. Expected "
                      "np.array(dtype=B) with shape (1,4) but found "
                      "np.array(dtype=";
    msg.append(std::to_string(rgba.get_typecode()))
        .append(") with shape (")
        .append(std::to_string(shape[0]))
        .append(",")
        .append(std::to_string(shape[1]))
        .append(")");
    throw std::runtime_error(std::move(msg));
  }
}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
