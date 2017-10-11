function init( addresses, sda, scl )
    i2c.setup( 0, sda, scl, i2c.SLOW )
    for name, address in pairs( addresses ) do
        write( address, 0x21 ) -- enable screen
        setBrightness( address, 7 ) -- min brightness
        output( address, "    " ) -- clear display
    end
end
 
function setBrightness( address, val ) -- 0 - off, 1 - 16 brightness level
    if val < 1 then
        write( address, 0x80 ) -- display "off"
    else    
        if val > 16 then val = 16 end
        write( address, 0x81 ) -- display "on"
        write( address, 0xE0 + val - 1 ) -- brightness 0 - 15
    end
end
 
function setBlink( address, yes )
    local val = yes == true and 0x85 or 0x81
    write( address, val ) -- blink or display "on"
end
 
function output( address, txt ) -- pass exactly 4 char string
    if txt:len() ~= 4 then return nil end
    write( address, { 0, chr( txt:sub(1,1) ), 0, chr( txt:sub(2,2) ), 0, 0, 0, chr( txt:sub(3,3) ), 0, chr( txt:sub(4,4) ) } )
end
 
function chr( x )
    local c = {[" "]=0,["-"]=64,["."]=128,["0"]=63,["1"]=6,["2"]=91,["3"]=79,["4"]=102,["5"]=109,["6"]=125,["7"]=7,["8"]=127,["9"]=111}
    if c[x] == nil then return cc[" "] end
    return c[x]
end
 
function write( address, data )
    i2c.start( 0 ) -- it is always zero
    i2c.address( 0, address, i2c.TRANSMITTER )
    i2c.write( 0, data )
    i2c.stop( 0 )
end
 
