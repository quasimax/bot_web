#ifndef _HTTPS_CLIENT_HPP_
#define _HTTPS_CLIENT_HPP_
/*                                                                
                              ;;;;;;: ,++++++++++++++++++          
                             .;;;;;;  ++++++++++++++++++           
                             ;;;;;;. '+++++++++++++++++'           
                            :;;;;;;  ++++++++++++++++++            
                            ;;;;;;  ++++++++++++++++++,            
                           ;;;;;;: `++++++::::::::::::             
                          `;;;;;;  ++++++:                         
                          ;;;;;;, :++++++                          
                         ,;;;;;;  ++++++`                          
             ;;;;;;;;;;;;;;;;;;` '++++++                           
            .;;;;;;;;;;;;;;;;;;  ++++++                            
            ;;;;;;;;;;;;;;;;;;  ++++++'                            
           :;;;;;;;;;;;;;;;;., `++++++                             
           ;;;;;;;;;;;;;;;.    ++++++,                             
          ;;;;;;;;;;;;;;;;      +++++                              
                                ++++`                              
          ,,,,,,,,,,,,,,,,      +++++                              
          ,+++++++++++++++     `+++++                              
           ++++++++++++++++    '++++++                             
           `++++++++++++++++++  ++++++,                            
            ++++++++++++++++++; ,++++++                            
             ++++++++++++++++++  ++++++'                           
                         +++++++ `++++++                           
                          ++++++  +++++++                          
                          '++++++  ++++++`                         
                           ++++++, ;++++++                         
                           ,++++++  ++++++                         
                            ++++++' .++++                          
                             ++++++  +++:                          
                             +++++++  ++                           
                              ++++++` +`                           
                              ;++++++                             
                                                           
*/

#include <boost/asio.hpp>
#include <iostream>

namespace net = boost::asio;   

#include <boost/beast/ssl.hpp>
namespace ssl = net::ssl;      

#include <boost/beast/core/tcp_stream.hpp>
namespace beast = boost::beast; 

#include "root_certificates.hpp"

#include <boost/beast/http.hpp>
namespace http = beast::http;  

#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/version.hpp>

class https_client{
protected:
    net::io_context _ioc;
    net::ip::tcp::resolver _resolver;
    ssl::context _ctx;
    beast::ssl_stream<beast::tcp_stream> _stream;

    const std::string _host;
    const std::string _port;
    const std::string _api_key;

public:
    https_client( const std::string& host, const std::string& port, const std::string& key = "" ) : _resolver(_ioc), _ctx(ssl::context::tlsv12_client), _stream(_ioc, _ctx),  _host(host), _port(port), _api_key(key){
        load_root_certificates(_ctx);
        _ctx.set_verify_mode(ssl::verify_none);

        if(! SSL_set_tlsext_host_name(_stream.native_handle(), host.c_str())){
            beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};            
            throw beast::system_error{ec};
        }

        _ioc.run();    
    } 

    virtual ~https_client(){
    }

    void connect(){
        auto const results = _resolver.resolve(_host.c_str(), _port.c_str());
        beast::get_lowest_layer(_stream).connect(results);
        _stream.handshake(ssl::stream_base::client);       
    }

    void shutdown(){
        beast::error_code ec;
        _stream.shutdown(ec);
        if(ec == net::error::eof){
            ec = {};
        }
        if(ec)
            throw beast::system_error{ec}; 
    }

    std::string write(const std::string& url, const std::string& data){
        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;

        http::request<http::string_body> req{http::verb::post, url.c_str(), 11};
        req.set(http::field::host, _host.c_str());
        req.set(http::field::content_type, "application/json");
        req.set(http::field::authorization, "key=AAAA760huFA:APA91bEp8uzgoo4fwZcW2T_xsPs06ZFKViQSPzUG4zU98AMo4pmA8rvgqEcIJHfYYMF8R_q3YHmtf7n5lsLIWbJer1sc6m6Nkf0XbqibDhbMNFw_Wg6odMW12nFV-OaVIwfHKDARhxMZ");
        req.body() = data;
        req.prepare_payload();

        http::write(_stream, req);
        http::read(_stream, buffer, res);   
        
        return boost::beast::buffers_to_string(res.body().data());      
    }

    virtual std::string post(const std::string& url, const std::string& data, const std::string& content_type = "text/plain"){
        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;

        auto const results = _resolver.resolve(_host.c_str(), _port.c_str());
        beast::get_lowest_layer(_stream).connect(results);
        _stream.handshake(ssl::stream_base::client);

        http::request<http::string_body> req{http::verb::post, url.c_str(), 11};
        req.set(http::field::host, _host.c_str());
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::content_type, content_type);
        req.set(http::field::connection, "close");
        req.set(http::field::accept, "*/*");
        if(_api_key.size() != 0)
            req.set("x-mbx-apikey", _api_key);

        req.body() = data;
        req.prepare_payload();

        std::cout << "request - " << req << std::endl;

        http::write(_stream, req);
        http::read(_stream, buffer, res);

        beast::error_code ec;
        _stream.shutdown(ec);
        if(ec == net::error::eof){
            ec = {};
        }
        if(ec)
        {
            if( ec != boost::asio::ssl::error::stream_truncated)
            throw beast::system_error{ec};  
        } 

        return boost::beast::buffers_to_string(res.body().data()); 
    }

    virtual std::string get(const std::string& url){
        auto const results = _resolver.resolve(_host.c_str(), _port.c_str());
        beast::get_lowest_layer(_stream).connect(results);
        _stream.handshake(ssl::stream_base::client);

        http::request<http::string_body> req{http::verb::get, url.c_str(), 11};
        req.set(http::field::host, _host.c_str());
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        if(_api_key.size() != 0)
            req.set("x-mbx-apikey", _api_key);

        http::write(_stream, req);
        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;

        http::read(_stream, buffer, res);

        beast::error_code ec;
        _stream.shutdown(ec);
        if(ec == net::error::eof){
            ec = {};
        }
        if(ec)
        {
            if( ec != boost::asio::ssl::error::stream_truncated)
            throw beast::system_error{ec};  
        }

        return boost::beast::buffers_to_string(res.body().data());  
    }  

    virtual std::string del(const std::string& url){
        auto const results = _resolver.resolve(_host.c_str(), _port.c_str());
        beast::get_lowest_layer(_stream).connect(results);
        _stream.handshake(ssl::stream_base::client);

        http::request<http::string_body> req{http::verb::delete_, url.c_str(), 11};
        req.set(http::field::host, _host.c_str());
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        if(_api_key.size() != 0)
            req.set("x-mbx-apikey", _api_key);

        http::write(_stream, req);
        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;

        http::read(_stream, buffer, res);

        beast::error_code ec;
        _stream.shutdown(ec);
        if(ec == net::error::eof){
            ec = {};
        }
        if(ec)
        {
            if( ec != boost::asio::ssl::error::stream_truncated)
            throw beast::system_error{ec};  
        }

        return boost::beast::buffers_to_string(res.body().data());  
    }        
};

#endif
