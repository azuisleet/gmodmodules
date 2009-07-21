require("tmysql")

tmysql.initialize("127.0.0.1", "root", "", "test", 3306, 2, 2)

local steamID = "azu"

tmysql.query("CREATE TABLE IF NOT EXISTS `fretta_players` (id int(11) NOT NULL AUTO_INCREMENT, steamid varchar(25), roundswon text, totalrounds text, money int(11), owned text, Equipped text, PRIMARY KEY (id)) ;")

tmysql.query("INSERT INTO fretta_players VALUES ('', 'azu', '1', '2', 555, 'a', 'b')", function(res, stat, id) print(stat, id) end, 2)

tmysql.query("SELECT money,owned,equipped FROM fretta_players WHERE steamid=\'"..steamID.."\'", function(res,stat,err) print(stat) PrintTable(res) end,1)