
require( "tmysql" )
tmysql.initialize("127.0.0.1", "root", "", "test")

count = 0
max = 500

function output(res, status, error)
	print(res[1][1])
end

function querytest1()
	tmysql.query("SELECT 1 + 1", output)
	if(count < max) then
		timer.Simple(.1,querytest1)
	end
end


function querytest2()
	tmysql.query("SELECT 2 + 2", output)
	if(count < max) then
		timer.Simple(.1,querytest2)
	end
end

function querytest3()
	count = count + 1
	tmysql.query("SELECT 3 + 3", output)
	if(count < max) then
		timer.Simple(.1,querytest3)
	end
end

timer.Simple(.1,querytest1)
timer.Simple(.1,querytest2)
timer.Simple(.1,querytest3)
timer.Simple(.1,querytest1)
timer.Simple(.1,querytest2)