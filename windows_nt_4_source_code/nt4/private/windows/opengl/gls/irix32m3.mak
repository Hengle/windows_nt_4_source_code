# Copyright 1995-2095, Silicon Graphics, Inc.
# All Rights Reserved.
# 
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
# the contents of this file may not be disclosed to third parties, copied or
# duplicated in any form, in whole or in part, without the prior written
# permission of Silicon Graphics, Inc.
# 
# RESTRICTED RIGHTS LEGEND:
# Use, duplication or disclosure by the Government is subject to restrictions
# as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
# and Computer Software clause at DFARS 252.227-7013, and/or in similar or
# successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
# rights reserved under the Copyright Laws of the United States.

MAKEDEFS = \
    ABI="-n32 -mips3" \
    SYSLIBS="-L $(ROOT)/usr/lib32/mips3 -L $(ROOT)/usr/lib32" \
    WARNFLAGS="-woff 1048,1429" \
    $(NULL)

MAKEFILE = irix.mak
NULL =

default clean:
	cd inc; $(MAKE) -f $(MAKEFILE) -$(MAKEFLAGS) $@
	cd lib; $(MAKE) -f $(MAKEFILE) -$(MAKEFLAGS) $(MAKEDEFS) $@
	cd cmd; $(MAKE) -f $(MAKEFILE) -$(MAKEFLAGS) $(MAKEDEFS) $@
	cd test; $(MAKE) -f $(MAKEFILE) -$(MAKEFLAGS) $(MAKEDEFS) $@
