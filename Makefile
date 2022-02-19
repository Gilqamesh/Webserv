name = webserv
compile_flags = -Wall -Wextra -Werror -std=c++98
link_flags =
compiler = clang++
server_files = server.cpp utils.cpp http_request.cpp http_response.cpp resource.cpp CGI.cpp
server_headers = header.hpp http_request.hpp server.hpp utils.hpp http_response.hpp resource.hpp CGI.hpp
header_directory = headers
source_directory = sources
object_directory = objects
test_name = test

$(name): $(object_directory)/main.o $(foreach file,$(server_files:.cpp=.o),$(object_directory)/$(file))
	$(compiler) $(link_flags) -o $@ $(object_directory)/main.o $(foreach file,$(server_files:.cpp=.o),$(object_directory)/$(file))

$(object_directory)/%.o: $(source_directory)/%.cpp $(foreach file,$(server_headers),$(header_directory)/$(file))
	cd $(object_directory) && $(compiler) $(compile_flags) -I../$(header_directory) -c ../$<

$(test_name): $(object_directory)/test.o $(foreach file,$(server_files:.cpp=.o),$(object_directory)/$(file))
	$(compiler) $(link_flags) -o $@ $(object_directory)/test.o $(foreach file,$(server_files:.cpp=.o),$(object_directory)/$(file))

.PHONY: all clean re fclean bonus
all:
	make $(name)
clean:
	rm -f $(object_directory)/*.o
fclean: clean
	rm -f $(name) $(test_name)
re: fclean
	make $(name)
bonus: all
