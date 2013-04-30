#!/usr/bin/env python
import logging
import os
import re
from fetchimg import Fetcher

from mediawiki import IMG_NOT_FOUND

def getHtml(htmldir):
    """
    Recursively find all html files in the supplied directory.
    @param htmldir The base directory for the html site.
    @returns All html files with full path.
    """
    candidates = os.listdir(htmldir)
    results = []
    for candidate in candidates:
        if os.path.isdir(candidate):
            results.extend(getHtml(os.path.join(htmldir, candidate)))
        elif candidate.endswith('.html'):
            results.append(os.path.join(htmldir, candidate))
    return results

def processHtml(htmldir, filename):
    """
    @param htmldir The base directory for the html site.
    @param filename The html file to parse and look for missing images in.
    @returns All Missing image files.
    """
    logger = logging.getLogger("processHtml")
    logger.info("processHtml(%s, %s)" % (htmldir, filename))

    # read in the entire html file
    handle = file(filename, 'r')
    text = handle.read()
    handle.close()

    ##### determine all the requested images
    # that aren't set to IMG_NOT_FOUND
    candidates = re.findall(r"img(.+)/>", text, flags=re.MULTILINE)
    if len(candidates) <= 0:
        return []
    options = []
    for candidate in candidates:
        start = candidate.find("src=")
        if start <= 0:
            continue
        candidate = candidate[start+5:]
        end = candidate.find('"')
        if end <= start:
            end = candidate.find("'")
        if end <= start:
            continue
        options.append(candidate[:end])
    # that are set to IMG_NOT_FOUND
    if IMG_NOT_FOUND in text:
        logger.info("IMAGE_NOT_FOUND in '%s'" % filename)
        candidates = []
        index = 0
        while index >= 0:
            index = text.find(IMG_NOT_FOUND, index)
            end = text.find("</figure>", index)
            if end < index or index < 0:
                break
            figs = re.findall(r'Missing image:\s+(.+)</figcaption>',
                              text[index:end])
            candidates.extend(figs)
            index += len(IMG_NOT_FOUND)
            logger.info("CANDIDATES: %s" % str(candidates))
        options.extend(candidates)

    # add them to the list of missing images if not found
    results = []
    for candidate in options:
        candidate = os.path.join(htmldir, candidate)
        logger.info("looking for '%s'" % candidate)
        if not os.path.exists(candidate):
            logger.info("candidate = '%s' not found" % candidate)
            results.append(candidate)

    # return everything that isn't found
    return results


if __name__ == "__main__":
    # set up the command line option parser
    import optparse
    parser = optparse.OptionParser(usage="usage: %prog [options] <htmldir>",
                                   description="Determine if there are images missing from the built documentation.")
    parser.add_option('', '--shortnames', dest='shortnames',
                      default=False, action="store_true",
                      help="Only print the names of missing images rather than full path")
    parser.add_option('', '--nosummary', dest='summary',
                      default=True, action="store_false",
                      help="Turn off the summary information")
    parser.add_option('', '--download', dest="downloadarea",
                      default=None,
                      help="Download the missing images from the mantid wiki to the specified directory")
    parser.add_option('', '--loglevel', dest="loglevel",
                      default="warn",
                      help="Set the logging level (options are 'debug', 'info', 'warn', 'error')")
    parser.add_option('', '--logfile', dest="logfile",
                      default=None,
                      help="Set filename to log to")
    (options, args) = parser.parse_args()

    # get the html base directory
    if len(args) <= 0:
        parser.error("Failed to specify a html directory to parse")
    htmldir = os.path.abspath(args[0])
    if not os.path.exists(htmldir):
        parser.error("Must specify an existing html directory")

    # configure the logger
    if options.loglevel.startswith('debug'):
        options.loglevel=logging.DEBUG
    elif options.loglevel.startswith('info'):
        options.loglevel=logging.INFO
    elif options.loglevel.startswith('warn'):
        options.loglevel=logging.WARNING
    elif options.loglevel.startswith('error'):
        options.loglevel=logging.ERROR
    else:
        parser.error("Failed to specify valid log level: '%s'" % options.loglevel)
    logging.basicConfig(filename=options.logfile, level=options.loglevel)

    # get the list of html files
    htmlfiles = getHtml(htmldir)
    if options.summary:
        print "Verifying %d html files in '%s'" % (len(htmlfiles), htmldir)

    # determine what images are missing
    missing = []
    for filename in htmlfiles:
        missing.extend(processHtml(htmldir, filename))

    # remove repeated filenames
    missing = list(set(missing))
    missing.sort()

    # print the results
    if options.summary:
        print "Missing %d image files" % len(missing)
    for filename in missing:
        if options.shortnames:
            print os.path.split(filename)[-1]
        else:
            print filename

    if options.downloadarea is not None:
        for filename in missing:
            fetcher = Fetcher(filename)
            fetcher.download(options.downloadarea)
