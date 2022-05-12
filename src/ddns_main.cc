#ifndef __ddns_main_cc__
#define __ddns_main_cc__

#include "ddns.h"


int main(int argc, char* argv[])
{
    std::string ip("");
    std::string port = "8080";

    ACE_LOG_MSG->open (argv[0], ACE_LOG_MSG->STDERR|ACE_LOG_MSG->SYSLOG);
#if 0
   /* The last argument tells from where to start in argv - offset of argv array */
   ACE_Get_Opt opts (argc, argv, ACE_TEXT ("i:p:"), 1);

   opts.long_option(ACE_TEXT("ip"), 'i', ACE_Get_Opt::ARG_REQUIRED);
   opts.long_option(ACE_TEXT("port"), 'p', ACE_Get_Opt::ARG_REQUIRED);
   opts.long_option(ACE_TEXT("help"), 'h', ACE_Get_Opt::ARG_REQUIRED);

   int c = 0;

   while ((c = opts ()) != EOF) {
     switch (c) {
       case 'i':
       {
         ip.assign(std::string(opts.opt_arg()));
         ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [master:%t] %M %N:%l IP %s\n"), ip.c_str()));
       }
         break;

       case 'p':
       {
          port.assign(opts.opt_arg());
         ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [master:%t] %M %N:%l PORT %s\n"), port.c_str()));
       }
         break;
       case 'h':
       default:
          ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT("%D [Master:%t] %M %N:%l usage: %s\n"
                            " [-i --ip]\n"
                            " [-p --port]\n"
                            " [-h --help]\n"
                            ),
                            argv [0]),
                            -1);
     }
   }

    WebServer webServer(ip, port);

    webServer.start();
#endif

  TLSClient ddns("ipvx.herokuapp.com", 443);

  if(ddns.start()) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [master:%t] %M %N:%l Client is connected successfully\n")));
    if(ddns.buildAndSendRequestToGetDynamicIP() < 0) {
      /* Error case */
      ACE_ERROR((LM_ERROR, ACE_TEXT("%D [worker:%t] %M %N:%l Request is sent successfully\n")));
    } else {
      ACE_Time_Value to(1,0);
      while(ddns.isRunning()) {
        ACE_Reactor::instance()->handle_events(to);
      }
    }
  }
}












#endif /* __ddns_main_cc__*/