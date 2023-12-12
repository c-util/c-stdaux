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
    'hawkmoth.ext.transformations',
]

# Hawkmoth Options

hawkmoth_clang = capidocs.kerneldoc.hawkmoth_include_args()
hawkmoth_clang += ["-I" + os.path.abspath("..")]
hawkmoth_clang += ["-DC_COMPILER_DOCS"]

hawkmoth_root = os.path.abspath('..')

cautodoc_transformations = {
    'kerneldoc': capidocs.kerneldoc.hawkmoth_converter,
}

# HTML Options

html_theme = 'sphinx_rtd_theme'
