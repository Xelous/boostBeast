//
// Copyright (c) 2017 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: HTTP server, fast
//
//------------------------------------------------------------------------------

#include "fields_alloc.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <list>
#include <memory>
#include <string>

#include "ServerStartDetails.h"

#include <boost/lexical_cast.hpp>

const std::string GetExtensionFromFilename(const boost::beast::string_view& p_Path)
{
	std::string l_result("");

	if (!p_Path.empty())
	{
		auto l_pos(p_Path.rfind("."));
		if (l_pos != boost::beast::string_view::npos)
		{
			l_result = p_Path.substr(l_pos).to_string();
		}
	}

	return l_result;
}

// Return a reasonable mime type based on the extension of a file.
const boost::beast::string_view GetMimeType(const boost::beast::string_view& p_Path)
{
	boost::beast::string_view l_result("application/text");

	const auto l_ext(GetExtensionFromFilename(p_Path));

	using boost::beast::iequals;
	if (iequals(l_ext, ".html"))				l_result = "text/html";
	else if (iequals(l_ext, ".htm"))			l_result = "text/html";	
	else if (iequals(l_ext, ".cpp"))			l_result = "text/html";
	else if (iequals(l_ext, ".h"))				l_result = "text/html";
	else if (iequals(l_ext, ".hpp"))			l_result = "text/html";
	else if (iequals(l_ext, ".php"))			l_result = "text/html";
	else if (iequals(l_ext, ".css"))			l_result = "text/css";
	else if (iequals(l_ext, ".txt"))			l_result = "text/plain";
	else if (iequals(l_ext, ".js"))				l_result = "application/javascript";
	else if (iequals(l_ext, ".json"))			l_result = "application/json";
	else if (iequals(l_ext, ".xml"))			l_result = "application/xml";
	else if (iequals(l_ext, ".swf"))			l_result = "application/x-shockwave-flash";
	else if (iequals(l_ext, ".flv"))			l_result = "video/x-flv";
	else if (iequals(l_ext, ".png"))			l_result = "image/png";
	else if (iequals(l_ext, ".jpe"))			l_result = "image/jpeg";
	else if (iequals(l_ext, ".jpeg"))			l_result = "image/jpeg";
	else if (iequals(l_ext, ".jpg"))			l_result = "image/jpeg";
	else if (iequals(l_ext, ".gelse if"))		l_result = "image/gelse if";
	else if (iequals(l_ext, ".bmp"))			l_result = "image/bmp";
	else if (iequals(l_ext, ".ico"))			l_result = "image/vnd.microsoft.icon";
	else if (iequals(l_ext, ".telse iff"))		l_result = "image/telse iff";
	else if (iequals(l_ext, ".telse if"))		l_result = "image/telse iff";
	else if (iequals(l_ext, ".svg"))			l_result = "image/svg+xml";
	else if (iequals(l_ext, ".svgz"))			l_result = "image/svg+xml";

	return l_result;
}


constexpr int Mbyte = 1024 * 1024;

using alloc_t = fields_alloc<char>;
using request_body_t = boost::beast::http::basic_dynamic_body<boost::beast::flat_static_buffer<Mbyte>>;

using ParserType =
	boost::optional<
		boost::beast::http::request_parser<
			request_body_t,
			alloc_t>>;

using StringResponseType = 
	boost::optional<
		boost::beast::http::response<
			boost::beast::http::string_body,
			boost::beast::http::basic_fields<alloc_t>>>;

using StringSerializerTye =
	boost::optional<
		boost::beast::http::response_serializer<
			boost::beast::http::string_body,
			boost::beast::http::basic_fields<alloc_t>>>;

using FileResponseType =
	boost::optional<
		boost::beast::http::response<
			boost::beast::http::file_body,
			boost::beast::http::basic_fields<alloc_t>>>;

using FileSerializerType =
	boost::optional<
		boost::beast::http::response_serializer<
			boost::beast::http::file_body,
			boost::beast::http::basic_fields<alloc_t>>>;

class HttpWorker;
using HttpWorkerPtr = std::shared_ptr<HttpWorker>;
class HttpWorker
{
	private:

