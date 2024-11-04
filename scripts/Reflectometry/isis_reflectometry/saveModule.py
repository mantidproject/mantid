# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from PyQt4 import QtCore
from mantid.api import mtd
import numpy as n

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:

    def _fromUtf8(s):
        return s


def saveCustom(idx, fname, sep=" ", logs=[], title=False, error=False):
    fname += ".dat"
    print("FILENAME: ", fname)
    a1 = mtd[str(idx.text())]
    titl = "#" + a1.getTitle() + "\n"
    x1 = a1.readX(0)
    X1 = n.zeros((len(x1) - 1))
    for i in range(0, len(x1) - 1):
        X1[i] = (x1[i] + x1[i + 1]) / 2.0
    y1 = a1.readY(0)
    e1 = a1.readE(0)
    f = open(fname, "w")
    if title:
        f.write(titl)
    samp = a1.getRun()
    for log in logs:
        prop = samp.getLogData(str(log.text()))
        headerLine = "#" + log.text() + ": " + str(prop.value) + "\n"
        print(headerLine)
        f.write(headerLine)
    qres = (X1[1] - X1[0]) / X1[1]
    print("Constant dq/q from file: ", qres)
    for i in range(len(X1)):
        if error:
            dq = X1[i] * qres
            s = "%e" % X1[i] + sep + "%e" % y1[i] + sep + "%e" % e1[i] + sep + "%e" % dq + "\n"
        else:
            s = "%e" % X1[i] + sep + "%e" % y1[i] + sep + "%e" % e1[i] + "\n"
        f.write(s)
    f.close()


def saveANSTO(idx, fname):
    fname += ".txt"
    print("FILENAME: ", fname)
    a1 = mtd[str(idx.text())]
    x1 = a1.readX(0)
    X1 = n.zeros((len(x1) - 1))
    for i in range(0, len(x1) - 1):
        X1[i] = (x1[i] + x1[i + 1]) / 2.0
    y1 = a1.readY(0)
    e1 = a1.readE(0)
    sep = "\t"
    f = open(fname, "w")
    qres = (X1[1] - X1[0]) / X1[1]
    print("Constant dq/q from file: ", qres)
    for i in range(len(X1)):
        dq = X1[i] * qres
        s = "%e" % X1[i] + sep + "%e" % y1[i] + sep + "%e" % e1[i] + sep + "%e" % dq + "\n"
        f.write(s)
    f.close()


def saveMFT(idx, fname, logs):
    fname += ".mft"
    print("FILENAME: ", fname)
    a1 = mtd[str(idx.text())]
    x1 = a1.readX(0)
    X1 = n.zeros((len(x1) - 1))
    for i in range(0, len(x1) - 1):
        X1[i] = (x1[i] + x1[i + 1]) / 2.0
    y1 = a1.readY(0)
    e1 = a1.readE(0)
    sep = "\t"
    f = open(fname, "w")
    f.write("MFT\n")
    f.write("Instrument: " + a1.getInstrument().getName() + "\n")
    f.write("User-local contact: \n")
    f.write("Title: \n")
    samp = a1.getRun()
    s = "Subtitle: " + samp.getLogData("run_title").value + "\n"
    f.write(s)
    s = "Start date + time: " + samp.getLogData("run_start").value + "\n"
    f.write(s)
    s = "End date + time: " + samp.getLogData("run_end").value + "\n"
    f.write(s)
    for log in logs:
        prop = samp.getLogData(str(log.text()))
        headerLine = log.text() + ": " + str(prop.value) + "\n"
        print(headerLine)
        f.write(headerLine)
    f.write("Number of file format: 2\n")
    s = "Number of data points:\t" + str(len(X1)) + "\n"
    f.write(s)
    f.write("\n")
    f.write("\tq\trefl\trefl_err\tq_res\n")
    qres = (X1[1] - X1[0]) / X1[1]
    print("Constant dq/q from file: ", qres)
    for i in range(len(X1)):
        dq = X1[i] * qres
        s = "\t%e" % X1[i] + sep + "%e" % y1[i] + sep + "%e" % e1[i] + sep + "%e" % dq + "\n"
        f.write(s)
    f.close()
