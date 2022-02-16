#include "../headers/conf_file.hpp"

conf_file::conf_file(char *file_name)
{
	get_input(file_name);
	parse_server();
}

conf_file::~conf_file()
{

}

std::vector<t_server>	conf_file::get_configs() const 
{
	return (_servers);
}

void					conf_file::get_input(char *file_name)
{
	int							brackets[2];
	std::string					line;
	std::ifstream				input(file_name);
	std::vector<std::string>	buf;

	brackets[0] = 0;
	brackets[1] = 0;
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
			if (get_server_config(_input[i][j]) == LOCATION)
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

void					conf_file::parse_location(int idx, int start)
{
	std::vector<std::string>	words;

	words = get_words(_input[idx][start]);
	_location_buf.path_name = words[1];
	start++;
	while (get_line_without_spaces(_input[idx][start]) != "}")
	{
		words = get_words(_input[idx][start]);
		if (words.size() != 2 || words[1].back() != ';')
			error("invalid line");
		words[1].pop_back();
		if (words.front() == "root")
			_location_buf.root = words[1];
		else if (words.front() == "index")
			_location_buf.index = words[1];
		else
			error("worong location config name");
		start++;
	}
	_server_buf.locations.push_back(_location_buf);
	update_location_buffer();
}

void						conf_file::update_server_buffer()
{
	_server_buf.port = -1;
	if (!_server_buf.locations.empty())
		_server_buf.locations.clear();
	if (!_server_buf.server_name.empty())
		_server_buf.server_name.clear();
}

void						conf_file::update_location_buffer()
{
	if (!_location_buf.root.empty())
		_location_buf.root.clear();
	if (!_location_buf.index.empty())
		_location_buf.index.clear();
	if (!_location_buf.path_name.empty())
		_location_buf.path_name.clear();
}

t_config					conf_file::get_server_config(std::string &line)
{
	std::vector<std::string>	words;

	words = get_words(line);
	if (words.front() == "location" && words.size() == 3 && words.back() == "{")
		return (LOCATION);
	if (words.size() != 2 || words[1].back() != ';')
		error("invalid line");
	words[1].pop_back();
	if (words.front() == "listen")
		_server_buf.port = std::stoi(words[1]);
	else if (words.front() == "server_name")
		_server_buf.server_name = words[1];
	else
		error("wrong config name");
	return (NONE);
}

std::vector<std::string>	conf_file::get_words(std::string &line) const
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
		if (line[i] == ' ')
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

bool						conf_file::header_is_valid(std::string &front, std::string &back) const
{
	if (get_line_without_spaces(front).compare("server{") ||
		get_line_without_spaces(back).compare("}"))
		return (0);
	return (1);
}

void	conf_file::print()
{
	for (size_t i = 0; i < _servers.size(); i++)
	{
		std::cout << _servers[i].port << std::endl;
		std::cout << _servers[i].server_name << std::endl;
		for (size_t j = 0; j < _servers[i].locations.size(); j++)
		{
			std::cout << _servers[i].locations[j].root << std::endl;
			std::cout << _servers[i].locations[j].index << std::endl;
			std::cout << _servers[i].locations[j].path_name << std::endl;
		}
	}
}