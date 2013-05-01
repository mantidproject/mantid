import logging
import os
import re
import urllib

WIKI_BASE_URL = "http://www.mantidproject.org"
DOWN_BASE_URL = "http://download.mantidproject.org/"
ALGO_BASE_URL = DOWN_BASE_URL + "algorithm_screenshots/ScreenShotImages"

def wiki_fig_key(stuff):
    filename = os.path.split(stuff)[-1]
    # aaa will come first for things that aren't thumbnails
    prefix = "aaa/"
    # thumnails are bad
    if stuff.startswith('/images/thumb/'):
        prefix = "zzz/"
        index = filename.find('px-')
        if index > 0:
            resolution = int(filename[:index])
            filename = filename[index+3:]
        else:
            resolution = 0
        prefix += "%06d/" % resolution
    stuff = prefix + filename
    #print stuff
    return stuff

class Fetcher:
    def __init__(self, filename, shrinkname=True):
        self._logger = logging.getLogger(__name__+'.'+self.__class__.__name__)
        self.filename = filename
        shortfilename = str(os.path.split(self.filename)[-1])
        if shrinkname:
            self.filename = shortfilename
        self._logger.info(self.filename)

        # download wiki page name
        url = WIKI_BASE_URL + "/File:" + self.filename
        handle = urllib.urlopen(url)
        text = handle.read()
        handle.close()
        text_len = len(text)

        candidates = []

        # find the location of the file from the image tags
        candidates.extend(self._getWikiCandidates(shortfilename, text, "<img alt=", "/>"))

        # find the location of the file from the image tags
        candidates.extend(self._getWikiCandidates(shortfilename, text, '<a href="/images/', '">'))

        # sort them according to favorites and remove duplicates
        candidates = list(set(candidates))
        candidates.sort(key=wiki_fig_key)
        self._logger.info("candidates:" + str(candidates))

        # always choose the first one
        if len(candidates) <= 0:
            raise RuntimeError("Failed to find any candidates")
        rel_link = candidates[0]
        self._logger.debug("rel_link = '%s'" % rel_link)
        self.url = WIKI_BASE_URL + rel_link

    def _getWikiCandidates(self, shortname, text, starttag, endtag):
        candidates = []
        text_len = len(text)
        index = 0
        while index >= 0 and index < text_len:
            index = text.find(starttag, index)
            end = text.find(endtag, index)
            self._logger.debug("indices: %d to %d" % (index, end))
            if end < index or index < 0:
                break
            stuff = text[index:end]
            index = end
            self._logger.debug(stuff)
            #if not shortfilename in stuff:
            #    continue
            attrs = stuff.split()
            for attr in attrs:
                if '/images/' in attr:
                    rel_link = attr.split('=')[-1][1:]
                    if rel_link.endswith('"') or rel_link.endswith("'"):
                        rel_link = rel_link[:-1]
                    if not "archive" in rel_link:
                        if not rel_link.startswith('/images/thumb'):
                            candidates.append(rel_link)
        return candidates
        

    def download(self, destdir):
        destname = os.path.join(destdir, self.filename)
        print "Downloading %s from %s" % (destname, self.url)
        urllib.urlretrieve(self.url, filename=destname)
