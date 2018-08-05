#ifndef SERVER_START_DETAILS
#define SERVER_START_DETAILS

#include <string>
#include <boost/asio.hpp>

struct ServerStartDetails
{
	const boost::asio::ip::address		Address;
	const unsigned short				Port;
	const std::string					DocumentRoot;
	const int							NumberOfWorkers;
	const bool							Spin;

	ServerStartDetails(const char* p_Arguments[]);

};


#endif