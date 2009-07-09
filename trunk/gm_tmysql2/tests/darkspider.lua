require("tmysql")

tmysql.initialize("127.0.0.1", "root", "", "test", 3306, 6, 5)

tmysql.query("CREATE TABLE IF NOT EXISTS `rp_playerdata_test` (ID int(10) NOT NULL AUTO_INCREMENT, SteamID varchar(25), Inventory text, Money int(15), Vehicle tinyint(2), PRIMARY KEY (ID))", function(res,status,error) print("created: " .. status) end)

for i=0, 900 do
tmysql.query("INSERT INTO rp_playerdata_test VALUES (0, 'STEAM_0:1:4556804', '', 0xDEADBEEF, 23)")
end

function RPLoadCallback(pl,res)
	print("Player: " .. pl)
	PrintTable(res)
end

pl = 1
steamID = "STEAM_0:1:4556804"
local data = tmysql.query("SELECT * FROM rp_playerdata_test WHERE SteamID=\'"..steamID.."\' LIMIT 1",function(res,stat,err) RPLoadCallback(pl,res) end,1)