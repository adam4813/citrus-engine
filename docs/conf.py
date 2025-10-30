# Configuration file for Sphinx documentation builder
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import os
import subprocess

# -- Project information -----------------------------------------------------
project = 'Citrus Engine'
copyright = '2024, Citrus Engine Contributors'
author = 'Citrus Engine Contributors'
release = '0.0.1'
version = '0.0.1'

# -- General configuration ---------------------------------------------------
extensions = [
    'breathe',
    'sphinx.ext.autodoc',
    'sphinx.ext.intersphinx',
    'sphinx.ext.todo',
    'sphinx.ext.viewcode',
]

templates_path = ['_templates']
exclude_patterns = ['_build', '_doxygen', 'Thumbs.db', '.DS_Store']

# -- Options for HTML output -------------------------------------------------
html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']
html_logo = '../assets/branding/citrus-engine-logo.svg'
html_favicon = '../assets/branding/citrus-engine-logo.svg'

html_theme_options = {
    'logo_only': False,
    'prev_next_buttons_location': 'bottom',
    'style_external_links': False,
    'style_nav_header_background': '#ff8c00',
    'collapse_navigation': False,
    'sticky_navigation': True,
    'navigation_depth': 4,
    'includehidden': True,
    'titles_only': False
}

# -- Breathe configuration (Doxygen integration) ----------------------------
breathe_projects = {
    "CitrusEngine": "_doxygen/xml"
}
breathe_default_project = "CitrusEngine"

# -- Doxygen build -----------------------------------------------------------
def run_doxygen(app):
    """Run Doxygen to generate XML for Breathe"""
    doxygen_config = os.path.join(app.srcdir, '..', 'Doxyfile')
    if os.path.exists(doxygen_config):
        print("Running Doxygen...")
        # Run Doxygen from the docs directory so paths work correctly
        result = subprocess.run(['doxygen', doxygen_config], 
                              cwd=app.srcdir,
                              capture_output=True,
                              text=True)
        if result.returncode != 0:
            print(f"Doxygen warning/error output:\n{result.stderr}")
        print("Doxygen completed")
    else:
        print(f"Warning: Doxyfile not found at {doxygen_config}")

def setup(app):
    app.connect('builder-inited', run_doxygen)
