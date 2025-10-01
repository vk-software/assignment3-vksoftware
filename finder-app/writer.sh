#!/bin/sh

writefile=$1
writestr=$2

# Validate if passed arguments are not empty

if [ -z "$writefile" ] || [ -z "$writestr" ]
then
	echo "Passed arguments cannot be empty"
        exit 1
fi

if [ ! -d $(dirname "$writefile") ]
then
    mkdir -p $(dirname "$writefile")
fi

echo "$writestr" > "$writefile"


# Validate if the the files was created

if [ ! -f "$writefile" ]
then
        echo "Failed to create $writefile!"
	exit 1
fi

