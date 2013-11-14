#!/bin/bash
# This script is sourced by Makefiles to provide various utility functions

function igt_result_directory()
{
	suite=$1
	[ -z "$suite" ] && suite=results

	date=`date +%Y%m%d`
	base=$date-piglit-$suite

	n=1
	dir=$base.$n
	while [ -e "$dir" ]; do
		let n+=1
		dir=$base.$n
	done

	echo $dir
}
