from assistant_common import WEB_BASE, HTML_DIR, addEle, addTxtEle
import re

def formatImgHtml(raw):
    #print "RAW:", raw

    # cleanup the tag from the text
    index = raw.index(':') # look for first ':'
    if index < 0:
        index = 0
    else:
        index += 1
    raw = raw[index:-2]
    
    #print "RAW:", raw

    # chop tag into something more workable
    components = raw.split('|')
    img = components[0] # image filename is always first
    
    # get the other bits of meta-data
    alt = None
    caption = None
    for item in components[1:]:
        item_low = item.lower()
        if item_low == "thumb":
            pass
        elif item.endswith('px'):
            pass
        elif item_low == "right" or item_low == "center":
            pass
        elif item_low.startswith("alt"):
            alt = '='.join(item.split('=')[1:])
        else:
            caption = item


    html = "<figure>"
    html += "<img src='img/" + img + "'"
    if alt is not None:
        html += " alt='%s'" % alt
    html += "/>"
    if caption is not None:
        html += "\n<figcaption>%s</figcaption>\n" % caption
    html += "</figure>\n"
    #print "HTML:", html
    return (img, html)

class MediaWiki:
    def __init__(self, htmlfile):
        self.__file = htmlfile
        self.__types = []
        self.images = []

    def __parseImgs(self, text):
        # Get all of the raw image links
        raw_img = re.findall(r'\[\[Image:.*\]\]', text, flags=re.MULTILINE)
        raw = re.findall(r'\[\[File:.*\]\]', text, flags=re.MULTILINE)
        raw.extend(raw_img)

        # generate the html
        html = []
        for src in raw:
            (imagefile, newtxt) = formatImgHtml(src)
            self.images.append(imagefile)
            html.append(newtxt)

        for (orig, repl) in zip(raw, html):
            #print ">>>", orig
            #print "<<<", repl
            text = text.replace(orig, repl)

        return text.strip()

    def __clearEmpty(self, text):
        result = []
        for line in text.split("\n"):
            if len(line.strip()) <= 0:
                result.append("")
            else:
                result.append(line)
        return "\n".join(result)
        

    def __fixUL(self, text):
        if text.find('*') < 0:
            return text # no candidates

        # lines that start with '*'
        starts = []
        text = text.split("\n")
        for (i, line) in zip(range(len(text)), text):
            if line.strip().startswith("*"):
                starts.append(i)

        # none of the stars were at the start of a line
        if len(starts) <= 0:
            return "\n".join(text)

        # lines where the list item ends
        text.append("")
        stops = starts[:]
        for i in range(len(starts)):
            # next line is the next item
            if starts[i]+1 <= len(starts) \
                    and starts[i]+1 == starts[i+1]:
                continue

            # look for next blank line
            index = text.index("", stops[i])
            if index > stops[i]:
                if i+1 < len(starts) and index < starts[i+1]:
                    stops[i] = index

        # compress each list item into a single line
        starts.reverse()
        stops.reverse()
        for (start, stop) in zip(starts,stops):
            if start == stop:
                continue
            #print "--->", start, "->", stop, "XXX".join(text[start:stop])
            text[start] = " ".join(text[start:stop])
            del text[start+1:stop]

        del text[-1] # we added in a tiny bit of text
        return "\n".join(text)

    def __fixHEADERS(self, text):
        results = re.findall(r'\s+==.*==\s+', text)
        for item in results:
            text = text.replace(item, "\n\n"+item.strip()+"\n\n")
        results = re.findall(r'^=====.*=====$', text, flags=re.MULTILINE)
        for item in results:
            formatted = "<h4>" + item[5:-5] + "</h4>"
            text = text.replace(item, formatted)
        results = re.findall(r'^====.*====$', text, flags=re.MULTILINE)
        for item in results:
            formatted = "<h3>" + item[4:-4] + "</h3>"
            text = text.replace(item, formatted)
        results = re.findall(r'^===.*===$', text, flags=re.MULTILINE)
        for item in results:
            formatted = "<h2>" + item[3:-3] + "</h2>"
            text = text.replace(item, formatted)
        results = re.findall(r'^==.*==$', text, flags=re.MULTILINE)
        for item in results:
            formatted = "<h1>" + item[2:-2] + "</h1>"
            text = text.replace(item, formatted)
        return text


    def __annotate(self, text):
        for line in text:
            stripped = line.strip()
            if len(stripped) <= 0:
                #self.__types.append(None)
                self.__types.append("blank")
            elif stripped.startswith('*'):
                stuff = stripped.split()[0]
                stuff = stuff.replace("*", "U")
                stuff = stuff.replace("#", "O")
                self.__types.append(stuff)
            else:
                self.__types.append(None)

    def parse(self, text):
        #print "00>>>", text, "<<<"
        text = text.strip()
        if len(text) <= 0:
            return # don't bother if it is empty
        text = self.__parseImgs(text)
        #print "01>>>", text, "<<<"
        if text.startswith("== Deprecation notice =="):
            stuff = "== Deprecation notice =="
            text = text.replace(stuff, stuff+"\n")
        #print "02>>>", text, "<<<"
        text = self.__clearEmpty(text)
        #print "03>>>", text, "<<<"
        text = self.__fixHEADERS(text)
        #print "04>>>", text, "<<<"
        text = self.__fixUL(text)

        text = text.split("\n")

        self.__annotate(text)

        self.__file.openTag('p')
        list_stack = ""
        num_lines = len(text)
        for i in range(num_lines):
            annotate = self.__types[i]
            line = text[i]
            if annotate is not None:
                if annotate.startswith("U") or annotate.startswith("O"):
                    if list_stack != annotate:
                        self.__file.write("<ul>\n")  # TODO
                        list_stack = annotate        # TODO
                    line = ' '.join(line.split()[1:])
                    line = "<li>"+line+"</li>" 
                else:
                    if len(list_stack) > 0:
                        self.__file.write("</ul>\n") # TODO
                    if annotate == "blank":
                        if not text[i-1].startswith("<h"):
                            self.__file.write("</p>\n")
                        if i+1 < num_lines and not text[i+1].startswith("<h"):
                            self.__file.write("<p>")
                    else:
                        self.__file.write(annotate)
            self.__file.write(line + "\n")
        self.__file.closeTag()
