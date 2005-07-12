#! /bin/bash
#
# TALPA test script
#
# Copyright (C) 2004 Sophos Plc, Oxford, England.
#
# This program is free software; you can redistribute it and/or modify it under the terms of the
# GNU General Public License Version 2 as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with this program; if not,
# write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
#

. ${srcdir}/functions.sh

${srcdir}/talpa-init.sh
get_mount_fs /tmp
echo /tmp/tlp-test/ >/proc/sys/talpa/intercept-filters/FilesystemInclusionProcessor/include-path
echo -${_mount_fs} >/proc/sys/talpa/intercept-filters/FilesystemExclusionProcessor/fstypes
echo +fs:${_mount_fs}:1 >/proc/sys/talpa/intercept-filters/VettingController/routing
./tlp-2-063 1 /tmp/tlp-test/file
res=$?

exit $res
