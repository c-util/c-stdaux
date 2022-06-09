#
# Sphinx Documentation Configuration
#

import re
import os
import sys

import capidocs.kerneldoc
import hawkmoth

# Global Setup

project = 'c-stdaux'

author = 'C-Util Community'
copyright = '2022, C-Util Community'

# Hawkmoth C-Audodoc Setup

capidocs.kerneldoc.hawkmoth_conf()

# Extensions

exclude_patterns = []

extensions = [
    'hawkmoth',
]

# Hawkmoth Options

def kerneldoc_transform(comment):
    """Custom kernel-doc conversion to reStructuredText"""

    #
    # Convert section docs:
    #

    # Insert section header
    comment = re.sub(
        r"(?m)\A([ \t]*)DOC: (.+)$",
        "\\2\n" + "-"*120 + "\n",
        comment,
    )
    # Insert section without header
    comment = re.sub(
        r"(?m)\A([ \t]*)DOC:$",
        "",
        comment,
    )

    #
    # Convert functions:
    #

    # Strip entity-name from synopsis.
    comment = re.sub(
        r"(?m)\A([ \t]*)([^-]+?) - ",
        "",
        comment,
    )
    # Convert parameter descriptions.
    comment = re.sub(
        r"(?m)^([ \t]*)@([a-zA-Z0-9_]+|\.\.\.):",
        "\n\\1:param \\2:",
        comment,
    )
    # Convert return-value section.
    comment = re.sub(
        r"(?m)^([ \t]*)([Rr]eturns?):",
        "\n\\1:return:",
        comment,
    )

    return comment

cautodoc_clang = capidocs.kerneldoc.hawkmoth_include_args()

cautodoc_root = os.path.abspath('..')

cautodoc_transformations = {
#    'kerneldoc': capidocs.kerneldoc.hawkmoth_converter,
    'kerneldoc': kerneldoc_transform,
}

# HTML Options

html_theme = 'sphinx_rtd_theme'
