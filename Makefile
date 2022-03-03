name = webserv
compile_flags = -Wall -Wextra -Werror -std=c++98
link_flags =
compiler = clang++
server_files = Network.cpp server.cpp utils.cpp http_request.cpp http_response.cpp resource.cpp conf_file.cpp CGI.cpp EventHandler.cpp
server_headers = Network.hpp header.hpp http_request.hpp server.hpp utils.hpp http_response.hpp resource.hpp conf_file.hpp CGI.hpp EventHandler.hpp
header_directory = headers
source_directory = sources
object_directory = objects
test_name = test

$(name): $(object_directory)/main.o $(foreach file,$(server_files:.cpp=.o),$(object_directory)/$(file))
	$(compiler) $(link_flags) -o $@ $(object_directory)/main.o $(foreach file,$(server_files:.cpp=.o),$(object_directory)/$(file))

# main.cpp
$(object_directory)/main.o: $(source_directory)/main.cpp $(foreach file,$(server_headers),$(header_directory)/$(file))
	cd $(object_directory) && $(compiler) $(compile_flags) -I../$(header_directory) -c ../$<

# for .cpp files that has their own .hpp
$(object_directory)/%.o: $(source_directory)/%.cpp $(header_directory)/%.hpp
	cd $(object_directory) && $(compiler) $(compile_flags) -I../$(header_directory) -c ../$<


# test

$(test_name): $(object_directory)/test.o $(foreach file,$(server_files:.cpp=.o),$(object_directory)/$(file))
	$(compiler) $(link_flags) -o $@ $(object_directory)/test.o $(foreach file,$(server_files:.cpp=.o),$(object_directory)/$(file))

$(object_directory)/test.o: $(source_directory)/test.cpp $(foreach file,$(server_headers),$(header_directory)/$(file))
	cd $(object_directory) && $(compiler) $(compile_flags) -I../$(header_directory) -c ../$<

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
