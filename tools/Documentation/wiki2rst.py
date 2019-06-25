#!/usr/bin/python
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""Accepts a list of page names on the wiki and translates them to .rst

The bulk of the work is done by pandoc, which is expected to be in the PATH. There is then some post processing to:

  - downloading and correcting image links
  - add links for algorithms, fit functions and common concepts (workspace types etc)


 Limitations:
 1. in adding text to links or images in tables the table formatting will be disrupted
 2. pandoc creates some images in an inline format, these cannot have the align tags added
    back on, this is marked with a comment, the solution is probably to move the image from
    the inline to normal format.
 3. Image links cannot have spaces in the filename in rst.
    The spaces are removed in the downloaded file names, but not the links in the rst files."""
from __future__ import (absolute_import, division, print_function)
import argparse
import json
import os
import re
import subprocess
import sys
import tempfile
import urllib2

import mantid

DEBUG = 0


def parse_arguments(argv):
    parser = argparse.ArgumentParser(description='Converts mediawiki pages to rst pages for sphinx.')
    parser.add_argument('pagename', help='The name of a page on the wiki site')
    parser.add_argument('-o', '--output-file',
                        help='Provide a path to output to a file. If empty the text will go to stdout')
    parser.add_argument('--images-dir', default="../images",
                        help='Provide a relative path from the file to the images directory')
    parser.add_argument('-rp', '--ref-link-prefix',
                        help='Provide a reference link prefix')
    parser.add_argument('-r', '--ref-link',
                        help='Provide a reference link')
    parser.add_argument('--index_url', default='http://www.mantidproject.org/',
                        help='The url of the main wiki landing page')
    parser.add_argument('--add_handle', action='store_true', dest='add_page_handle',
                        help='Should we add a heading to this page?', default=False)
    parser.add_argument('--page_handle',
                        help='The page handle to add at the top of the rst pages')
    parser.add_argument('--add_heading', action='store_true', dest='add_heading',
                        help='Should we add a heading to this page?', default=False)
    return parser.parse_args(argv)

# ------------------------------------------------------------------------------


def display_info(*args):
    print(*args)


def display_debug(*args):
    if DEBUG == 1:
        print(*args)

# ------------------------------------------------------------------------------


class WikiURL(object):

    def __init__(self, indexurl, pagename):
        self._indexurl = indexurl
        self._pagename = pagename

    @property
    def indexurl(self):
        return self._indexurl

    @property
    def pagename(self):
        return self._pagename

    def pretty(self):
        return self.indexurl + self.pagename

    def raw(self):
        return self.ugly() + "&action=raw"

    def ugly(self):
        return self.indexurl + "index.php?title=" + self.pagename

    def fileurl(self, name):
        apicall_url = self.indexurl + \
                  "api.php?titles=File:{0}&action=query&prop=imageinfo&iiprop=url&format=json".format(name.replace(" ", "_"))
        try:
            json_str = urllib2.urlopen(apicall_url).read()
        except urllib2.URLError as err:
            raise RuntimeError("Failed to fetch JSON describing image page: {}".format(str(err)))
        apicall = json.loads(json_str)
        pages = apicall['query']['pages']
        for _, page in pages.iteritems():
            return page['imageinfo'][0]['url']

# ------------------------------------------------------------------------------


def fetch_wiki_markup(wiki_url):
    try:
        response = urllib2.urlopen(wiki_url.raw())
        return response.read()
    except urllib2.URLError as err:
        raise RuntimeError("Failed to fetch wiki page: {}".format(str(err)))

# ------------------------------------------------------------------------------


def to_rst(wiki_url, post_process_args):
    wiki_markup_text = fetch_wiki_markup(wiki_url)
    display_debug("Converting wiki markup '{}'".format(wiki_markup_text))
    pandoc_rst = run_pandoc(wiki_markup_text)
    mantid_rst = post_process(pandoc_rst, wiki_url, wiki_markup_text,
                              post_process_args)
    return mantid_rst

# ------------------------------------------------------------------------------


def run_pandoc(wiki_markup_text):
    wiki_tmp_file = tempfile.NamedTemporaryFile(delete=True)
    wiki_tmp_file.write(wiki_markup_text)
    wiki_tmp_file.flush()
    cmd = "pandoc"
    args = ["-f", "mediawiki", "-t", "rst", wiki_tmp_file.name]
    try:
        rst_text = execute_process(cmd, args)
    except Exception as err:
        raise RuntimeError("Error executing pandoc: '{}'".format(str(err)))
    finally:
        wiki_tmp_file.close()

    return rst_text

# ------------------------------------------------------------------------------


def execute_process(cmd, args):
    """Execute the command with the given arguments.
    Raise an exception
    """
    if sys.platform == "win32":
        # instantiate a startupinfo obj:
        startupinfo = subprocess.STARTUPINFO()
        # set the use show window flag, might make conditional on being in Windows:
        startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    else:
        startupinfo = None

    cmd = [cmd]
    cmd.extend(args)
    display_debug("Running command '{}' in child process".format(" ".join(cmd)))
    proc = subprocess.Popen(cmd, shell=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            startupinfo=startupinfo)
    output, _ = proc.communicate()
    output = output.replace("\r\n", "\n")
    return output

# ------------------------------------------------------------------------------


def post_process(pandoc_rst, wiki_url, wiki_markup,
                 post_process_args):
    """Takes raw reST text produced by pandoc and applies several post-processing steps
    as described in the module documentation
    """
    post_processed_rst = pandoc_rst  # fix_underscores_in_ref_links(pandoc_rst)
    post_processed_rst, image_names = fix_image_links(post_processed_rst, wiki_markup,
                                                      post_process_args["rel_img_dir"])
    download_images_from_wiki(wiki_url, image_names, post_process_args["img_dir"])
    post_processed_rst = add_mantid_concept_links(post_processed_rst)
    if post_process_args["add_heading"]:
        post_processed_rst = add_page_heading(post_processed_rst,
                                              post_process_args["page_handle"])
    if post_process_args["add_page_handle"]:
        post_processed_rst = add_page_handle(post_processed_rst,
                                             post_process_args["page_handle"])
    post_processed_rst = fix_internal_links(post_processed_rst)
    return post_processed_rst

# ------------------------------------------------------------------------------


def add_page_heading(pandoc_rst, page_handle):
    """Add a heading at the top of the page - this is the page name of the wiki page"""
    _heading = re.findall('[A-Za-z][^A-Z  _]*', page_handle)
    page_heading = ' '.join(word for word in _heading)
    heading_markup = "=" * len(page_heading)
    pandoc_rst = heading_markup + "\n" + page_heading + "\n" + \
        heading_markup + "\n\n" + pandoc_rst
    return pandoc_rst

# ------------------------------------------------------------------------------


def add_page_handle(pandoc_rst, page_handle):
    """Add a handle on the top line of the page - this is the page name of the wiki page
    and should match references written by default in the conversion"""
    pandoc_rst = ".. _" + page_handle + ": \n \n" + pandoc_rst
    return pandoc_rst

# ------------------------------------------------------------------------------


def fix_internal_links(pandoc_rst):
    """Change the pandoc default internal link format to Sphinx-ready :ref: style"""
    link_strings = re.findall("`[a-zA-Z ]+?<[a-zA-Z _-]+?>`__+", pandoc_rst, re.DOTALL)
    print("Converting links: ", link_strings)
    for string in link_strings:
        ref_text = ":ref:" + string[:-2]
        pandoc_rst = re.sub(string, ref_text, pandoc_rst)
    return pandoc_rst

# ------------------------------------------------------------------------------


def fix_underscores_in_ref_links(pandoc_rst):
    """Defalt pandoc-translated references links seem to contain a backslash before
    the underscore of a link. This removes the backslash
    """
    return re.sub(r"\b\\\_", "_", pandoc_rst)

# ------------------------------------------------------------------------------


def fix_image_links(rst_text, wiki_markup, rel_img_dir):
    """Fix the image links to point to the correct relative image location
    It returns the processed text + the names of the required images from the wiki
    """
    mw_img_re = re.compile(r'\[\[(Image|File):(.+?)(\|.*?)?(\|.*?)?(\|.*?)?(\|.*?)?(\|.*?)?(\|.*?)?\]\]',
                           re.IGNORECASE)
    rst_img_re = re.compile(r'figure:: (([\w\-\_\\\/]+)\.(\w{2,5}))(.{0,50}\3){2}',
                            re.DOTALL)
    rst_sub_re = re.compile(r'(figure|image):: (([\w\-\_\\\/]+)\.(\w{2,5}))')

    img_link_dict = dict()
    # for all of the mediawiki links
    for match in re.finditer(mw_img_re, wiki_markup):
        display_info("Processing image link", match.group(0))
        img_link_dict[match.group(2)] = generate_rst_img_link(match, rel_img_dir)

    # for all of the rst figure links
    replacements = []
    for match in re.finditer(rst_img_re, rst_text):
        rst_link = img_link_dict[match.group(1)]
        replacements.append((match.start(), match.end(), match.group(1), rst_link))

    # perform replacements in reverse order
    for (start, end, _, rst_link) in reversed(replacements):
        rst_text = rst_text[0:start] + rst_link + rst_text[end:]

    replacements = []
    for match in re.finditer(rst_sub_re, rst_text):
        rst_link = img_link_dict[match.group(2)]
        rst_link = clean_inline_img_def(rst_link)
        replacements.append((match.start(), match.end(), match.group(2), rst_link))

    # perform replacements in reverse order
    for (start, end, _, rst_link) in reversed(replacements):
        rst_text = rst_text[0:start] + rst_link + rst_text[end:]

    return rst_text, img_link_dict.keys()

# ------------------------------------------------------------------------------


def generate_rst_img_link(match, rel_img_dir):
    link = "image:: " + rel_img_dir + "/" + match.group(2) + "\n"
    for i in range(3, len(match.groups())):
        if match.group(i) is None:
            break
        # strip off the first character as it is the | pipe
        img_opt = add_img_option(match.group(i)[1:])
        if img_opt is not None:
            link += "\t\t\t" + img_opt + "\n"
    return link

# ------------------------------------------------------------------------------


def add_img_option(mw_img_opt):
    mw_img_opt = mw_img_opt.strip()
    if len(mw_img_opt) > 0:
        if mw_img_opt.endswith("px"):
            return ":width: " + mw_img_opt
        elif mw_img_opt in ["right", "left", "middle", "centre"]:
            return ":align: " + mw_img_opt
        else:
            return ":alt: " + mw_img_opt
    else:
        return None

# ------------------------------------------------------------------------------


def clean_inline_img_def(rst_link):
    match = re.search(r'^\s+:align:\s+\w+\s*$', rst_link, re.MULTILINE)
    if match is not None:
        # take the align out
        rst_link = rst_link[0:match.start()] + rst_link[match.end()+1:]
        # then add it at the end as a comment
        rst_link += ".. FIX (inline definition does not allow align)" + match.group(0)
    return rst_link

# ------------------------------------------------------------------------------


def download_images_from_wiki(wiki_url, image_names, img_dir):
    for name in image_names:
        download_image_from_wiki(wiki_url, name, img_dir)

# ------------------------------------------------------------------------------


def download_image_from_wiki(wiki_url, name, img_dir):
    image_url = wiki_url.fileurl(name)
    try:
        response = urllib2.urlopen(image_url)
    except urllib2.URLError as err:
        raise RuntimeError("Failed to fetch image data: {}".format(str(err)))
    filename = os.path.join(img_dir, name)
    display_info("Downloading {0} to {1}".format(name, filename))
    with open(filename, 'w+b') as imagefile:
        imagefile.write(response.read())

    return None

# ------------------------------------------------------------------------------


def add_mantid_concept_links(post_processed_rst):
    """Adds the concept link for mantid things
    """
    concepts = ['EventWorkspace', 'MatrixWorkspace', 'PeaksWorkspace', 'MDWorkspace', 'Table Workspaces', 'WorkspaceGroup',
                'RectangularDetector', 'RAW File', 'Facilities File', 'FitConstraint', 'Framework Manager',
                'Instrument Data Service', 'InstrumentDefinitionFile', 'InstrumentParameterFile', 'Properties File',
                'MDHistoWorkspace', 'MDNorm', 'Nexus file', 'PeaksWorkspace', 'Point and space groups', 'RAW File',
                'Shared Pointer', 'Symmetry groups', 'Unit Factory', 'UserAlgorithms', 'Workflow Algorithm',
                'Workspace', 'Workspace2D', 'WorkspaceGroup']
    post_processed_rst = add_local_links(post_processed_rst, concepts, "")
    algorithms = mantid.AlgorithmFactory.getRegisteredAlgorithms(False).keys()
    if len(algorithms) > 0:
        post_processed_rst = add_local_links(post_processed_rst, algorithms, "algm-")
    fitfunctions = mantid.FunctionFactory.getFunctionNames()
    if len(fitfunctions) > 0:
        post_processed_rst = add_local_links(post_processed_rst, fitfunctions, "func-")

    return post_processed_rst


# ------------------------------------------------------------------------------

def add_local_links(rst_text, linkable_items, prefix):
    """Add links to items already within the documentation
    """
    # build regex string for simple words
    word_re = re.compile(r"[^`<]((\*{0,2})(\b" + r"\b|\b".join(linkable_items) + r"\b)\2)[^`_]")

    replacements = []
    for match in re.finditer(word_re, rst_text):
        link = ":ref:`" + match.group(3) + " <" + prefix + match.group(3) + ">`"
        replacements.append((match.start(1), match.end(1), match.group(1), link))

    # perform replacements in reverse order
    for (start, end, _, link) in reversed(replacements):
        rst_text = rst_text[0:start] + link + rst_text[end:]

    # build regex string for links
    link_re = re.compile(r"`(.+?)<(\b" + r"\b|\b".join(linkable_items) + r"\b)>`__")
    replacements = []
    for match in re.finditer(link_re, rst_text):
        link = ":ref:`" + match.group(1) + " <" + prefix + match.group(2) + ">`"
        replacements.append((match.start(), match.end(), match.group(0), link))

    # perform replacements in reverse order
    for (start, end, _, link) in reversed(replacements):
        rst_text = rst_text[0:start] + link + rst_text[end:]

    return rst_text


# ------------------------------------------------------------------------------

def main(argv):
    """Run the script as a program
    """
    args = parse_arguments(argv)
    try:
        wiki_url = WikiURL(args.index_url, args.pagename)
        display_info("Converting {} to reStructeredText..".format(wiki_url.pretty()))

        if args.output_file:
            output_dir = os.path.dirname(args.output_file)
            rel_img_dir = args.images_dir
            img_dir = os.path.normpath(os.path.join(output_dir, rel_img_dir))
        else:
            rel_img_dir, img_dir = None, None
        if not args.page_handle:
            page_handle = args.pagename
        else:
            page_handle = args.page_handle

        post_process_args = {"rel_img_dir": rel_img_dir, "img_dir": img_dir, "page_handle":
                             page_handle, "add_page_handle": args.add_page_handle,
                             "add_heading": args.add_heading}
        display_debug("Post processing arguments: '{}'".format(post_process_args))
        rst_text = to_rst(wiki_url, post_process_args)
        if args.output_file:
            open(args.output_file, 'w').write(rst_text + "\n")
        else:
            display_info(rst_text)
    except RuntimeError as exc:
        display_info(str(exc))
        return 1
    return 0

# ------------------------------------------------------------------------------


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
