name = webserv
compile_flags = -Wall -Wextra -Werror -std=c++98
link_flags =
compiler = clang++
source_files = main.cpp
header_directory = headers
source_directory = sources
object_directory = objects
server_name = server
client_name = client

$(name): $(object_directory)/$(source_files:.cpp=.o)
	$(compiler) $(link_flags) -o $@ $^

$(object_directory)/main.o: $(source_directory)/main.cpp $(header_directory)/*.hpp
	cd $(object_directory) && $(compiler) $(compile_flags) -I../$(header_directory) -c ../$<

$(object_directory)/%.o: $(source_directory)/%.cpp # $(header_directory)/%.hpp
	cd $(object_directory) && $(compiler) $(compile_flags) -I../$(header_directory) -c ../$<

.PHONY: all clean re fclean bonus server client
all:
	make $(name)
clean:
	rm -f $(object_directory)/*.o
fclean: clean
	rm -f $(name) $(server_name) $(client_name)
re: fclean
	make $(name)
bonus: all
server:
	$(compiler) $(link_flags) -o $(server_name) $(object_directory)/server.o
client:
	$(compiler) $(link_flags) -o $(client_name) $< $(object_directory)/client.o
