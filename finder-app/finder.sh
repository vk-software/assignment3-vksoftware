#!/bin/sh

filesdir=$1
searchstr=$2

# Validate if passed arguments are not empty

if [ -z "$filesdir" ] || [ -z "$searchstr" ]
then
	echo "Passed arguments cannot be empty"
        exit 1
fi


# Validate if the directory with files exists

if [ ! -d "$filesdir" ]
then
        echo "$filesdir doesn't exist!"
	exit 1
fi

number_of_files=$(eval "grep -rlI $searchstr $filesdir | wc -l")
number_of_lines=$(eval "grep -roI $searchstr $filesdir | wc -l")

echo "The number of files are $number_of_files and the number of matching lines are $number_of_lines"
