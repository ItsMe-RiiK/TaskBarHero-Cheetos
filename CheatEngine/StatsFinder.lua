[ENABLE]
{$lua}
if syntaxcheck then return end

--[[
  TaskBarHero - Hero Stats Finder v1.3 (Safe Multithread)
  Copyright © 2026 - Present RiiK
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
]]

-- All heroes name already registered by its heroes IDs
local HERO_MAP = {
  [101] = "Knight",   -- index 21
  [201] = "Ranger",   -- index 20
  [301] = "Sorcerer", -- index 19
  [401] = "Priest",   -- index 18
  [501] = "Hunter",   -- index 17
  [601] = "Slayer",   -- index 16
}

local STAT_OFFSETS = {
  MHP         = 0x7C, -- Max HP             (float, key 6)
  DPS         = 0x3C, -- Damage Per Second  (float, key 2)
  ATK_SPD     = 0x4C, -- Attack Speed       (float, key 3)
  CRIT_CHANCE = 0x5C, -- Crit Chance        (float x100 for %, key 4)
  CRIT_DMG    = 0x6C, -- Crit Damage        (float x100 for %, key 5)
  ARMOR       = 0x8C, -- Armor              (float, key 7)
  MOV_SPD     = 0x9C, -- Movement Speed     (float, key 7)
}

local HEADER_COLOR = 0x00FFFF 
local STAT_COLORS = {
  MHP         = 0x008000, 
  DPS         = 0x0000FF, 
  ATK_SPD     = 0x0055FF, 
  CRIT_CHANCE = 0xCC44FF, 
  CRIT_DMG    = 0x4444FF, 
  ARMOR       = 0xFF8800, 
  MOV_SPD     = 0x00FF00, 
}

-- Obtain base reference outside the background thread to avoid touching the GUI
local al = getAddressList()
local knownStatsArr = 0

-- find the max hp value and then put it into the list and rename it with whatever u want
-- then change the TARGET_RECORD_NAME to that name, then run this script
local TARGET_RECORD_NAME = "HPKnight" -- find value for max HP knight then rename on cheat engine description list.

local mhpRec = al.getMemoryRecordByDescription(TARGET_RECORD_NAME)
if mhpRec and mhpRec.CurrentAddress ~= 0 then
  knownStatsArr = mhpRec.CurrentAddress - 0x7C
  print(string.format("[HeroStatsFinder] ⏳ CALCULATING. PLS WAIT until the scan complete. Known Base: 0x%X", knownStatsArr))
else
  print(string.format("[HeroStatsFinder] ❌ ERROR: Benchmark '%s' not found!", TARGET_RECORD_NAME))
  print(string.format("[HeroStatsFinder] Find dynamic address Max HP, put it into Address List, then rename looks like: %s", TARGET_RECORD_NAME))
  return
end

