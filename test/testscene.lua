local skynet = require "skynet"

local profile = require "profile"


local command = {}

function test_adduser(uid, x, y)
    skynet.scene(2, uid, x, y);
end

function test_leave(uid)
    skynet.scene(1, uid);
end

function test_move(uid, x, y)
    skynet.scene(3,uid, x, y);
end

function test_debug()
    skynet.scene(0, -1)
end


--print (skynet)
skynet.start(function()
   test_debug()
   i=0
   profile.start()

   while(i<3) do
        --print (i)
        test_adduser(i, i+2, i+2)
   test_debug()
     --   test_leave(i)
  -- test_debug()
        i = i+1
   end
   local time = profile.stop()
   
   skynet.error("time is %d", time);
  -- test_debug()
end)
    
