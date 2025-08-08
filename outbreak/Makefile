all:
	mkdir -p target
	cp artwork/* target/
	clang -g src/main.cpp src/Game.cpp -o target/OutBreak -lstdc++ -lSDL2 -lSDL2_image -lSDL2_gfx -I/opt/homebrew/include -L/opt/homebrew/lib -lm
