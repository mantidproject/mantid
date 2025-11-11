# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np


def _do_single_plot(ax, workspace, index, errors, plot_kwargs):
    plot_fn = ax.errorbar if errors else ax.plot
    plot_kwargs["wkspIndex"] = index
    plot_fn(workspace, **plot_kwargs)


def get_y_min_max_between_x_range(line, x_min, x_max, y_min, y_max):
    x, y = line.get_data()
    start = np.searchsorted(x, x_min)
    end = np.searchsorted(x, x_max)
    crop = y[start:end]
    if start == end:
        return y_min, y_max
    current_min = np.min(crop)
    current_max = np.max(crop)
    y_min = min(y_min, current_min)
    y_max = max(y_max, current_max)
    return y_min, y_max


def get_num_row_and_col(num_axis):
    n_rows, n_cols = 0, 0
    # num axis is the number of axis in use
    sqrt = np.floor(np.sqrt(num_axis))
    if sqrt**2 == num_axis:
        n_rows = sqrt
        n_cols = sqrt
    else:
        # cols increment first
        n_cols = sqrt + 1
        n_rows = n_cols
        if n_cols * (n_cols - 1) >= num_axis:
            n_rows -= 1
    return int(n_rows), int(n_cols)


def convert_index_to_row_and_col(index, n_rows, n_cols):
    # number of completed rows
    row = np.floor(index / n_cols)
    col = index - row * (n_cols)
    return row, col
