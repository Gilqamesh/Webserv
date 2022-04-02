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
	std::string					root;
	std::string					index;
	std::string					route;
	std::string					redirect;
	std::string					media_type;
	std::string					cgi_path;
	std::string					cgi_extension;
	std::vector<std::string>	methods;
}			t_location;

typedef struct s_server
{
	int							port;
	off_t						client_max_body_size;
	std::string					host;
	std::string					error_page;
	std::string					server_name;
	std::string					general_cgi_path;
	std::string					general_cgi_extension;
	std::vector<t_location>		locations;
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
		void						get_input(std::string file_name);
		void						parse_location(int idx, int start);
		void						error(std::string message) const;
		bool						is_number(std::string& port) const;
		bool						get_server_config(std::string& line);
		bool						line_is_empty(std::string& line) const;
		bool						line_is_comment(std::string& line) const;
		bool						header_is_valid(std::string& front, std::string& back) const;
		off_t						convert_to_bytes(std::string& size) const;
		const std::string			get_line_without_spaces(std::string& line) const;
	public:
		static std::vector<std::string>	get_words(std::string& line);
		conf_file(std::string file_name);
		~conf_file();

		std::vector<t_server>	get_configs() const;

		void		print();
};

#endif
