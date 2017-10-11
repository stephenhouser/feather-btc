disp = require "display7"

displays = { main = 0x70 }

init(displays, 2, 1)
output(displays["main"], "    ")

function get_btc() 
    --http.get("http://blockchain.info/stats", nil, function(code, data)
    http.get("http://blockchain.info/ticker", nil, function(code, data)
        if (code < 0) then
            print("HTTP request failed")
        else
            --print(code, data)
            local usd = 0
            local usdline = false
            for line in data:gmatch("[^\r\n]+") do
                usdline = false
                for word in line:gmatch("%w+") do 
                    --print(word)
                    if nextword then
                        usd = word
                        nextword = false                        
                    end
                    
                    if usdline == true and word == "last" then
                        nextword = true
                    end

                    if word == "USD" then
                        usdline = true
                        print(line)
                    end
                end
            end
            print(usd)
            output(displays["main"], tostring(usd))
        end
    end)
end

-- initial population of btc value
output(displays["main"], "----")
get_btc()

local btctimer = tmr.create()
-- oo calling
btctimer:register(60000, tmr.ALARM_AUTO, function(t)
    output(displays["main"], "----")
    get_btc()
    node.heap()
end)
btctimer:start()
btctimer = nil
