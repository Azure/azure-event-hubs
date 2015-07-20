# Microsoft Azure IoT Client Libraries
# Copyright (c) Microsoft Corporation
# All rights reserved. 
# MIT License
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
# documentation files (the Software), to deal in the Software without restriction, including without limitation 
# the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, 
# and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED 
# TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
# CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
# IN THE SOFTWARE.

version.o: version.txt

#If we have a non-empty version string, then add it to the build as a C macro.
version.o: CFLAGS += $(if $(SEMANTIC_VERSION), -DEHC_VERSION=$(SEMANTIC_VERSION))

#Read the property $(SEMANTIC_VERSION) from a file.
semver: version.txt
	$(eval SEMANTIC_VERSION := $(shell cat version.txt))

#Make an effort to request the semantic version string from Git. Ignore errors.
#Write version string to a file so that the library will still report a version in
#case e.g. someone builds from a zipped archive rather than cloning our Git repo.
version.txt:
	$(eval SEMANTIC_VERSION := $(shell git describe --always --dirty))
	@[ -z $(SEMANTIC_VERSION) ] || echo "$(SEMANTIC_VERSION)" > $@

