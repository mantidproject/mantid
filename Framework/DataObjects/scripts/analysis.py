# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Analysis of results from mdevents_optimize.py and others

@author: Janik Zikovsky
"""

# This line has to be first for some reason.
# from enthought.mayavi import mlab

from pylab import figure, legend, plot, title, savefig, show, xlabel, ylabel
import pickle
from scipy import stats


class Params:
    def __init__(self):
        self.NumberEvents = 0
        self.MaxRecursionDepth = 10
        self.MakeTime = 0
        self.CoarseBinTime = 0
        self.MediumBinTime = 0
        self.FineBinTime = 0
        self.MemoryUsed = 0


results = []


def plot_results_vs_other(results, x_field, y_field, other_field, extra_title=""):
    """Function to plot Y vs X of anything. It accesses the members of "results" to plot them.
    other_field is used to separate by another field, and make separate line plots for each"""
    others = set()
    for par in results:
        others.add(eval("par.%s" % other_field))
    others = list(others)
    others.sort()

    figure()

    for other in others:
        data = []
        for _ in results:
            this_other = eval("par.%s" % other_field)
            if this_other == other:
                x = eval("par.%s" % x_field)
                y = eval("par.%s" % y_field)
                data.append((x, y))
        data.sort()
        xs = [x for (x, y) in data]
        ys = [y for (x, y) in data]
        plot(xs, ys, marker=".", label="%s = %f" % (other_field, other))

    if extra_title != "":
        extra_title = "\n" + extra_title
    title("%s vs %s%s" % (y_field, x_field, extra_title))
    xlabel(x_field)
    ylabel(y_field)
    legend(loc="best")
    savefig("%s_vs_%s.png" % (y_field, x_field))


def plot_results_with_slope(results, x_field, y_field, x_scale=1.0):
    """Function to plot Y vs X of anything. It accesses the members of "results" to plot them.
    other_field is used to separate by another field, and make separate line plots for each

    @param x_scale :: multiply x by this amount
    """
    figure()

    data = []
    for _ in results:
        x = eval("par.%s" % x_field)
        y = eval("par.%s" % y_field)
        data.append((x, y))
    data.sort()
    xs = [x * x_scale for (x, y) in data]
    ys = [y for (x, y) in data]

    # Now get the slope
    gradient, intercept, r_value, p_value, std_err = stats.linregress(xs, ys)

    plot(xs, ys, marker=".", label="y = %.3gx + %.3g" % (gradient, intercept))

    title("%s vs %s" % (y_field, x_field))
    xlabel("%s x %s" % (x_field, x_scale))
    ylabel(y_field)
    legend(loc="best")
    savefig("%s_vs_%s.png" % (y_field, x_field))


def do_analysis(file_list, type):
    # Load back the results
    results = []
    for filename in file_list:
        f = open(filename, "r")
        these_results = pickle.load(f)
        results += these_results
        f.close()

    if type == 1:
        plot_results_vs_other(results, "SplitInto", "MakeTime", "SplitThresholdBase")
        plot_results_vs_other(results, "SplitInto", "MemoryUsed", "SplitThresholdBase")
        plot_results_vs_other(results, "SplitInto", "CoarseBinTime", "SplitThresholdBase")
        plot_results_vs_other(results, "SplitInto", "MediumBinTime", "SplitThresholdBase")
        plot_results_vs_other(results, "SplitInto", "FineBinTime", "SplitThresholdBase")

    elif type == 2:
        plot_results_with_slope(results, "NumberEvents", "MakeTime", x_scale=1e-9)
        plot_results_with_slope(results, "NumberEvents", "MemoryUsed", x_scale=1e-9)
        plot_results_with_slope(results, "NumberEvents", "CoarseBinTime", x_scale=1e-9)
        plot_results_with_slope(results, "NumberEvents", "MediumBinTime", x_scale=1e-9)
        plot_results_with_slope(results, "NumberEvents", "FineBinTime", x_scale=1e-9)

    elif type == 3:
        extra_title = "Binary Splitting Method"
        plot_results_vs_other(results, "SplitThreshold", "MakeTime", "SplitInto", extra_title)
        plot_results_vs_other(results, "SplitThreshold", "MemoryUsed", "SplitInto", extra_title)
        plot_results_vs_other(results, "SplitThreshold", "CoarseBinTime", "SplitInto", extra_title)
        plot_results_vs_other(results, "SplitThreshold", "MediumBinTime", "SplitInto", extra_title)
        plot_results_vs_other(results, "SplitThreshold", "FineBinTime", "SplitInto", extra_title)
    show()


if __name__ == "__main__":
    file_list = ["optimize_results1.dat"]
    do_analysis(file_list, 1)
