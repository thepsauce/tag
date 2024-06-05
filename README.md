# tag
file tagging system

## Specification

The following specification is used for the tag script:
All files that are allowed to be tagged shall be in a directory called
`$TAG_DEFAULT_DIR` (the default of that is `all`). Tags are stored as symlinks
within neighboring directories, you should never change `TAG_DEFAULT_DIR` after
you have chosen a name for it (or redo all symlinks).

This specification allows for:
- Easy reproducibility
- Good integration with the file system
- You can throw away the tag script and manage it yourself

## Installation (Linux)

Put the tag script somewhere on your path for convenience. Create a dedicated
directory for the tag directories (named `$TAG_DEFAULT_DIR`) and put some files
into there. Now call the script for the first time within that directory:
`tag -h`. If there is no error message at the beginning and only the usage, all went fine.

If you want to test this program:
```bash
cd /tmp &&
git clone https://github.com/thepsauce/tag &&
cd tag &&
./test
```
If you have yazi, the test program will detect this, put configuration files in
`~/.config/yazi` and start yazi. If you did not yet bind `[` to anything else,
it will be usable to tag a file.
To clean up after the tests, remove `~/.config/yazi/plugins/tag.yazi` and remove
the keybind from `~/.config/yazi/keybind.toml` (search for `run = "plugin tag"`).

### Integration with yazi

Using the [yazi](https://yazi-rs.github.io/) file manager, you can put the `tag.yazi` directory into
`.config/yazi/plugins/tag.yazi` (best to put the repo somewhere and then create
this as symlink for updates).

Then create a keybind to call the plugin, for example:
```toml
[[manager.prepend_keymap]]
on = [ "[" ]
run = "plugin tag"
desc = "add tags"
```

Then using `[` within yazi on a file within a child directory of `$TAG_DEFAULT_DIR/..` and
having the tag script on the path.
