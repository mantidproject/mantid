class HtmlWriter:
    def __init__(self, filename, title=None):
        self.name = filename
        self.__handle = open(filename, 'w')
        self.__tagstack = []
        self.openTag("html")
        if title is not None:
            self.nl()
            self.openTag("head")
            self.openTag("title")
            self.write(title)
            self.closeTag()
            self.closeTag()
        self.nl()
        self.openTag("body")

    def write(self, text):
        self.__handle.write(text)

    def nl(self):
        """Write out a new line"""
        self.write("\n" + (len(self.__tagstack)*"  "))

    def emptyTag(self, tag, attrs={}, newline=False):
        self.write("<" + tag)
        for key in attrs.keys():
            self.write(" " + key + "=\"" + attrs[key] + "\"")
        self.write("/>")
        if newline:
            self.nl()

    def openTag(self, tag, attrs={}):
        self.__tagstack.append(tag)
        self.write("<" + tag)
        for key in attrs.keys():
            self.write(" " + key + "=\"" + attrs[key] + "\"")
        self.write(">")

    def closeTag(self, newline=False):
        tag = self.__tagstack.pop()
        self.write("</" + tag + ">")
        if newline:
            self.nl()

    def addTxtEle(self, tag, text, attrs={}, newline=False):
        self.openTag(tag, attrs)
        self.write(text)
        self.closeTag(newline)

    def hr(self):
        self.emptyTag("hr", newline=True)

    def h1(self, text, newline=True):
        self.addTxtEle("h1", text, newline=newline)

    def h2(self, text, newline=True):
        self.addTxtEle("h2", text, newline=newline)

    def h3(self, text, newline=True):
        self.addTxtEle("h3", text, newline=newline)

    def link(self, text, href):
        self.openTag("a", {"href":href})
        self.write(text)
        self.closeTag()

    def writeRow(self, cells, header=False):
        tag = "td"
        if header:
            tag = "th"

        self.openTag("tr")
        for cell in cells:
            self.openTag(tag)
            self.write(cell)
            self.closeTag()
        self.closeTag(True)

    def p(self, text, newline=True):
        self.addTxtEle("p", text, newline=newline)
