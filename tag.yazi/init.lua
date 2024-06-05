local M = {}

function tag_file(file, tags)
    file = file:gsub("\"", "\\\"")
    tags = tags:gsub("\"", "\\\"")
    local cmd = "bash -c \"tag -f \\\"" .. file .. "\\\" -a \\\"" .. tags .. "\\\" -w >/dev/null 2>/dev/null\""
    local r = os.execute(cmd)
    if not r then
        ya.notify {
            title = "Tag error",
            content = "Could not tag " .. file,
            timeout = 6.5
        }
    end
end

local tag_files = ya.sync(function(state, tags)
    local folder = Folder:by_kind(Folder.CURRENT)
    if not folder then
        return
    end

    local sel = cx.active.selected
    if #sel == 0 then
        local h = cx.active.current.hovered
        if h then
            tag_file(h.name, tags)
        end
    else
        for i, f in pairs(sel) do
            tag_file(f:name(), tags)
        end
    end
end)

local get_tags = ya.sync(function(state)
    local folder = Folder:by_kind(Folder.PARENT)

    if not folder then
        return nil
    end

    local tags = {}
    local all = os.getenv("TAG_DEFAULT_DIR")
    if not all then
        all = "all"
    end
    for i, f in ipairs(folder.window) do
        if f.cha.is_dir then
            table.insert(tags, f.name)
        end
    end
    return tags
end)

function gen_cands(tags, str)
--[[ old algorithm..
    local cands = {}
    for i, t in ipairs(tags) do

    end
    for i, t in ipairs(tags) do
        if t == str then
            table.insert(cands, { on = t:sub(#str + 1, #str + 1):upper(), desc = t })
        elseif t:sub(1, #str):lower() == str then
            local on = t:sub(#str + 1, #str + 1):lower()
            if #cands > 0 and cands[#cands].on == on then
                cands[#cands].desc = cands[#cands].desc .. "|" .. t
            else
                table.insert(cands, { on = on, desc = t })
            end
        end
    end
    return cands
--]]

    table.sort(tags)

    local run = {}

    for _, t in ipairs(tags) do
        table.insert(run, { w = t, i = 1 })
    end

    local cands = { }
    while #cands ~= #run do
        cands = { }
        local i = 1
        while i < #run + 1 do
            local r = run[i]
            local c = r.w:sub(r.i, r.i)
            local min = #r.w - r.i
            local min_index = i
            i = i + 1
            while i < #run + 1 do
                local ro = run[i]
                if ro.w:sub(ro.i, ro.i) == c then
                    if #ro.w - r.i < min then
                        run[min_index].i = run[min_index].i + 1
                        min_index = i
                        min = #ro.w - r.i
                    else
                        ro.i = ro.i + 1
                    end
                else
                    break
                end
                i = i + 1
            end
            table.insert(cands, { on = c, desc = run[min_index].w })
        end
    end
    return cands
end

function filter_cands(cands, key)
    local tags = { }
    for _, c in ipairs(cands) do
        if c.on == key then
            table.insert(tags, c.desc)
        end
    end
    return tags
end

function compact_cands(cands)
    local com = { }
    for i = 1, #cands do
        local c = cands[i]

        local found = false

        for _, co in ipairs(com) do
            if c.on == co.on then
                found = true
                break
            end
        end

        if not found then
            local group = { }

            table.insert(group, c.desc)
            for j = i + 1, #cands do
                local co = cands[j]
                if c.on == co.on then
                    table.insert(group, co.desc)
                end
            end

            local s = group[1]
            for i = 2, #group do
                s = s .. "|" .. group[i]
            end
            table.insert(com, { on = c.on, desc = s })
        end
    end
    return com
end

function input_tag(state, tags)
    local key
    local cand, cands, comp

    cands = gen_cands(tags, keys)
    -- additional + and . binds for first time
    table.insert(cands, { on = "+", desc = "Add tags" })
    if state.rep then
        table.insert(cands, { on = ".", desc = "Repeat" })
    end

    comp = compact_cands(cands)
    cand = ya.which { cands = comp }
    if not cand then
        return
    end

    key = comp[cand].on
    -- handle the special binds
    if key == "+" then
        t, e = ya.input {
            title = "Tag file",
            position = { "center", x = -25, w = 50 },
        }
        if e == 1 then
            tag_files(t)
        end
        return
    end
    if key == "." then
        if state.rep then
            tag_files(state.rep)
        end
        return
    end

    -- remove special keys
    table.remove(cands)
    table.remove(cands)

    tags = filter_cands(cands, key)

    --  get key presses until we have a unique tag to pick
    while #tags > 1 do
        cands = gen_cands(tags)
        comp = compact_cands(cands)
        cand = ya.which { cands = comp }
        if not cand then
            return
        end
        key = comp[cand].on
        tags = filter_cands(cands, key)
    end
    tag_files(tags[1])
end

function M:setup(state)
    -- saved for repeating action
    state.rep = nil
end

function M:entry(state)
    local tags = get_tags()
    if not tags then
        ya.notify {
            title = "Tag error",
            content = "Can't tag in here",
            timeout = 6.5
        }
    else
        input_tag(state, tags)
    end
end

return M
