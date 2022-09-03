#!/bin/bash
# -*- c-basic-offset: 4; indent-tabs-mode: nil -*-

## Parse command-line arguments.
while [ $# -gt 0 ] ; do
    arg="$1" ; shift
    case "$arg" in
        (--prefix=*)
            prefix="${arg#--prefix=}"
            ;;
        (*)
            if [ -z "$major_file" ] ; then
                major_file="$arg"
            elif [ -z "$minor_file" ] ; then
                minor_file="$arg"
            elif [ -z "$patch_file" ] ; then
                patch_file="$arg"
            fi
            ;;
    esac
done

## Get the most recent tag as the release, and strip off any prefix.
release="$(git describe 2> /dev/null)"
release="${release#"$prefix"}"

## Extract the number of commits since the tag, and the latest commit.
if [[ "$release" =~ ^(.*)-([0-9]+)-g([0-9a-f]+)$ ]] ; then
    release="${BASH_REMATCH[1]}"
    gitadv="${BASH_REMATCH[2]}"
    gitrev="${BASH_REMATCH[3]}"
fi

## Detect untracked files or uncommitted changes.
if git diff-index --quiet HEAD 2> /dev/null ; then
    unset alpha
else
    ## There's something to commit.
    alpha='a'
fi

unset longext
if [[ "$release" =~ ^([0-9]+)[^0-9\.]*\.([0-9]+)[^0-9\.]*\.([0-9]+) ]] ; then
    if [ -n "$major_file" -a -r "$major_file" ] ; then
        longext="$alpha"
        release="$((BASH_REMATCH[1] + 1)).0.0"
    elif [ -n "$minor_file" -a -r "$minor_file" ] ; then
        longext="$alpha"
        release="${BASH_REMATCH[1]}.$((BASH_REMATCH[2] + 1)).0"
    elif [ -n "$patch_file" -a -r "$patch_file" ] ; then
        longext="$alpha"
        release="${BASH_REMATCH[1]}.${BASH_REMATCH[2]}.$((BASH_REMATCH[3] + 1))"
    elif [ -n "$gitadv" ] ; then
        longext=".$((gitadv + 1))$alpha"
        release="${BASH_REMATCH[1]}.${BASH_REMATCH[2]}.${BASH_REMATCH[3]}"
    else
        longext=".1$alpha"
        release="${BASH_REMATCH[1]}.${BASH_REMATCH[2]}.${BASH_REMATCH[3]}"
    fi
fi

longrelease="${release}$longext${gitrev:+-g"$gitrev"}"


printf '%s %s\n' "$release" "$longrelease"
