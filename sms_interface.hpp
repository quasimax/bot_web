#ifndef SMSMODULE_H
#define SMSMODULE_H

#include <iostream>
#include <vector>
#include <map>

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include "http_client.hpp"
#include <boost/optional/optional.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
namespace pt = boost::property_tree;

typedef struct
{
    std::vector<std::string> phones;
    std::string text;

} sms_t;

typedef struct
{
    enum types {Queued, Sended, Delivered} type;
    bool result;
    int ecode;    
    std::string emessage;
    std::map<std::string, uint64_t> sms_id;

} kvit_t;

typedef boost::function<void (const kvit_t&)> callback_kvit;

class SMSInterface
{
public:
    SMSInterface(const std::string& _main_server,
                 const std::string& _rsrv_server,
                 const std::string& _sender_url,
                 const std::string& _login,
                 const std::string& _password,
                 callback_kvit _kvit_handler) : m_sender_url(std::string("/") + _sender_url),
                                                m_login(_login),
                                                m_password(_password),
                                                m_kvit_handler(_kvit_handler)
    {
        m_clients.emplace_back(std::make_shared<https_client>(_rsrv_server, "443"));
        m_clients.emplace_back(std::make_shared<https_client>(_main_server, "443"));
    }

    virtual ~SMSInterface()
    {
    }

    kvit_t send(const sms_t& sms)
    {
        std::string sep("&");
        std::string data;
        data += "login=" + m_login;
        data += sep + "password=" + m_password;
        data += sep + "want_sms_ids=1";
        data += sep + "phones=";

        for(const auto& phone : sms.phones)
            data += phone + ',';

        data.erase(--data.end());
        data += sep + "message=" + sms.text;

        std::string answer;
        kvit_t kvit;
        kvit.type = kvit_t::types::Queued;
        bool sended = false;

        for(auto client : m_clients)
        {
            try
            {
                answer = client->post(m_sender_url, data, "application/x-www-form-urlencoded");
                sended = true;
                break;
            }
            catch(std::exception& e)
            {
                std::cout << "Send error:\n" << e.what();                
            }
        }

        if(!sended)
        {
            kvit.ecode = -1;
            return kvit;
        }

        pt::ptree xtree;        

        try
        {
            std::stringstream ss;
            ss << answer;
            pt::read_xml(ss, xtree);

            for(auto&& rtag : xtree)
            {
                if(rtag.first != "reply")
                    continue;

                kvit.result = ("OK" == rtag.second.get<std::string>("result"));
                kvit.ecode = rtag.second.get<int>("code");
                kvit.emessage = rtag.second.get<std::string>("description");                

                boost::optional<pt::ptree&> msg_info = rtag.second.get_child_optional("message_infos");
                if(!msg_info)
                    continue;

                for(const auto& minfo : msg_info.get())
                {
                    if(minfo.first == "message_info")
                    {
                        kvit.sms_id.emplace(minfo.second.get<std::string>("phone"),
                                            minfo.second.get<uint64_t>("sms_id"));
                    }
                }
            }
        }
        catch(std::exception& e)
        {
            std::cout << "Answer parse error:\n" << e.what();
            throw;
        }

        m_kvit_handler(kvit);
        return kvit;
    }

private:
    std::string m_sender_url;
    std::string m_login;
    std::string m_password;

    std::vector<std::shared_ptr<https_client>> m_clients;
    callback_kvit  m_kvit_handler;
};

#endif // SMSMODULE_H
