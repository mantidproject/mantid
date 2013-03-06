#!/usr/bin/env python
from lxml import etree as le # python-lxml on rpm based systems
import lxml.html
from lxml.html import builder as lh
import os

OUTPUTDIR = "generated"
WEB_BASE  = "http://www.mantidproject.org/"

class QHPFile:
    def __init__(self, namespace, folder="doc"):
        self.__root = le.Element("QtHelpProject",
                                 **{"version":"1.0"})

        self.__addtxtele(self.__root, "namespace", namespace)
        self.__addtxtele(self.__root, "virtualFolder", folder)

        self.__filterSect = le.SubElement(self.__root, "filterSection")

        self.__keywordsEle = None # should have 'keywords' section
        self.__keywords = []
        self.__filesEle = None # should have 'files' section
        self.__files = []

    def addTOC(self, contents):
        """
        The contents must all be specified at once. They should be in the
        form of [(title1, ref1), (title2, ref2), ...].
        """
        toc = le.SubElement(self.__filterSect, "toc")
        for (title, ref) in contents:
            le.SubElement(toc, "section",
                          **{"title":title, "ref":ref})

    def addFile(self, filename, keyword=None):
        if not keyword is None:
            if self.__keywordsEle is None:
                self.__keywordsEle = le.SubElement(self.__filterSect, "keywords")
            if not keyword in self.__keywords:
                le.SubElement(self.__keywordsEle, "keyword",
                              **{"name":keyword, "ref":os.path.split(filename)[1]})
                self.__keywords.append(keyword)
        if self.__filesEle is None:
            self.__filesEle = le.SubElement(self.__filterSect, "files")
        if not filename in self.__files:
            self.__addtxtele(self.__filesEle, "file", filename)
            self.__files.append(filename)

    def __addtxtele(self, parent, tag, text):
        ele = le.SubElement(parent, tag)
        ele.text = text
        return ele

    def __str__(self):
        return le.tostring(self.__root, pretty_print=True)#,
    #xml_declaration=True)

    def write(self, filename):
        """
        Write the help configuration to the given filename.
        """
        handle = open(filename, 'w')
        handle.write(str(self))