		HttpWorker() = delete;
		HttpWorker(const HttpWorker&) = delete;
		HttpWorker& operator=(const HttpWorker &) = delete;
		HttpWorker(
			boost::asio::ip::tcp::acceptor& p_Acceptor,
			const std::string& p_DocumentRoot)
			:
			m_Acceptor(p_Acceptor),
			m_DocumentRoot(p_DocumentRoot),
			m_Socket({ m_Acceptor.get_executor().context() }),
			m_Alloc(8192),
			m_RequestDeadline(
			{
				m_Acceptor.get_executor().context(),
				(std::chrono::steady_clock::time_point::max)()
			})
		{
		}

	public:
	
		static HttpWorkerPtr Create(
			boost::asio::ip::tcp::acceptor& p_Acceptor,
			const std::string& p_DocumentRoot)
		{
			return HttpWorkerPtr(
				new HttpWorker(
					p_Acceptor,
					p_DocumentRoot));
		}
	

		void Start()
		{
			Accept();
			CheckDeadline();
		}

	private:
			

	// The acceptor used to listen for incoming connections.
	boost::asio::ip::tcp::acceptor& m_Acceptor;

	// The path to the root of the document directory.
	std::string m_DocumentRoot;

	// The socket for the currently connected client.
	boost::asio::ip::tcp::socket m_Socket;

	// The buffer for performing reads
	boost::beast::flat_static_buffer<8192> m_Buffer;

	// The allocator used for the fields in the request and reply.
	alloc_t m_Alloc;

	// The parser for reading the requests
	ParserType m_Parser;

	// The timer putting a time limit on requests.
	boost::asio::basic_waitable_timer<std::chrono::steady_clock> m_RequestDeadline;	

	// The string-based response message.
	StringResponseType m_StringResponse;

	// The string-based response serializer.
	StringSerializerTye m_StringSerializer;

	// The file-based response message.
	FileResponseType m_FileResponse;

	// The file-based response serializer.
	FileSerializerType m_FileSerializer;

	void Accept()
	{
		// Clean up any previous connection.
		boost::beast::error_code ec;
		m_Socket.close(ec);
		m_Buffer.consume(m_Buffer.size());

		m_Acceptor.async_accept(
			m_Socket,
			[this](boost::beast::error_code ec)
			{
				if (ec)
				{
					Accept();
				}
				else
				{
					// Request must be fully processed within 60 seconds.
					m_RequestDeadline.expires_after(
						std::chrono::seconds(60));

					ReadRequest();
				}
			});
	}

	void ReadRequest()
	{
		// On each read the parser needs to be destroyed and
		// recreated. We store it in a boost::optional to
		// achieve that.
		//
		// Arguments passed to the parser constructor are
		// forwarded to the message object. A single argument
		// is forwarded to the body constructor.
		//
		// We construct the dynamic body with a 1MB limit
		// to prevent vulnerability to buffer attacks.
		//
		m_Parser.emplace(
			std::piecewise_construct,
			std::make_tuple(),
			std::make_tuple(m_Alloc));

		boost::beast::http::async_read(
			m_Socket,
			m_Buffer,
			*m_Parser,
			[this](boost::beast::error_code ec, std::size_t)
			{
				if (ec)
				{
					Accept();
				}
				else
				{
					ProcessRequest(m_Parser->get());
				}
			});
	}

	void ProcessRequest(const boost::beast::http::request<request_body_t,boost::beast::http::basic_fields<alloc_t>>& p_Request)
	{
		switch (p_Request.method())
		{
			case boost::beast::http::verb::get:
				SendFile(p_Request.target());
				break;

			default:
				// We return responses indicating an error if
				// we do not recognize the request method.
				SendBadResponse(
					boost::beast::http::status::bad_request,
					"Invalid request-method '" + p_Request.method_string().to_string() + "'\r\n");
				break;
		}
	}

	void SendBadResponse(
		const boost::beast::http::status& p_Status,
		std::string const& p_Error)
	{
		m_StringResponse.emplace(
			std::piecewise_construct,
			std::make_tuple(),
			std::make_tuple(m_Alloc));

		m_StringResponse->result(p_Status);
		m_StringResponse->keep_alive(false);
		m_StringResponse->set(boost::beast::http::field::server, "Beast");
		m_StringResponse->set(boost::beast::http::field::content_type, "text/plain");
		m_StringResponse->body() = p_Error;
		m_StringResponse->prepare_payload();

		m_StringSerializer.emplace(*m_StringResponse);

		boost::beast::http::async_write(
			m_Socket,
			*m_StringSerializer,
			[this](boost::beast::error_code ec, std::size_t)
			{
				m_Socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
				m_StringSerializer.reset();
				m_StringResponse.reset();
				Accept();
			});
	}

