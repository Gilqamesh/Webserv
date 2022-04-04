#include "../headers/conf_file.hpp"

conf_file::conf_file(std::string file_name)
{
	if (file_name.substr(file_name.find_last_of(".") + 1) != "conf")
		error("wrong conf file extension. Usege: ./webser <name>.conf");
	get_input(file_name);
	update_server_buffer();
	update_location_buffer();
	parse_server();
}

conf_file::~conf_file()
{

}

std::vector<t_server>	conf_file::get_configs() const 
{
	return (_servers);
}

void					conf_file::get_input(std::string file_name)
{
	int							brackets[2];
	std::string					line;
	std::ifstream				input(file_name);
	std::vector<std::string>	buf;

	brackets[0] = 0;
	brackets[1] = 0;
	if (input == 0)
		error("file does not exist");
	while (std::getline(input, line))
	{
		if (line_is_comment(line) || line_is_empty(line))
			continue ;
		if (line.find('{') != std::string::npos)
			brackets[0]++;
		if (line.find('}') != std::string::npos)
			brackets[1]++;
		buf.push_back(line);
		if (brackets[0] == brackets[1] && brackets[0] != 0)
		{
			_input.push_back(buf);
			buf.clear();
			brackets[0] = 0;
			brackets[1] = 0;
		}
	}
	if (brackets[0] != 0 || brackets[1] != 0)
		error("invalid breackets");
}

void					conf_file::parse_server()
{
	for (size_t i = 0; i < _input.size(); i++)
	{
		if (!header_is_valid(_input[i].front(), _input[i].back()))
			error("invalid header");
		for (size_t j = 1; j < _input[i].size() - 1; j++)
		{
			if (get_server_config(_input[i][j]))
			{
				parse_location(i, j);
				while (get_line_without_spaces(_input[i][j]) != "}")
					j++;
			}
		}
		_servers.push_back(_server_buf);
		update_server_buffer();
	}
}

bool					conf_file::get_server_config(std::string &line)
{
	std::vector<std::string>	words;

	words = get_words(line);
	if (words.front() == "location" && words.size() == 3 && words.back() == "{")
		return (1);
	if (words.back().back() != ';')
		error("no semicolon");
	words.back().pop_back();
	if (words.back().empty())
		error("wrong config value");
	if (words.size() == 2 && words.front() == "error_page")
	{
		if (!_server_buf.error_page.empty())
			error("error page already exist");
		_server_buf.error_page = words.back();
	}
	else if (words.size() == 2 && words.front() == "listen")
	{
		if (_server_buf.port != -1)
			error("port already exist");
		_server_buf.host = words[1].substr(0, words[1].find(':'));
		std::string port = words[1].substr(words[1].find(':') + 1, words[1].size() - _server_buf.host.size() - 1);
		if (!is_number(port))
			error("wrong port");
		_server_buf.port = std::stoi(port);
	}
	else if (words.size() == 2 && words.front() == "server_name")
	{
		if (!_server_buf.server_name.empty())
			error("server_name already exist");
		_server_buf.server_name = words[1];
	}
	else if (words.size() == 2 && words.front() == "client_max_body_size")
	{
		if (_server_buf.client_max_body_size != -1)
			error("client_max_body_size already exist");
		_server_buf.client_max_body_size = convert_to_bytes(words[1]);
	}
	else if (words.size() == 2 && words.front() == "general_cgi_extension")
	{
		if (!_server_buf.general_cgi_extension.empty())
			error("general_cgi_extension already exist");
		_server_buf.general_cgi_extension = words[1];
	}
	else if (words.size() == 2 && words.front() == "general_cgi_path")
	{
		if (!_server_buf.general_cgi_path.empty())
			error("general_cgi_path already exist");
		_server_buf.general_cgi_path = words[1];
	}
	else
		error("wrong config name - " + line);
	return (0);
}

void					conf_file::parse_location(int idx, int start)
{
	std::vector<std::string>	words;

	words = get_words(_input[idx][start]);
	_location_buf.route = words[1];
	start++;
	while (get_line_without_spaces(_input[idx][start]) != "}")
	{
		words = get_words(_input[idx][start]);
		if (words.back().back() != ';')
			error("no semicolon");
		words.back().pop_back();
		if (words.back().empty())
			error("wrong config value");
		if (words.size() == 2 && words.front() == "root")
		{
			if (!_location_buf.root.empty())
				error("root aleady exist");
			_location_buf.root = words[1];
		}
		else if (words.size() == 2 && words.front() == "index")
		{
			if (!_location_buf.index.empty())
				error("index already exist");
			_location_buf.index = words[1];
//			_location_buf.media_type = words[1].substr(words[1].find_last_of('.'), words[1].size());
		}
		else if (words.size() >= 2 && words.front() == "method")
		{
			for (size_t i = 1; i < words.size(); i++)
			{
				for (size_t j = 0; j < _location_buf.methods.size(); j++)
					if (_location_buf.methods[j] == words[i])
						error("method already exist");
				_location_buf.methods.push_back(words[i]);
			}
		}
		else if (words.size() == 2 && words.front() == "cgi_extension")
		{
			if (!_location_buf.cgi_extension.empty())
				error("cgi_extension for location already exist");
			_location_buf.cgi_extension = words.back();
		}
		else if (words.size() == 2 && words.front() == "cgi_path")
		{
			if (!_location_buf.cgi_path.empty())
				error("cgi_path for location already exists");
			_location_buf.cgi_path = words.back();
		}
		else if (words.size() == 2 && words.front() == "redirect")
		{
			if (!_location_buf.redirect.empty())
				error("redirect already exists");
			_location_buf.redirect = words.back();
		}
        else if (words.size() == 2 && words.front() == "client_max_body_size")
		{
			if (_location_buf.client_max_body_size != -1)
				error("client_max_body_size already exists");
			_location_buf.client_max_body_size = convert_to_bytes(words.back());
		}
		else if (words.size() == 2 && words.front() == "autoindex")
		{
			if (_location_buf.autoindex != -1)
				error("autoindex already exist");
			if (words[1] == "on")
				_location_buf.autoindex = 1;
			else if (words[1] == "off")
				_location_buf.autoindex = 0;
			else
				error("wrong autoindex value");
		}
		else
			error("wrong location config name");
		start++;
	}   
	_server_buf.locations.push_back(_location_buf);
	update_location_buffer();
}

