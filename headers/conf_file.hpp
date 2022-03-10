#ifndef CONF_FILE_HPP
# define CONF_FILE_HPP

# include <vector>
# include <iostream>
# include <fstream>
# include <cmath>
# include <map>

typedef struct s_location
{
	int							autoindex;
	off_t						client_max_body_size;
	std::string					root;
	std::string					index;
	std::string					route;
	std::string					language; // would be cool to implement
	std::string					media_type;
	std::vector<std::string>	methods;
}			t_location;

typedef struct s_server
{
	int							port;
	off_t						client_max_body_size;
	std::string					server_name;
	std::vector<t_location>		locations;
	std::string					error_page;
}			t_server;

class conf_file
{
	private:
		t_server								_server_buf;
		t_location								_location_buf;
		std::vector<t_server>					_servers;
		std::vector<std::vector<std::string> >	_input;

		void						parse_server();
		void						update_server_buffer();
		void						update_location_buffer();
		void						get_input(char *file_name);
		void						parse_location(int idx, int start);
		void						error(std::string message) const;
		bool						is_number(std::string& port) const;
		bool						get_server_config(std::string& line);
		bool						line_is_empty(std::string& line) const;
		bool						line_is_comment(std::string& line) const;
		bool						header_is_valid(std::string& front, std::string& back) const;
		off_t						convert_to_bytes(std::string& size) const;
		const std::string			get_line_without_spaces(std::string& line) const;
		std::vector<std::string>	get_words(std::string& line) const;
	public:
		conf_file(char *file_name);
		~conf_file();

		std::vector<t_server>	get_configs() const;

		void		print();
};

#endif
