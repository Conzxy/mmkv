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

-- default: output diagostic log to terminal
-- If directory is empty, behavior like as default
--DiagosticLogDirectory: ./log

-- default: 0B
-- The maximum memory usage.
-- Starting replace key to release space 
-- when memory usage over the limit,
MaxMemoryUsage = "100.11 MB"

function ParseMemoryUsage(usage)
  if #usage < 2 or not string.match(usage, "%d*%.?%d+[ ]*[kKmMgG]?[Bb]") then
    return -1
  end
  
  if string.byte(usage, #usage-1) >= string.byte('0') and
     string.byte(usage, #usage-1) <= string.byte('9') then
    return math.floor(string.sub(usage, 1, #usage-1))
  end
  
  numeric = tonumber(string.sub(usage, 1, #usage-2))
  
  size_hint = string.byte(string.lower(usage), #usage-1)

  if size_hint == string.byte('k') then
    return math.floor(numeric * (1 << 10))
  elseif size_hint == string.byte('m') then
    return math.floor(numeric * (1 << 20))
  elseif size_hint == string.byte('g') then
    return math.floor(numeric * (1 << 30))
  end
end

-- default: 4096
EveryShardNum = 4096

-- default: 
ConfigServerEndpoint = "0.0.0.0:9997"

-- default: RouterPort + 10000
TrackerEndpoint = "0.0.0.0:19997"

-- default: empty
DataNodes = {
  "1",
  "2",
  "3",
}
