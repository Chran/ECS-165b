all:
	bison HW4-sql.ypp
	flex --c++ HW4-sql.x
	g++ -std=c++11 HW4-parser.cpp -lboost_system -lboost_filesystem -o HW6
run:
	./HW6
clean:
	rm -f HW6