local M = {}

function tag_file(file, tags)
    local r = os.execute("bash -c \"../tag -f " .. file .. " -a " .. tags .. " -w >/dev/null\"")
    if not r then
        ya.notify {
            title = "Tag error",
            content = "Could not tag file " .. file,
            timeout = 6.5
        }
    end
end

local tag_files = ya.sync(function(state, tags)
    local folder = Folder:by_kind(Folder.CURRENT)
    if not folder then
        return
    end

    local marked = false
    for i, f in ipairs(folder.window) do
        local marker = File:marker(f)
        if marker ~= 0 then
            marked = true
            tag_file(f.name, tags)
        end
    end

    if not marked then
        local h = cx.active.current.hovered
        if h then
            tag_file(h.name, tags)
        end
    end

end)

function M:entry()
    local value, event = ya.input {
	    title = "Tag file",
	    position = { "center", x = -25, w = 50 },
    }
    if event == 1 then
        tag_files(value)
    end
end

return M
