local skynet = require "skynet"
local codecache = require "skynet.codecache"
local core = require "skynet.core"
local socket = require "socket"
local snax = require "snax"
local memory = require "memory"
local httpd = require "http.httpd"
local sockethelper = require "http.sockethelper"


-- one word room!!

local port = 8210

local COMMAND = {}

local user_login = {}


local info_alive
local function add_user(id, print)
    table.insert(user_login, id)
    info_alive(print)
end

local function filter_alive()
    active = {}
    for k,v in pairs(user_login)
    do
        if socket.isalive(v) then
            table.insert(active, v)
        end
    end
    user_login = active
end

function info_alive(print)
    filter_alive()
    print("alive:\n")
    print(table.unpack(user_login))
end

local function broadcast(message, id)
    filter_alive()

    id = id or ""; 
    for k,v in pairs(user_login)
    do
        local function print(...)
        	local t = { ... }
        	for k,v in ipairs(t) do
        		t[k] = tostring(v)
        	end
        	socket.write(v, id .. "> " .. table.concat(t,"\t"))
        	socket.write(v, "\n")
        end
        print(message)
    end
end        
    

local function talk_main_loop(stdin)

    local function print(...)
    	local t = { ... }
    	for k,v in ipairs(t) do
    		t[k] = tostring(v)
    	end
    	socket.write(stdin, table.concat(t,"\t"))
    	socket.write(stdin, "\n")
    end
	
    add_user(stdin, print)
	skynet.error(stdin, "connected")
    pcall(function()
        while true do
			local message = socket.readline(stdin, "\n")
			if not message then
				break
			end
			if message ~= "" then
				broadcast(message, stdin)
			end
		end
	end)
	skynet.error(stdin, "disconnected")
	socket.close(stdin)
end

skynet.start(function()
	local listen_socket = socket.listen ("127.0.0.1", port)
	skynet.error("Start talkbox at 127.0.0.1 " .. port)
	socket.start(listen_socket , function(id, addr)
		socket.start(id)
		skynet.fork(talk_main_loop, id )
	end)
end)



