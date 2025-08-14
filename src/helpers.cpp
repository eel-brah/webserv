
#include "../include/helpers.hpp"
#include "../include/parser.hpp"
#include "../include/errors.hpp"
#include <algorithm>
#include <cctype>
#include <iomanip>

// std::vector<std::string> split(const std::string &str, char delimiter) {
//     std::vector<std::string> tokens;
//     size_t start = 0;
//     size_t end = str.find(delimiter);
//     
//     while (end != std::string::npos) {
//         tokens.push_back(str.substr(start, end - start)); // Extract substring
//         start = end + 1;
//         end = str.find(delimiter, start);
//     }
//     tokens.push_back(str.substr(start)); // Add the last token
//     return tokens;
// }

std::string join(const std::vector<std::string>& vec, const std::string& delimiter) {
    std::ostringstream oss;
    for (size_t i = 0; i < vec.size(); ++i) {
        oss << vec[i];
        if (i != vec.size() - 1) {
            oss << delimiter;  // Add delimiter between elements
        }
    }
    return oss.str();
}

// convert HTTP_METHOD to printable string
// returns "NONE" for NONE
std::string httpmethod_to_string(HTTP_METHOD method) {
  switch (method) {
    case GET:
      return "GET";
    case POST:
      return "POST";
    case OPTIONS:
      return "OPTIONS";
    case DELETE:
      return "DELETE";
    default:
      return "NONE";
  }
}

std::string httpversion_to_string(HTTP_VERSION method) {
  switch (method) {
    case HTTP1:
      return "HTTP1";
  }
  return "not suppose to be reached";
}

std::string toLower(std::string str) {
  for (size_t i = 0; i < str.length(); i++) {
    str[i] = std::tolower(str[i]);
  }
  return str;
}



// trim from end of string (right)
std::string& rtrim(std::string& s, const char* t)
{
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}

// trim from beginning of string (left)
std::string& ltrim(std::string& s, const char* t)
{
    s.erase(0, s.find_first_not_of(t));
    return s;
}

// trim from both ends of string (right then left)
std::string& trim(std::string& s, const char* t)
{
    return ltrim(rtrim(s, t), t);
}

void catch_setup_serverconf(Client *client, std::vector<ServerConfig> &servers_conf) {
  if (!client->get_request()) {
    try {
    client->set_request(new HttpRequest());
  } catch (std::exception &e) {
      LOG_STREAM(ERROR, e.what());
      return;
    }
  }
  try {
    if (!client->get_request()->server_conf)
      client->get_request()->setup_serverconf(servers_conf, client->port);
  } catch (std::exception &e) {
    //NOTE: im not sure why u need this cuz the case of no header in the request is handled inside setup_serverconf
    client->get_request()->server_conf = &servers_conf[0]; // set 0 as default
  }
}

unsigned char hex_char_to_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    return 0; // Fallback (shouldn't occur with proper validation)
};

std::string decode_url(const std::string& encoded) {
    // Helper lambda to convert hex char to numerical value
    std::string decoded;
    decoded.reserve(encoded.size()); // Pre-allocate memory

    for (size_t i = 0; i < encoded.size(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.size()) {
            const char c1 = encoded[i + 1];
            const char c2 = encoded[i + 2];
            
            if (std::isxdigit(static_cast<unsigned char>(c1)) &&
                std::isxdigit(static_cast<unsigned char>(c2))) {
                // Convert hex digits to byte value
                const unsigned char byte = 
                    (hex_char_to_value(c1) << 4) | 
                    hex_char_to_value(c2);
                decoded += static_cast<char>(byte);
                i += 2; // Skip processed hex characters
            } else {
                // Invalid encoding - leave % as-is
                decoded += '%';
            }
        } else {
            // Preserve character (including '+' and invalid %)
            decoded += encoded[i];
        }
    }
    
    return decoded;
}

std::string replace_first(const std::string& str, const std::string& old_sub, const std::string& new_sub) {
    if (old_sub.empty()) {
        return new_sub + str;
    }

    size_t pos = str.find(old_sub);
    if (pos == std::string::npos) {
        return str;  // No match found
    }

    return str.substr(0, pos) + new_sub + str.substr(pos + old_sub.size());
}

std::string clean_path(const std::string url) {
    std::string clean = url;
    while (clean != replace_first(clean, "//", "/")) {
        clean = replace_first(clean, "//", "/");
    }

    std::vector<std::string> parts = split(clean, '/');
    std::vector<std::string> clean_parts = std::vector<std::string>();
    
    for (size_t i = 0; i < parts.size(); i++) {
        if (parts[i] == "..") {
            if (!clean_parts.empty())
                clean_parts.pop_back();
        } else {
            clean_parts.push_back(parts[i]);
        }
    }

    clean = join(clean_parts, "/");

    while (clean != replace_first(clean, "./", "")) {
        clean = replace_first(clean, "./", "");
    }

    if (clean[0] != '/')
        clean = "/" + clean;

    return clean;
}


std::string bufferToHexString(const uint8_t* buffer, size_t length) {
    std::ostringstream oss;

    for (size_t i = 0; i < length; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(buffer[i]);
    }

    return oss.str();
}

std::string random_string() {
  std::srand(std::time(0));
  int rand_number = rand();

  std::stringstream filename;
  filename << rand_number << "_" << std::time(0);
  return filename.str();
}


void check_method_not_allowed(Client &client, ServerConfig *server_conf, std::string request_path, HTTP_METHOD method) {
  LocationConfig *location =
    get_location(server_conf->getLocations(), request_path);
  if (!location) {
    throw ParsingError(BAD_REQUEST, "failed to get location from conf");
  }
  HttpRequest *req = client.get_request();
  if (!req)
    throw ParsingError(INTERNAL_SERVER_ERROR, "Failed");
  req->location = location;
  req->allowed_methods = location->allowed_methods;
  if (find_in_vec(location->allowed_methods2, method) == -1)
    throw ParsingError(METHOD_NOT_ALLOWED, "method not allowed");

}

static int	ft_isspace(char c)
{
	if (c == '\t' || c == '\n' || c == '\v'
		|| c == '\r' || c == '\f' || c == ' ')
		return (1);
	return (0);
}

int	ft_atoi(const char *str)
{
	int			sign;
	long int	nbr;
	long int	nb;

	sign = 1;
	nb = 0;
	while (ft_isspace(*str))
		str++;
	if (*str == '-' || *str == '+')
		if (*str++ == '-')
			sign = -1;
	while (*str >= '0' && *str <= '9')
	{
		nbr = nb * 10 + (*str++ - '0');
		if (nb > nbr && sign == 1)
			return (-1);
		else if (nb > nbr && sign == -1)
			return (0);
		nb = nbr;
	}
	return (nb * sign);
}


bool isValidHeaderKey(const std::string& key) {
    for (size_t i = 0; i < key.size(); i++) {
      char c = key[i];
        // Disallow control characters (ASCII < 33 or ASCII 127)
        if (c <= 32 || c == 127)
            return false;

        switch (c) {
            case '(': case ')': case '<': case '>': case '@':
            case ',': case ';': case ':': case '\\': case '"':
            case '/': case '[': case ']': case '?': case '=':
            case '{': case '}': case ' ':
            case '\t':
                return false;
        }
    }
    return !key.empty();
}
