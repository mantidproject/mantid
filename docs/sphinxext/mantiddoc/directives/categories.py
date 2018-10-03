# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
    Provides directives for dealing with category pages.

    While parsing the directives a list of the categories and associated pages/subcategories
    is tracked. When the final set of html pages is collected, a processing function
    creates "index" pages that lists the contents of each category. The display of each
    "index" page is controlled by a jinja2 template.
"""
from __future__ import (absolute_import, division, print_function)
from mantiddoc.directives.base import AlgorithmBaseDirective, algorithm_name_and_version #pylint: disable=unused-import
from sphinx.util.osutil import relative_uri
import os
import posixpath
from six import iteritems, itervalues

CATEGORY_PAGE_TEMPLATE = "category.html"
# relative to the directory containing the source file
CATEGORIES_DIR = "categories"

# List of category names that are considered the index for everything in that type
# When this category is encountered an additional index.html is written to both the
# directory of the document and the category directory
INDEX_CATEGORIES = ["Algorithms", "FitFunctions", "Concepts", "Techniques", "Interfaces"]

class LinkItem(object):
    """
    Defines a linkable item with a name and html reference
    """
    # Name displayed on listing page
    name = None
    # location of item relative to source
    location = None

    def __init__(self, name, location):
        """
        Arguments:
          name (str): Display name of document
          location (str): Location of item relative to source directory
        """
        self.name = str(name).replace("\\\\","/")
        self.location = location

    def __eq__(self, other):
        """
        Define comparison for two objects as the comparison of their names

        Arguments:
          other (PageRef): Another PageRef object to compare
        """
        return self.name == other.name

    def __hash__(self):
        return hash(self.name)

    def __repr__(self):
        return self.name

    def link(self, base, ext=".html"):
        """
        Returns a link for use as a href to refer to this document from a
        categories page. It assumes that the category pages are in a subdirectory
        of the root and that the item to be referenced is in the algorithms directory
        under the root.

        Arguments:
          base (str): The path to the referrer
          ext (str):  The extension to use

        Returns:
          str: A string containing the link to reach this item
        """
        link = relative_uri(base=to_unix_style_path(base), to=self.location)
        if not link.endswith(ext):
            link += ext
        return link

# endclass

class PageRef(LinkItem):
    """
    Store details of a single page reference
    """

    def __init__(self, name, location):
        super(PageRef, self).__init__(name, location)

#endclass

class Category(LinkItem):
    """
    Store information about a single category
    """
    # Collection of PageRef objects that link to members of the category
    pages = None
    # Collection of PageRef objects that form subcategories of this category
    subcategories = None
    # Relative path for the final html to be written. \ separators are converted to /
    html_path = None

    def __init__(self, name, docname):
        """
        Create a named category that is referenced from the given document.

        Arguments:
          name (str): The name of the category
          docname (str): Relative path to document from root directory
        """
        docname = to_unix_style_path(docname)

        dirpath, filename = posixpath.split(docname)
        html_dir = dirpath + "/" + CATEGORIES_DIR
        self.html_path = html_dir + "/" + to_unix_style_path(name) + ".html"
        super(Category, self).__init__(name, self.html_path)
        self.pages = set([])
        self.subcategories = set([])


#endclass

class CategoriesDirective(AlgorithmBaseDirective):
    """
    Records the page as part of given categories. Index pages for each
    category are then automatically created after all pages are collected
    together.

    Subcategories can be given using the "\\" separator, e.g. Algorithms\\Transforms

    If the argument list is empty then the the file is assumed to be an algorithm file
    and the lists is pulled from the algorithm object
    """

    required_arguments = 0
    # it can be in many categories and we put an arbitrary upper limit here
    optional_arguments = 25

    def execute(self):
        """
        Called by Sphinx when the defined directive is encountered.
        """
        categories = self._get_categories_list()
        display_name = self._get_display_name()
        links = self._create_links_and_track(display_name, categories)

        self.add_rst("\n" + links)
        return []

    def skip(self):
        """
        Return error message if the directive should be skipped.
        If there are no arguments, it calls the base class skip() method
        else it returns and empty string.

        Returns:
          str: Return error if directive to be skipped
        """
        args = self.arguments
        if len(args) == 0:
            return super(CategoriesDirective, self).skip()
        else:
            return ""

    def _get_categories_list(self):
        """
        Returns a list of the category strings

        Returns:
          list: A list of strings containing the required categories
        """
        # if the argument list is empty then assume this is in an algorithm file
        args = self.arguments
        if len(args) > 0:
            return args
        else:
            if self.algorithm_version() is not None:
                return self._get_algorithm_categories_list()
            else:
                return self._get_ifunction_categories_list()

    def _get_algorithm_categories_list(self):
        """
        Returns a list of the category strings

        Returns:
          list: A list of strings containing the required categories
        """
        category_list = ["Algorithms"]
        alg_cats = self.create_mantid_algorithm(self.algorithm_name(), self.algorithm_version()).categories()
        for cat in alg_cats:
            # double up the category separators so they are not treated as escape characters
            category_list.append(cat.replace("\\", "\\\\"))

        return category_list

    def _get_ifunction_categories_list(self):
        """
        Returns a list of the category strings

        Returns:
          list: A list of strings containing the required categories
        """
        category_list = ["FitFunctions"]
        func_cats = self.create_mantid_ifunction(self.algorithm_name()).categories()
        for cat in func_cats:
            # double up the category separators so they are not treated as escape characters
            category_list.append(cat.replace("\\", "\\\\"))

        return category_list

    def _get_display_name(self):
        """
        Returns the name of the item as it should appear in the category
        """
        # If there are no arguments then take the name directly from the document name, else
        # assume it is an algorithm and use its name
        if len(self.arguments) > 0:
            env = self.state.document.settings.env
            # env.docname returns relative path from doc root. Use name after last "/" separator
            return env.docname.split("/")[-1]
        else:
            return self.algorithm_name()

    def _create_links_and_track(self, page_name, category_list):
        """
        Return the reST text required to link to the given
        categories. As the categories are parsed they are
        stored within the current environment for use in the
        "html_collect_pages" function.

        Args:
          page_name (str): Name to use to refer to this page on the category index page
          category_list (list): List of category strings

        Returns:
          str: A string of reST that will define the links
        """
        env = self.state.document.settings.env
        if not hasattr(env, "categories"):
            env.categories = {}

        link_rst = ""
        ncategs = 0
        for categ_name in category_list:
            #categ_name is the full category name - register that
            category = self.register_category(categ_name, env)
            category.pages.add(PageRef(page_name, env.docname))

            #now step up a step up each time the category hierarchy
            parent_category = categ_name
            while True:
                if r"\\" in parent_category:
                    categs = parent_category.split(r"\\")
                else:
                    break
                # remove the last item
                subcat = Category(categ_name, env.docname) #create the category with the full name
                subcat.name=categs.pop() # and then replace it with the last token of the name
                parent_category = r"\\".join(categs)

                #register the parent category
                parent = self.register_category(parent_category, env)
                parent.subcategories.add(subcat)

            # endwhile

            #category should be the last subcategory by this point
            link_rst += "`%s <%s>`_ | " % (categ_name, category.link(env.docname))
            ncategs += 1
        # endfor

        link_rst = "**%s**: " + link_rst.rstrip(" | ") # remove final separator
        if ncategs == 1:
            link_rst = link_rst % "Category"
        else:
            link_rst = link_rst % "Categories"
        #endif

        return link_rst
    #end def

    def register_category(self, categ_name, env):
        category = Category(categ_name, env.docname)
        if categ_name not in env.categories:
            category = Category(categ_name, env.docname)
            env.categories[categ_name] = category
        else:
            category = env.categories[categ_name]
        return category


#---------------------------------------------------------------------------------

def to_unix_style_path(path):
    """
    Replaces any backslashes in the given string with forward slashes
    and replace consecutive forward slashes with a single forward slash.

    Arguments:
      path: A string possibly containing backslashes
    """
    return path.replace("\\", "/").replace("//", "/")

#---------------------------------------------------------------------------------

def html_collect_pages(app):
    """
    Callback for the 'html-collect-pages' Sphinx event. Adds category
    pages + a global Categories.html page that lists the pages included.

    Function returns an iterable (pagename, context, html_template),
    where context is a dictionary defining the content that will fill the template

    Arguments:
      app: A Sphinx application object
    """
    if not hasattr(app.builder.env, "categories"):
        return # nothing to do

    for name, context, template in create_category_pages(app):
        yield (name, context, template)
# enddef

def create_category_pages(app):
    """
    Returns an iterable of (category_name, context, "category.html")

    Arguments:
      app: A Sphinx application object
    """
    env = app.builder.env
    # jinja2 html template
    template = CATEGORY_PAGE_TEMPLATE

    categories = env.categories
    for name, category in iteritems(categories):
        context = {}
        # First write out the named page
        context["title"] = category.name

        #get parent category
        if "\\" in category.name:
            categs = category.name.split("\\")
            categs.pop()
            parent_category = r"\\".join(categs)
            parent_category_link = "../" + categs[-1] + ".html"
            parent_category = "<b>Category:</b> <a href='{0}'>{1}</a>"\
                .format(parent_category_link,parent_category)
            context["parentcategory"] = parent_category

        # sort subcategories & pages alphabetically
        context["subcategories"] = sorted(category.subcategories, key = lambda x: x.name)
        context["pages"] = sorted(category.pages, key = lambda x: x.name)
        context["outpath"] = category.html_path

        #jinja appends .html to output name
        category_html_path_noext = posixpath.splitext(category.html_path)[0]
        yield (category_html_path_noext, context, template)

        # Now any additional index pages if required
        if category.name in INDEX_CATEGORIES:
            # index in categories directory
            category_html_dir = posixpath.join(category.name.lower(), 'categories')
            category_html_path_noext = posixpath.join(category_html_dir, 'index')
            yield (category_html_path_noext, context, template)

            # index in document directory
            document_dir = posixpath.dirname(category_html_dir)
            category_html_path_noext = posixpath.join(document_dir, 'index')
            context['outpath'] = category_html_path_noext + '.html'
            yield (category_html_path_noext, context, template)
# enddef

#-----------------------------------------------------------------------------------------------------------

def purge_categories(app, env, docname):
    """
    Purge information about the given document name from the tracked algorithms

    Arguments:
      app (Sphinx.application): Application object
      env (Sphinx.BuildEnvironment):
      docname (str): Name of the document
    """
    if not hasattr(env, "categories"):
        return # nothing to do

    categories = env.categories
    try:
        name, version = algorithm_name_and_version(docname)
    except RuntimeError: # not an algorithm page
        return

    deadref = PageRef(name, docname)
    for category in itervalues(categories):
        pages = category.pages
        if deadref in pages:
            pages.remove(deadref)

#------------------------------------------------------------------------------
def setup(app):
    # Add categories directive
    app.add_directive('categories', CategoriesDirective)

    # connect event html collection to handler
    app.connect("html-collect-pages", html_collect_pages)

    # connect document clean up to purge information about this page
    app.connect('env-purge-doc', purge_categories)
