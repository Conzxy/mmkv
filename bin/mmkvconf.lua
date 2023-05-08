--[[
This is the config file of mmkv server.
The syntax and format follow the Lua language(In fact, this is a lua file).
--]]

-- The method to log database data to somewhere or not
-- Options:
-- * request: log MMBP request content
-- * none: don't log
-- NOTICE:
-- If method is not request, don't recover from request log also.
LogMethod = "NONE"

-- default: 0 seconds
-- The granularity is second.
-- Check expired key actively.
-- If the cycle is not greater than 0, check is disabled.
ExpirationCheckCycle = 0

-- default: /tmp/.mmkv-request.log
-- Store the request content
RequestLogLocation = "/tmp/.mmkv-request.log"

-- default: disable lazy check expiration
-- When acquire and modify the key,
-- check the key if is expired first.
LazyExpiration = true

-- default: disable replacement
-- The policy to replace key when maximum memory usage of database is reached
-- Options:
-- * LRU(Least-recently-used)
-- * NONE(No replace any key, i.e. don't limit the memory usage)
ReplacePolicy = "NONE"

-- default: 0B
-- The maximum memory usage.
-- Starting replace key to release space 
-- NOTICE: 0 bytes indicates disable
MaxMemoryUsage = "0B"

function ParseMemoryUsage(usage)
  if #usage < 2 or not string.find(usage, "%d*%.?%d+ *[kKmMgG]?[Bb]") then
    return nil
  end
  
  if string.byte(usage, #usage-1) >= string.byte('0') and
     string.byte(usage, #usage-1) <= string.byte('9') then
    return tonumber(string.sub(usage, 1, #usage-1))
  end
  
  numeric = tonumber(string.sub(usage, 1, #usage-2))
  
  size_hint = string.byte(string.lower(usage), #usage-1)

  if size_hint == string.byte('k') then
    return numeric * (1 << 10)
  elseif size_hint == string.byte('m') then
    return numeric * (1 << 20)
  elseif size_hint == string.byte('g') then
    return numeric * (1 << 30)
  end
end

-----------------------------------------
-- Multithread
-----------------------------------------
ThreadNum = 1

------------------------------------------
-- Shard Configuration
------------------------------------------

-- Enable shard
-- If this is configured to false, the following 
-- configs related sharder are disabled also.
--
-- default: false
-- EnableShard = false
-- To enable shard, see ShardControllerEndPoint

-- default: 4096
ShardNum = 4096

-- default: 
-- ConfigServerEndpoint = "*.9997"
ConfigServerEndpoint = "*:9997"

-- To disable shard, you can set this to empty string
-- default: RouterPort + 10000
ShardControllerEndpoint = "*:19997"

-- default: ShardControllerPort + 1
SharderEndpoint = "*:19998"

ForwarderEndpoint = "*:9994"

-- default: empty
DataNodes = {
}
