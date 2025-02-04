main.out, s.out, p.out: main.o logger.o store.o product.o
	g++ main.o logger.o -o main.out
	g++ store.o logger.o -o s.out
	g++ product.o logger.o -o p.out
main.o: main.cpp logger.cpp logger.hpp manual.hpp
	g++ -c main.cpp
logger.o: logger.cpp logger.hpp
	g++ -c logger.cpp
store.o: store.cpp logger.cpp logger.hpp manual.hpp
	g++ -c store.cpp
product.o: product.cpp logger.cpp logger.hpp manual.hpp
	g++ -c product.cpp
clean:
	rm *.o