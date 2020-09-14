#
# This script allows to build and install the Python version
# of the Imebra library.
#
# The Python version of Imebra is composed by the Imebra C++
# code and by Python wrappers generated with SWIG.
#
# To install Imebra:
#
#    cd imebra_folder
#    python setup.py install
#
# To install on a local folder (don't need administrator 
# permissions):
#
#    cd imebra_folder
#    python setup.py install --user
#
# To uninstall Imebra:
#
#    pip uninstall imebra
#
# Imebra is distributed under GPLv2 license.
# Commercial licenses are available at http://imebra.com
#
#-----------------------------------------------------------------

from distutils.core import setup, Extension
from distutils.sysconfig import get_python_inc
import sys
import os

# Get all the Imebra C++ source files
#------------------------------------
def get_sources(root_folder):
    modules_files = []

    for root, subdirs, files in os.walk(root_folder):
        for file in files:
            if file.endswith(".cpp") or file == "python_wrapper.cxx":
                modules_files.append(os.path.join(root, file))

    return modules_files


# Set proper libraries and flags according to the OS
#---------------------------------------------------
include_directories = []
include_directories.append(get_python_inc(plat_specific=1))
include_directories.append("./library/include")
librariesArray = []
compileFlags = []

print(include_directories)

if sys.platform.startswith('linux'):
    librariesArray.append('pthread')
    compileFlags = ["-std=c++11", "-Wall", "-Wextra", "-Wpedantic", "-Wconversion", "-Wfloat-equal"]

if sys.platform.startswith('darwin'):
    librariesArray.append('iconv')
    compileFlags = ["-std=c++11", "-Wall", "-Wextra", "-Wpedantic", "-Wconversion", "-Wfloat-equal"]

if sys.platform.startswith('win'):
    librariesArray.append('kernel32')

imebra_sources = get_sources("./library")
imebra_sources.extend(get_sources("./wrappers/pythonWrapper"))

# Define the C++ module
#----------------------
imebraModule = Extension('_imebra',
                    define_macros = [('MAJOR_VERSION', '5'),
                                     ('MINOR_VERSION', '2')],
                    libraries = librariesArray,
                    sources = imebra_sources,
                    extra_compile_args = compileFlags,
                    include_dirs= include_directories)

# Define the package
#-------------------
setup (name = 'imebra',
       version = '5.2',
       description = 'Imebra library for Python',
       author = 'Paolo Brandoli',
       author_email = 'paolo@binarno.com',
       url = 'https://imebra.com',
       long_description = '''
Imebra Dicom Library.
''',
       ext_modules = [imebraModule],
       packages = ['imebra'],
       package_dir = {'imebra': './wrappers/pythonWrapper'})

