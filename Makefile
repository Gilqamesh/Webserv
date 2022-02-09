name = webserv
compile_flags = -Wall -Wextra -Werror -std=c++98
link_flags =
compiler = clang++
source_files = main.cpp
header_directory = headers
source_directory = sources
object_directory = objects

$(name): $(object_directory)/$(source_files:.cpp=.o)
	$(compiler) $(link_flags) -o $@ $^

$(object_directory)/main.o: $(source_directory)/main.cpp $(header_directory)/*.hpp
	cd $(object_directory) && $(compiler) $(compile_flags) -c ../$<

$(object_directory)/%.o: $(source_directory)/%.cpp $(header_directory)/%.hpp
	cd $(object_directory) && $(compiler) $(compile_flags) -c ../$<

.PHONY: all clean re fclean bonus
all:
	make $(name)
clean:
	rm -f $(object_directory)/*.o
fclean: clean
	rm -f $(name)
re: fclean
	make $(name)
bonus: all
