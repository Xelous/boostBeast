#include "ServerStartDetails.h"

#include <boost/lexical_cast.hpp>

ServerStartDetails::ServerStartDetails(char* p_Arguments[])
	:
	Address(boost::asio::ip::make_address(p_Arguments[1])),
	Port(boost::lexical_cast<unsigned short>(p_Arguments[2])),
	DocumentRoot(p_Arguments[3]),
	NumberOfWorkers(boost::lexical_cast<int>(p_Arguments[4])),
	Spin(std::strcmp(p_Arguments[5], "spin") == 0)
{
}