# -*- coding: utf-8 -*-
# All configuration values have a default; values that are commented out
# serve to show the default.
#
# Options here are applied and assumed to be those required when the builder is set the "html".
# For common options see conf.py.in. This file is presumed to be "execfiled" by the main conf.py.in

# -- Override options for non-embedded standalone HTML output --------------------------------------------

# Theme-specific options to customize the look and feel of a theme.
# We config the bootstrap settings here, and apply CSS changes in
# custom.css rather than here.
html_theme_options = {
    # Navigation bar title.
    'navbar_title': " ", # deliberate single space so it's not visible
    # Tab name for entire site.
    'navbar_site_name': "Mantid",
    # Add links to the nav bar. Third param of tuple is true to create absolute url.
    'navbar_links': [
        ("Home", "http://www.mantidproject.org", True),
        ("Download", "http://download.mantidproject.org", True),
        ("Documentation", "http://www.mantidproject.org/Documentation", True),
        ("Contact Us", "http://www.mantidproject.org/Contact", True),
    ],
    # Do not show the "Show source" button.
    'source_link_position': "no",
    # Remove the local TOC from the nav bar
    'navbar_pagenav': False,
    # Hide the next/previous in the nav bar.
    'navbar_sidebarrel': False,
    # Use the latest version.
    'bootstrap_version': "3",
    # Ensure the nav bar always stays on top of page.
    'navbar_fixed_top': "false",
}
