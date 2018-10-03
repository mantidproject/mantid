# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)
import os
import re
import urllib2
from mantid.api import AlgorithmFactory


def readWebPage(url):
    proxy = urllib2.ProxyHandler({'http': 'wwwcache.rl.ac.uk:8080'})
    opener = urllib2.build_opener(proxy)
    urllib2.install_opener(opener)
    aResp =urllib2.urlopen(url)
    web_pg = aResp.read()
    return web_pg


def ticketExists(alg, ticketHash):
    retVal = ""
    algPattern = re.compile(alg, re.IGNORECASE)
    for ticket in ticketHash:
        ticketText = ticketHash[ticket]
        if algPattern.search(ticketText):
            retVal = str(ticket)
            break
    return retVal


def outputError(alg, algVersion, description, notes=""):
    print("%s, %i, %s, %s" % (alg, algVersion, description, notes))


rstdir = r"C:\Mantid\Code\Mantid\docs\source\algorithms"
ticketList = [9582,9586,9607,9610,9704,9804,9726]

ticketHash = {}
for ticket in ticketList:
    ticketHash[ticket] = readWebPage( r"http://trac.mantidproject.org/mantid/ticket/" + str(ticket))

usagePattern = re.compile('Usage', re.IGNORECASE)
excusesPattern = re.compile('(rarely called directly|designed to work with other algorithms|only used for testing|deprecated)',
                            re.IGNORECASE)


algs = AlgorithmFactory.getRegisteredAlgorithms(True)
for alg in algs:
    algVersion = algs[alg][0]
    fileFound = False
    filename = os.path.join(rstdir,alg + "-v" + str(algVersion) + ".rst")
    if os.path.exists(filename):
        with open (filename, "r") as algRst:
            fileFound = True
            algText = algRst.read()
            if (usagePattern.search(algText) is None) and (excusesPattern.search(algText) is None):
                #check if already in a ticket
                usageTicket = ticketExists(alg,ticketHash)
                outputError(alg, algVersion, "No usage section", usageTicket)
    if not fileFound:
        outputError(alg, algVersion, "File not found")