-- ============================================================================
-- BACKGROUND THREAD: memory calculation (Crash-safe)
-- ============================================================================
createThread(function(thread)
  
  -- PHASE 1: MEMORY SCANNING
  local heroInfoAddrs = {}
  for heroId, heroName in pairs(HERO_MAP) do
    local idBytes = string.format("00 00 00 00 00 00 00 00 %02X %02X %02X %02X 00 00 00 00",
      heroId & 0xFF, (heroId >> 8) & 0xFF, (heroId >> 16) & 0xFF, (heroId >> 24) & 0xFF)

    local scan = AOBScan(idBytes, "+W-C")
    if scan then
      for si = 0, scan.Count - 1 do
        local matchAddr = tonumber(scan[si], 16)
        local possibleHI = matchAddr - 0x28 

        local strPtr = readPointer(possibleHI + 0x38)
        if strPtr and strPtr ~= 0 then
          local strLen = readInteger(strPtr + 0x10)
          if strLen and strLen > 8 and strLen < 30 then
            local s = ""
            for ci = 0, math.min(strLen - 1, 20) do
              local ch = readSmallInteger(strPtr + 0x14 + ci * 2)
              if ch and ch > 31 and ch < 127 then s = s .. string.char(ch) end
            end
            if s:match("HeroName_" .. tostring(heroId)) then
              heroInfoAddrs[heroId] = possibleHI
              break
            end
          end
        end
      end
      scan.destroy()
    end
  end

  local finalResults = {} -- Table to store raw findings
  local foundCount = 0

  -- PHASE 2: POINTER WALKING
  for heroId, hiAddr in pairs(heroInfoAddrs) do
    local ptrBytes = string.format("%02X %02X %02X %02X %02X %02X %02X %02X",
      hiAddr & 0xFF, (hiAddr >> 8) & 0xFF, (hiAddr >> 16) & 0xFF, (hiAddr >> 24) & 0xFF,
      (hiAddr >> 32) & 0xFF, (hiAddr >> 40) & 0xFF, (hiAddr >> 48) & 0xFF, (hiAddr >> 56) & 0xFF)

    local refScan = AOBScan(ptrBytes, "+W-C")
    if refScan then
      for ri = 0, refScan.Count - 1 do
        local refAddr = tonumber(refScan[ri], 16)
        local uyAddr = refAddr - 0x30

        local yu = readPointer(uyAddr + 0x10)
        if yu and yu ~= 0 then
          local dict = readPointer(yu + 0x20)
          if dict and dict ~= 0 then
            local statsArr = readPointer(dict + 0x18)
            if statsArr and statsArr ~= 0 then
              local count = readInteger(statsArr + 0x18)
              if count and count > 50 and count < 200 then
                
                -- Store the result in a temporary table (Do NOT register symbols here!)
                table.insert(finalResults, {
                  id = heroId,
                  name = HERO_MAP[heroId],
                  arrayAddr = statsArr
                })
                
                foundCount = foundCount + 1
                break
              end
            end
          end
        end
      end
      refScan.destroy()
    end
  end

  -- ============================================================================
  -- MAIN UI THREAD: Build GUI & register symbols in batch
  -- ============================================================================
  synchronize(function()
    
    -- 1. CLEANUP (Remove old records & symbols)
    for i = al.Count - 1, 0, -1 do
      local rec = al.getMemoryRecord(i)
      if rec and rec.Description:match("^%[Auto%]") then
        rec.destroy()
      end
    end
    for _, heroName in pairs(HERO_MAP) do
      for statName, _ in pairs(STAT_OFFSETS) do
        pcall(unregisterSymbol, heroName .. "_" .. statName)
      end
      pcall(unregisterSymbol, heroName .. "_StatsArray")
    end

    -- 2. BUILD UI & REGISTER SYMBOLS FROM SCAN RESULTS
    for _, heroData in ipairs(finalResults) do
      local hName = heroData.name
      local sArr = heroData.arrayAddr

      registerSymbol(hName .. "_StatsArray", sArr)

      local heroGroup = al.createMemoryRecord()
      heroGroup.Description = "[Auto] " .. hName .. " Stats"
      heroGroup.IsGroupHeader = true
      heroGroup.Color = HEADER_COLOR 

      for statName, offset in pairs(STAT_OFFSETS) do
        local statSymbol = hName .. "_" .. statName
        local statAddr = sArr + offset
        registerSymbol(statSymbol, statAddr)

        local rec = al.createMemoryRecord()
        rec.Description = hName .. " " .. statName
        rec.Address = statSymbol
        rec.Type = vtSingle
        rec.appendToEntry(heroGroup)

        if STAT_COLORS[statName] then
          rec.Color = STAT_COLORS[statName]
        end

        local val = readFloat(statAddr) or 0
        local displayVal = val
        local suffix = ""
        if statName == "MOV_SPD" then displayVal = val * 100 end
        if statName == "CRIT_CHANCE" then displayVal = val * 100; suffix = "%" end
        if statName == "CRIT_DMG" then displayVal = val * 100; suffix = "%" end
        print(string.format("  [%s] %s: addr=0x%X val=%.4f (display: %g%s)",
          hName, statName, statAddr, val, displayVal, suffix))
      end
    end

    print(string.format("[HeroStatsFinder] ✅ DONE! Founded %d/6 heroes. Pls take a look at list address", foundCount))
  end) -- End of synchronize
end) -- End of thread

{$asm}

[DISABLE]
{$lua}
if syntaxcheck then return end

local al = getAddressList()

-- Cleanup old script generated records
for i = al.Count - 1, 0, -1 do
  local rec = al.getMemoryRecord(i)
  if rec and rec.Description:match("^%[Auto%]") then
    rec.destroy()
  end
end

-- Unregister symbols to keep the table clean
local HERO_MAP = {
  [101] = "Knight", [201] = "Ranger", [301] = "Sorcerer",
  [401] = "Priest", [501] = "Hunter", [601] = "Slayer"
}
local STAT_OFFSETS = { MHP = 0x7C, DPS = 0x3C, ATK_SPD = 0x4C, CRIT_CHANCE = 0x5C, CRIT_DMG = 0x6C, ARMOR = 0x8C, MOV_SPD = 0x9C }

for _, heroName in pairs(HERO_MAP) do
  for statName, _ in pairs(STAT_OFFSETS) do
    pcall(unregisterSymbol, heroName .. "_" .. statName)
  end
  pcall(unregisterSymbol, heroName .. "_StatsArray")
end
{$asm}