void						conf_file::update_server_buffer()
{
	_server_buf.port = -1;
	_server_buf.client_max_body_size = -1;
	if (!_server_buf.locations.empty())
		_server_buf.locations.clear();
	if (!_server_buf.server_name.empty())
		_server_buf.server_name.clear();
	if (!_server_buf.host.empty())
		_server_buf.host.clear();
	if (!_server_buf.error_page.empty())
		_server_buf.error_page.clear();
	if (!_server_buf.general_cgi_path.empty())
		_server_buf.general_cgi_path.clear();
	if (!_server_buf.general_cgi_extension.empty())
		_server_buf.general_cgi_extension.clear();
}

void						conf_file::update_location_buffer()
{
	_location_buf.autoindex = -1;
    _location_buf.client_max_body_size = -1;
	if (!_location_buf.root.empty())
		_location_buf.root.clear();
	if (!_location_buf.index.empty())
		_location_buf.index.clear();
	if (!_location_buf.route.empty())
		_location_buf.route.clear();
	if (!_location_buf.redirect.empty())
		_location_buf.redirect.clear();
	if (!_location_buf.methods.empty())
		_location_buf.methods.clear();
	if (!_location_buf.media_type.empty())
		_location_buf.media_type.clear();
	if (!_location_buf.cgi_extension.empty())
		_location_buf.cgi_extension.clear();
	if (!_location_buf.cgi_path.empty())
		_location_buf.cgi_path.clear();
}

off_t						conf_file::convert_to_bytes(std::string& size) const
{
	size_t		i;
	std::string	type;

	for (i = 0; i < size.size(); i++)
		if (!isdigit(size[i]))
			break ;
	if (i == 0)
		error("wrong client_max_body_size");
	type = size.substr(i, size.size() - i);
	if (type == "B" || type.empty())
		return (stoi(size.substr(0, i)));
	else if (type == "KB" || type == "K")
		return (stoi(size.substr(0, i)) * 1024);
	else if (type == "MB" || type == "M")
		return (stoi(size.substr(0, i)) * pow(1024, 2));
	else if (type == "GB" || type == "G")
		return (stoi(size.substr(0, i)) * pow(1024, 3));
	else
		error("wrong client_max_body_size size type");
	return (0);
}

std::vector<std::string>	conf_file::get_words(std::string &line)
{
	std::string					buf;
	std::vector<std::string>	words;

	for (size_t i = 0; i < line.size(); i++)
	{
		while (isspace(line[i]) && i < line.size())
			i++;
		while (!isspace(line[i]) && i < line.size())
		{
			buf.push_back(line[i]);
			i++;
		}
		words.push_back(buf);
		buf.clear();
	}
	return (words);
}

void						conf_file::error(std::string message) const
{
	std::cout << message << std::endl;
	exit(1);
}

bool						conf_file::line_is_comment(std::string &line) const
{
	for (size_t i = 0; i < line.size(); i++)
	{
		if (isspace(line[i]))
			continue ;
		else if (line[i] == '#')
			return (1);
		else
			return (0);
	}
	return (0);
}

bool						conf_file::line_is_empty(std::string &line) const
{
	if (line.empty())
		return (1);
	for (size_t i = 0; i < line.size(); i++)
		if (!isspace(line[i]))
			return (0);
	return (1);
}

const std::string			conf_file::get_line_without_spaces(std::string &line) const
{
	std::string	res;

	for (size_t i = 0; i < line.size(); i++)
		if (!isspace(line[i]))
			res.push_back(line[i]);

	return (res);
}

bool						conf_file::is_number(std::string& port) const
{
	for (size_t i = 0; i < port.size(); i++)
		if (!isdigit(port[i]))
			return (0);
	return (1);
}

bool						conf_file::header_is_valid(std::string &front, std::string &back) const
{
	if (get_line_without_spaces(front).compare("server{") ||
		get_line_without_spaces(back).compare("}"))
		return (0);
	return (1);
}
