#include <iostream>
#include "sms_interface.hpp"

#include <boost/bind.hpp>

using namespace std;

void kvit_handler(const kvit_t& kvit)
{
    std::cout << "SMS Kvit:" << '\n';
    std::cout << "result: " << kvit.result << '\n';
    std::cout << "code: " << kvit.ecode << '\n';
    std::cout << "message: " << kvit.emessage << '\n';
    std::cout << "sms_id: \n";
    for(const auto& sms_id : kvit.sms_id)
        std::cout << sms_id.first << " - " << sms_id.second << '\n';
}

int main()
{
    SMSInterface smi("api.smstraffic.ru",       // основной сервер
                     "api2.smstraffic.ru",      // резервный сервер
                     "multi.php",               // скрипт отправки - https://api.smstraffic.ru/multi.php
                     "rfi:cryptobank",          // логин
                     "VcP8ufZN5Beboqv1",        // пароль
                     boost::bind(&kvit_handler, ::_1)); // callback

    sms_t sms{{"79523585785", "79062601962"}, "Hello 11 mar"};
    smi.send(sms);

    return 0;
}
