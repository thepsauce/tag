# tag
file tagging system

## specification

The following specification is used for the tag script:
All files that are allowed to be tagged shall be in a directory called
`$TAG_DEFAULT_DIR` (the default of that is `all`). Tags are stored as symlinks
within neighboring directories, you should never change `TAG_DEFAULT_DIR` after
you have chosen a name for it (or redo all symlinks).

## installation (Linux)

Put the tag script somewhere on your path for convenience. Create a dedicated
directory for the tag directories (named `$TAG_DEFAULT_DIR`) and put some files
into there. Now call the script for the first time within that directory:
`tag -h`
If there is no error message at the beginning and just the usage, all went fine.

If you want to test this program:
```bash
cd /tmp &&
git clone https://github.com/thepsauce/tag &&
cd tag &&
./test
```

### integration with yazi

Using the [yazi]() file manager, you can put the `tag.yazi` directory into
`.config/yazi/plugins/tag.yazi` (best to put the repo somewhere and then create
this as symlink for updates).

Then create a keybind to call the plugin, for example:
```toml
[[manager.prepend_keymap]]
on = [ "[" ]
run = "plugin tag"
desc = "add tags"
```

Then using `[` within yazi on a file within a child directory of all/.. and
having the tag script on the path.