	void SendBadFileResponse()
	{
		SendBadResponse(
			boost::beast::http::status::not_found,
			"File not found\r\n");
	}	

	void SendFile(const boost::beast::string_view& p_Target)
	{
		// Request path must be absolute and not contain "..".
		if (p_Target.empty() ||
			p_Target[0] != '/' ||
			p_Target.find("..") != std::string::npos)
		{
			SendBadFileResponse();
		}
		else
		{
			std::string l_FullPath (m_DocumentRoot);
			l_FullPath.append(
				p_Target.data(),
				p_Target.size());

			boost::beast::error_code l_ec;
			boost::beast::http::file_body::value_type l_File;			
			l_File.open(
				l_FullPath.c_str(),
				boost::beast::file_mode::read,
				l_ec);
			if (l_ec)
			{
				SendBadFileResponse();
			}
			else
			{
				m_FileResponse.emplace(
					std::piecewise_construct,
					std::make_tuple(),
					std::make_tuple(m_Alloc));

				m_FileResponse->result(boost::beast::http::status::ok);
				m_FileResponse->keep_alive(false);
				m_FileResponse->set(boost::beast::http::field::server, "Beast");
				m_FileResponse->set(boost::beast::http::field::content_type, GetMimeType(p_Target.to_string()));
				m_FileResponse->body() = std::move(l_File);
				m_FileResponse->prepare_payload();

				m_FileSerializer.emplace(*m_FileResponse);

				boost::beast::http::async_write(
					m_Socket,
					*m_FileSerializer,
					[this](boost::beast::error_code ec, std::size_t)
					{
						m_Socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
						m_FileSerializer.reset();
						m_FileResponse.reset();
						Accept();
					});
			}
		}
	}

	void CheckDeadline()
	{
		// The deadline may have moved, so check it has really passed.
		if (m_RequestDeadline.expiry() <= std::chrono::steady_clock::now())
		{
			// Close socket to cancel any outstanding operation.
			boost::beast::error_code l_ec;
			m_Socket.close();

			// Sleep indefinitely until we're given a new deadline.
			m_RequestDeadline.expires_at(
				std::chrono::steady_clock::time_point::max());
		}

		m_RequestDeadline.async_wait(
			[this](boost::beast::error_code)
			{
				CheckDeadline();
			});
	}
};

void PrintHelp()
{
	std::cout << "Usage: http_server_fast <address> <port> <doc_root> <num_workers> {spin|block}\n";
	std::cout << "  For IPv4, try:\n";
	std::cout << "    http_server_fast 0.0.0.0 80 . 100 block\n";
	std::cout << "  For IPv6, try:\n";
	std::cout << "    http_server_fast 0::0 80 . 100 block\n";
}

int main(const int p_argc, const char* p_argv[])
{
	try
	{
		// Check command line arguments.
		if (p_argc != 6)
		{
			PrintHelp();
		}
		else
		{
			try
			{
				ServerStartDetails l_SSD(p_argv);				

				boost::asio::io_context l_IOContext { 1 };

				boost::asio::ip::tcp::acceptor l_Acceptor
				{
					l_IOContext,
					{
						l_SSD.Address,
						l_SSD.Port 
					} 
				};

				std::list<HttpWorkerPtr> l_Workers;

				for (int i(0); i < l_SSD.NumberOfWorkers; ++i)
				{
					try
					{
						auto l_Candidate(
							HttpWorker::Create(
								l_Acceptor,
								l_SSD.DocumentRoot));
						if (l_Candidate)
						{
							try
							{
								l_Candidate->Start();
								l_Workers.push_back(l_Candidate);
							}
							catch (const std::exception&)
							{
								std::cerr << "Error Starting worker" << std::endl;
							}
						}
					}
					catch (const std::bad_alloc&)
					{
						std::cerr << "Out of memory for workers" << std::endl;
						break;
					}
				}

				if (l_SSD.Spin)
				{
					do
					{
						l_IOContext.poll();
					}
					while (true);
				}
				else
				{
					l_IOContext.run();
				}
			}
			catch (const boost::bad_lexical_cast& l_blc)
			{
				std::cerr << "Error parsing server start port or number of workers [" << l_blc.what() << "]" << std::endl;
			}
		}
	}
	catch (const std::exception& l_ex)
	{
		std::cerr << "Error: " << l_ex.what() << std::endl;
	}
}