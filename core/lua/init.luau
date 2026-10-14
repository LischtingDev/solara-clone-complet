-- init.luau : joue une fois apres la construction de l'environnement.
-- Tout ce qui est custom/politique se met ici plutot qu'en C++.

if getgenv == nil then
    getgenv = function() return _G end
end

local genv = getgenv()

genv.VESTA_EXECUTOR = true
genv.VESTA_VERSION  = "0.1.0"
genv.VESTA_IDENTITY = 8

-- Alias historiques attendus par les scripts publics.
genv.wait  = genv.wait  or task.wait
genv.spawn = genv.spawn or task.spawn
genv.delay = genv.delay or task.delay
genv.Wait  = genv.wait
genv.Spawn = genv.spawn
genv.Delay = genv.delay
genv.tick  = genv.tick  or os.clock

-- print capture vers la console de l'UI.
local real_print = genv.print or print
genv.print = function(...)
    local n = select("#", ...)
    local parts = table.create(n)
    for i = 1, n do parts[i] = tostring(select(i, ...)) end
    local line = table.concat(parts, " ")
    if genv.__sc_console then genv.__sc_console(line, "lua") end
    real_print(line)
end

genv.warn = function(...) genv.print("[warn]", ...) end

-- TODO(J4) : traceback formate sur erreur.
-- TODO(J6) : hookfunction, getrawmetatable, getconnections, firesignal.