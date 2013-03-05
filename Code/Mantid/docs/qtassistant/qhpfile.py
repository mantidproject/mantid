#!/usr/bin/env python
from xml.dom.minidom import Document

class QHPFile:
    def __init__(self, namespace, folder="doc"):
        self.__doc = Document()
        self.__root = self.__addele(self.__doc,"QtHelpProject",
                                    {"version":"1.0"})
        self.__doc.appendChild(self.__root)

        self.__addtxtele(self.__root, "namespace", namespace)
        self.__addtxtele(self.__root, "virtualFolder", folder)


        self.__filterSect =  self.__addele(self.__root, "filterSection")

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
                self.__keywordsEle = self.__addele(self.__filterSect, "keywords")
            if not keyword in self.__keywords:
                self.__addele(self.__keywordsEle, "keyword",
                              {"name":keyword, "ref":filename})
                self.__keywords.append(keyword)
        if self.__filesEle is None:
            self.__filesEle = self.__addele(self.__filterSect, "files")
        if not filename in self.__files:
            self.__addtxtele(self.__filesEle, "file", filename)
            self.__files.append(filename)

    def __addtxtele(self, parent, tag, text):
        ele = self.__addele(parent, tag)
        text = self.__doc.createTextNode(text)
        ele.appendChild(text)
        return ele

    def __addele(self, parent, tag, attrs={}):
        ele = self.__doc.createElement(tag)
        for key in attrs.keys():
            ele.setAttribute(key, attrs[key])
        parent.appendChild(ele)
        return ele

    def __str__(self):
        return self.__doc.toprettyxml(indent="  ")
    #xml_declaration=True)

    def write(self, filename):
        """
        Write the help configuration to the given filename.
        """
        handle = open(filename, 'w')
        handle.write(str(self))
