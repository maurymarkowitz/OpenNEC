#!/bin/zsh

# default paths
PROGPATH="./"
INPUTPATH="./tests/"
COMPAREPATH="./originals/"
OUTPUTPATH=""
DIFFPATH=""

#default program name
PROGNAME=""

#default diff to on
DIFF_RESULTS=1

# report a problem if there are no parameters
if ((# == 0)); then
     echo "usage: test [-p] prog_path [-n] pattern [-i input_path] [-o output_path] [-c compare_path] [-d diff_path]"
     exit 1
fi

# parse the arguments,
FIRST_ARG=$1 # save the original value of the first arg in case we don't find -p
while [ "$1" != "" ]; do
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
        -n | --no_diff )        DIFF_RESULTS=0
                                ;;
    esac
    shift
done

#if there was no -p, PROGNAME will still be empty, so try the first argument
if [[ -z $PROGNAME ]]; then
    PROGNAME=$FIRST_ARG
fi

# and check again
if [[ -z $PROGNAME ]]; then
    echo "Program name not found in arguments"
    exit 1
fi

# see if the program name has a path, if not, add the default
if [[ $PROGNAME:t == $PROGNAME ]]; then # :t means "just the name and extension"
    PROGNAME=$PROGPATH$PROGNAME
fi

# check that the program exists at that location
if [[ ! -f $PROGNAME ]]; then
    echo "Program file not found at "$PROGNAME
    exit 1
fi

# and that is actually is a program
if [[ ! -x $PROGNAME ]]; then
    echo "File is not executable at "$PROGNAME
    exit 1
fi

# check that the input directory exists
if [[ ! -d $INPUTPATH ]]; then
    echo "Input path does not exist at "$INPUTPATH
    exit 1
fi

# now we need to build the output and diff paths if they were not passed in
if [[ -z $OUTPUTPATH ]]; then
    OUTPUTPATH=${INPUTPATH%/}"_"$(date '+%d%m%Y%H%M%S')
fi
if [[ -z $DIFFPATH ]]; then
    DIFFPATH=$OUTPUTPATH
fi

# now make sure there are trailing slashes on the directories
[[ "${INPUTPATH}" != */ ]] && INPUTPATH="${INPUTPATH}/"
[[ "${OUTPUTPATH}" != */ ]] && OUTPUTPATH="${OUTPUTPATH}/"
[[ "${DIFFPATH}" != */ ]] && DIFFPATH="${DIFFPATH}/"

# see if the output directory exists, if not, build it based on the input structure
if [[ ! -d $OUTPUTPATH ]]; then
    rsync -r -v -f"+ */" -f"- *" $INPUTPATH $OUTPUTPATH
fi

# check again that it exists to be safe
if [[ ! -d $OUTPUTPATH ]]; then
    echo "Output path could not be created at "$OUTPUTPATH
    exit 1
fi

# now the core of the script, iterate over the files in the input
# and pass each one into the program to be tested
find $INPUTPATH -exec $PROGNAME {} >> $OUTPUTPATH{} \;

# when that is done, optionally diff them with the originals
if [[ $DIFF_RESULTS -eq 1 ]]; then
  find $OUTPUTPATH -exec diff -rub ${} $COMPAREPATH \;
fi
