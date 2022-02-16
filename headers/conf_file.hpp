#ifndef CONF_FILE_HPP
# define CONF_FILE_HPP

# include <vector>
# include <iostream>
# include <fstream>

typedef enum	e_config
{
	NONE,
	PORT,
	ROOT,
	INDEX,
	LOCATION,
	PATH_NAME,
	SERVER_NAME
}				t_config;

typedef struct s_location
{
	std::string	root;
	std::string	index;
	std::string	path_name;
}			t_location;

typedef struct s_server
{
	int						port;
	std::string				server_name;
	std::vector<t_location>	locations;
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
		bool						line_is_empty(std::string& line) const;
		bool						line_is_comment(std::string& line) const;
		bool						header_is_valid(std::string& front, std::string& back) const;
		t_config					get_server_config(std::string& line);
		const std::string			get_line_without_spaces(std::string& line) const;
		std::vector<std::string>	get_words(std::string& line) const;
	public:
		conf_file(char *file_name);
		~conf_file();

		std::vector<t_server>	get_configs() const;

		void		print();
};

#endif
