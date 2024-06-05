#!/bin/bash

set -Eeo pipefail
#set -o xtrace
shopt -s nullglob

all=all

delim=$'\n'

IFS=$'\x1f'

usage="$(cat <<USAGE
Tag files using an all directory and symlinks. A symlink into the '$all'
directory is an effective tag counting towards the name of the symlink.

To get started, create the '$all' directory, if the script does not find this in
the current directory, it will look through all parent directories.

This program works by keeping an associative array where the keys are
files and the values of each key are the tags, this array is empty by default.

This associative array will be called 'table'.

usage:
    $0 [flags...]

flags:
    -h|--help                Show this help
    -d <delimiter>           Set the delimiter for printing
    -fo|--filter-out <tags>  Filter files from the table that have all tags
    -f|--files <files>       Add files to the table
    -a|--add <tags>          Add given tags to all files in the table
    -rm|--remove <tags>      Remove given tags from all files in the table
    -w|--write               Synchronize the table with the file system
    -p|--print               Print all files in the table
    -pt|--print-tags         Print all files in the table and their tags

-p and -pt use the given delimiter (default is $'\\n') to separate files

By default, the program will always print the difference of the table
and the file system at the program end. Nothing will be printed when nothing
has changed.
USAGE
)"

while [ ! "$(pwd)" = "/" ] && [ ! -d "$all" ] ; do
    builtin cd ..
done

if [ "$(pwd)" = "/" ] ; then
    echo "$usage"
    exit 1
fi

# Retrieves all tags of a given name
# $1: name
# tags: tags of name
# returns whether name exists
get_tags() {
    tags=()
    [ -f "$all/$1" ] || return 1
    for t in *"/$1" ; do
        tags+=("${t%/*}")
    done
}

# Get the difference of the tags in the files array and the tags on the file system
# $1: name
# add_tags: added tags
# rem_tags: removed tags
get_diff() {
    add_tags=()
    rem_tags=()
    get_tags "$f"
    read -ra now <<< "${files[$f]}"

    for t1 in "${now[@]}" ; do
        found=false
        for t2 in "${tags[@]}" ; do
            if [ "$t1" = "$t2" ] ; then
                found=true
                break
            fi
        done
        if ! $found ; then
            add_tags+=("$t1")
        fi
    done

    for t1 in "${tags[@]}" ; do
        found=false
        for t2 in "${now[@]}" ; do
            if [ "$t1" = "$t2" ] ; then
                found=true
                break
            fi
        done
        if ! $found ; then
            rem_tags+=("$t1")
        fi
    done
}

declare -A files

print_diff() {
    for f in "${!files[@]}" ; do
        get_diff "$f"
        [ ${#add_tags[@]} -eq 0 ] && [ ${#rem_tags[@]} -eq 0 ] && continue
        echo -n "$f : "
        for t in "${rem_tags[@]}" ; do
            echo -n "-$t "
        done

        for t in "${add_tags[@]}" ; do
            echo -n "+$t "
        done

        echo
    done
}

print_files() {
    IFS="$delim"
    echo -ne "${!files[*]}"
    IFS=$'\x1f'
}

print_files_tags() {
    for f in "${!files[@]}" ; do
        get_tags "$f"
        echo -n "$f : ${tags[0]}"
        for t in "${tags[@]:1}" ; do
            echo -n ", $t"
        done
        echo -n "$delim"
    done
}

# Synchronise files array with file system
sync_files() {
    for f in "${!files[@]}" ; do
        get_diff "$f"
        for t in "${add_tags[@]}" ; do
            mkdir -p "$t"
            ln -s "../all/$f" "$t/$f"
        done
        for t in "${rem_tags[@]}" ; do
            rm "$t$f"
        done
    done
}

# Add given files to the files array
# $@: files
add_files() {
    for f in "$@" ; do
        [[ -v files["$f"] ]] && continue
        get_tags "$f"
        files["$f"]="${tags[*]}"
    done
}

# Add all files matching all given tags to the files array
# $@: tags
list_files() {
    tag="$1/"
    for f in "$tag"* ; do
        file="${f:${#tag}}"
        [[ -v files["$file"] ]] && continue
        get_tags "$file"
        for t1 in "${@:2}" ; do
            contains=false
            for t2 in "${tags[@]}" ; do
                if [ "$t1" = "$t2" ] ; then
                    contains=true
                    break
                fi
            done
            ! $contains && continue 2
        done
        files["$file"]="${tags[*]}"
    done
}

# Remove entries from files array that have all of the given tags
# $@: tags
filter_out_files() {
    for f in "${!files[@]}" ; do
        match=true
        for t in "$@" ; do
            [ -L "$t/$f" ] || match=false
        done
        ! $match && unset 'files["$f"]' || true
    done
}

# $@: tags
add_tags() {
    local tags
    for f in "${!files[@]}" ; do
        read -ra tags <<< "${files[$f]}"
        for t in "$@" ; do
            tags+=("$t")
        done
        files["$f"]="${tags[*]}"
    done
}

# $@: tags
remove_tags() {
    local tags
    local new_tags
    for f in "${!files[@]}" ; do
        read -ra tags <<< "${files[$f]}"
        new_tags=()
        for t1 in "${tags[@]}" ; do
            found=false
            for t2 in "$@" ; do
                if [ "$t1" = "$t2" ] ; then
                    found=true
                    break
                fi
            done
            if ! $found ; then
                new_tags+=("$t1")
            fi
        done
        files["$f"]="${new_tags[*]}"
    done
}

if [ $# -eq 0 ] ; then
    echo "nothing do to..."
    echo "use '$0 -h' for more information"
    exit 0
fi

while [ $# -ne 0 ] ; do
    case "$1" in
    -*)
        action="$1"
        shift
        args=()
        while [ $# -ne 0 ] ; do
            case "$1" in
            -*)
                break
                ;;
            *)
                args+=("$1")
                shift
                ;;
            esac
        done
        case "$action" in
        -h|--help)
            echo "$usage"
            ;;
        -d|--delimiter)
            [ "${#args[@]}" -eq 1 ]
            delim="${args[0]}"
            ;;
        -l|--list|--ls)
            list_files "${args[@]}"
            ;;
        -fo|--filter-out)
            filter_files "${args[@]}"
            ;;
        -f|--files)
            add_files "${args[@]}"
            ;;
        -a|--add)
            add_tags "${args[@]}"
            ;;
        -rm|--remove)
            remove_tags "${args[@]}"
            ;;
        -w|--write)
            print_diff
            sync_files
            ;;
        -p|--print)
            print_files
            ;;
        -pt|--print-tags)
            print_files_tags
            ;;
        esac
        ;;
    *)
        exit 1
        ;;
    esac
done

print_diff
