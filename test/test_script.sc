#!/bin/zsh

# default paths
PROGPATH="./"
INPUTPATH="./test/"
COMPAREPATH="./originals/"
OUTPUTPATH=""
DIFFPATH=""

#default program name
PROGNAME=""

#default diff to on
DIFF_RESULTS = 1

# report a problem if there are no parameters
if [$# == 0] then 
     echo usage: test [-p] prog_path [-n] pattern [-i input_path] [-o output_path] [-c compare_path] [-d diff_path] -n
     exit 1
fi

# parse the arguments
FIRST_ARG = $1 # save the original value of the first arg in case we don't find -p
while ["$1" != ""]; do
    case $1 in
        -p | --program_name )   shift
                                PROGNAME="$1"
                                ;;
        -i | --input_path )     shift
        						INPUTPATH="$1"
                                ;;
        -o | --output_path )    shift
        						OUTPUTPATH="$1"
                                ;;
        -c | --compare_path )   shift
        						COMPAREPATH="$1"
                                ;;
        -d | --diff_path )      shift
        						DIFFPATH="$1"
                                ;;
        -n | --no_diff )        DIFF_RESULTS = 0
                                ;;
    esac
    shift
done

#if there was no -p, PROGNAME will still be empty, so try the first argument
if [PROGNAME == ""] then
	PROGNAME = $FIRST_ARG
fi
# and check again
if [PROGNAME == ""] then
    echo "Program name not found."
    exit 1
fi

# see if the program name has a path, if not, add the default
if [PROGNAME:t 

# check that the program exists at that location
if (! -f prog_path); then
    echo "Program file not found"
    exit 1
fi

# now we need to build the output and diff paths if they were not passed in
if [OUTPUTPATH == ""] then
    OUTPUTPATH = 


#if the outputpath is empty, make it with the same name as the input and a timestamp


#copy the input directory structure to the output
#rsync -rnv --max-size=0 INPUTPATH/ OUTPUTPATH
