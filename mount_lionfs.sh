# lionfs, The Link Over Network File System
# Copyright (C) 2016  Ricardo Biehl Pasquali <rbpoficial@gmail.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

script_name="$0"

function print_usage {
	echo "usage: $script_name mount_point [OPTIONS]"
	exit 1
}

function print_help {
	echo "$script_name -- Mount util for lionfs"
	echo
	echo "    --help|-h  Show this."
	echo "    --fuse-version  Print fuse version."
	echo "    --fuse-help  Print fuse-help."
	echo "    --debug|-d  Active debug mode."
	echo
	echo "License: GNU/GPL (See COPYING); Author: Ricardo Biehl Pasquali"
	exit 0
}

if [ -z "$1" ]; then
	echo "No mount_point!"
	print_usage
fi

if [ ! -d "$1" ]; then
	case "$1" in
	"--fuse-help")
		exec ./lionfs -h
	;;
	"--fuse-version")
		exec ./lionfs -V
	;;
	"--help"|"-h")
		print_help
	;;
	*)
		echo "\"$1\" is not a directory!"
		print_usage
	esac

	exit 0
fi

mount_point="$1"

# Options used by default:
# `-s` = single-thread operation.
# `-o fsname` = Name of filesystem.
# `-o use_ino` = Allow filesystem set its own inode numbers.
default_opt="-s -o fsname=lionfs -o use_ino"

unset opt_arg

while [ 0 ]; do
	if [ -z "$2" ]; then
		break
	fi

	case "$2" in
	"-d"|"--debug")
		opt_arg="$opt_arg -d"
	;;
	esac

	shift
done

./lionfs $mount_point $opt_arg $default_opt
