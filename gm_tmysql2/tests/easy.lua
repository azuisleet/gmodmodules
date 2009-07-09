require("tmysql")

tmysql.initialize("127.0.0.1", "root", "", "test", 3306, 6, 5)

function topass(bool)
	if(bool) then
		return "PASS"
	end
	return "FAIL"
end

tmysql.query("CREATE TABLE IF NOT EXISTS `roflmao` (id int(6) NOT NULL AUTO_INCREMENT, name varchar(255) NOT NULL, PRIMARY KEY (id)) ;")

tmysql.query("SELECT 5*5", function(res,status,error) print("Simple query: " .. topass(tonumber(res[1][1]) == 25)) end)

tmysql.query("SELECT 5*5", function(res,status,error) print("Assoc query: " .. topass(tonumber(res[1]["5*5"]) == 25)) end, 1)

tmysql.query("SELECT doesnotexist", function(res,status,error) print("Error test: " .. topass(error != nil)) end, 1)

count = 0

for i=1, 900 do
	tmysql.query("SELECT 1+1", function(res,status,error) if(status) then count = count + 1 end if(count == 900) then print("IT'S 900!") count = 0 end end)
end

function dolookup(res,status,lastid)
	print("joe added with id " .. lastid)
	tmysql.query("SELECT * FROM roflmao WHERE id = " .. lastid, function (res,status,error) PrintTable(res) end, 1)
end

tmysql.query("INSERT INTO roflmao VALUES (0, 'joe')", dolookup, 2)