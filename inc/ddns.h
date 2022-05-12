#ifndef __ddns_h__
#define __ddns_h__

/**
 * @file ddns.h
 * @author Mohammed Naushad Ahmed (naushad.dln@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2022-05-12
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <vector>
#include <memory>
#include <unordered_map>
#include <algorithm>
#include <iostream>
#include <fstream>

#include "ace/Reactor.h"
#include "ace/Basic_Types.h"
#include "ace/Event_Handler.h"
#include "ace/Task.h"
#include "ace/INET_Addr.h"
#include "ace/SOCK_Stream.h"
#include "ace/SOCK_Acceptor.h"
#include "ace/Task_T.h"
#include "ace/Timer_Queue_T.h"
#include "ace/Reactor.h"
#include "ace/OS_Memory.h"
#include "ace/Thread_Manager.h"
#include "ace/Get_Opt.h"
#include "ace/Signal.h"
/** For SSL */
#include "ace/SSL/SSL_SOCK.h"
#include "ace/SSL/SSL_SOCK_Stream.h"
#include "ace/SSL/SSL_SOCK_Connector.h"
#include "ace/SSL/SSL_Context.h"

#include "http_parser.h"

class WebServer ;

class WebClientEntry {
    public:
        WebClientEntry(WebServer *parent);
        ~WebClientEntry();
        bool processRequest(ACE_HANDLE handle);
        bool buildAndSendResponse(ACE_HANDLE handle, Http http);
        WebServer& get_parent() const;
        ACE_INT32 sendResponse(ACE_HANDLE handle, std::string rsp);
        std::string build200OK(Http http);
        std::string buildIPResponse(Http http);

    private:
        WebServer *m_parent;
};


class WebServer : public ACE_Event_Handler {
    public:
        ACE_INT32 handle_timeout(const ACE_Time_Value &tv, const void *act=0) override;
        ACE_INT32 handle_input(ACE_HANDLE handle) override;
        ACE_INT32 handle_signal(int signum, siginfo_t *s = 0, ucontext_t *u = 0) override;
        ACE_INT32 handle_close (ACE_HANDLE = ACE_INVALID_HANDLE, ACE_Reactor_Mask = 0) override;
        ACE_HANDLE get_handle() const override;

        WebServer(std::string _ip, std::string _port);
        virtual ~WebServer();
        void start();
        void stop();

    private:
        bool m_isRunning;
        ACE_Message_Block m_mb;
        ACE_SOCK_Stream m_stream;
        ACE_INET_Addr m_listen;
        ACE_SOCK_Acceptor m_server;
        std::unordered_map<ACE_INT32, WebClientEntry*> m_webClientEntry; 
};

class TLSClient : public ACE_Event_Handler {
    public:
        ACE_INT32 handle_timeout(const ACE_Time_Value &tv, const void *act=0) override;
        ACE_INT32 handle_input(ACE_HANDLE handle) override;
        ACE_INT32 handle_signal(int signum, siginfo_t *s = 0, ucontext_t *u = 0) override;
        ACE_INT32 handle_close (ACE_HANDLE = ACE_INVALID_HANDLE, ACE_Reactor_Mask = 0) override;
        ACE_HANDLE get_handle() const override;

        bool start();
        bool stop();
        /**
         * @brief This method prepares HTTP GET Request to get the Dynamic IP assigned by ISP.
         * 
         * @return * void 
         */
        std::string buildRequestToGetDYNIP();

        /**
         * @brief This method sends the HTTP request on connected SSL client.
         * 
         * @param req HTTP Request
         * @return ACE_INT32 
         */
        ACE_INT32 sendRequest(std::string req);

        /**
         * @brief 
         * 
         * @return ACE_INT32 
         */
        ACE_INT32 buildAndSendRequestToGetDynamicIP();

        /**
         * @brief 
         * 
         * @param mb 
         * @return ACE_INT32 
         */
        ACE_INT32 processResponse(ACE_Message_Block& mb);

        /**
         * @brief 
         * 
         * @param mb 
         * @return true 
         * @return false 
         */
        bool preProcessResponse(ACE_Message_Block& mb);

        bool isRunning() const {
            return(m_isRunning);
        }
        
        TLSClient(std::string ip, ACE_UINT16 port) : m_peerAddr(port, ip.c_str()), 
                                                     m_conn(m_stream, m_peerAddr), 
                                                     m_mb(nullptr),
                                                     m_isRunning(false) {

        }

        virtual ~TLSClient() {
            if(m_mb) {
                m_mb->release();
            }
        }

    private:
        ACE_INET_Addr m_peerAddr;
        ACE_SSL_SOCK_Connector m_conn;
        ACE_SSL_SOCK_Stream m_stream;
        ACE_Message_Block *m_mb;
        bool m_isRunning;
        
};

#endif /*__ddns_h__*/
