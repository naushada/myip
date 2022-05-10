#ifndef __ipvx_cc__
#define __ipvx_cc__

#include "ipvx.h"


WebClientEntry::WebClientEntry(WebServer *webServer)
{
    m_parent = webServer;
}


WebClientEntry::~WebClientEntry()
{

}

WebServer& WebClientEntry::get_parent() const 
{
    return(*m_parent);
}


bool WebClientEntry::processRequest(ACE_HANDLE handle)
{
    ACE_Message_Block *mb = nullptr;
    ACE_NEW_RETURN(mb, ACE_Message_Block(2048), 0);

    mb->reset();
    size_t ret = -1;

    ret = ACE_OS::recv(handle, mb->wr_ptr(), mb->size(),0);

    if(ret < 0) {
        /* Receive failed */
        ACE_ERROR((LM_ERROR, ACE_TEXT("%D [worker:%t] %M %N:%l recv on handle %u is failed\n"), handle));
        return(false);
    } else {

        /* Receive is success , process it. */
        mb->wr_ptr(ret);
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [Master:%t] %M %N:%l The Request is \n%s\n"), mb->rd_ptr()));
        std::string req(mb->rd_ptr(), mb->length());
        /* Parsing HTTP Request Now */
        Http http(req);

        if(http.header().length()) {
            /* HTTP Header */
            std::string CT = http.get_element("Content-Type");
            std::string CL = http.get_element("Content-Length");
            std::string DYNIP = http.get_element("X-Forwarded-For");
            if(DYNIP.length()) {
                /* got the Dynamic IP by Which HTTP Request is initiated */
                buildAndSendResponse(handle, http);
                mb->release();
                return(false);
            } else {
                /* Build 200 OK */
                buildAndSendResponse(handle, http);
                mb->release();
                return(false);
            }
        }

    }
    return(true);
}


std::string WebClientEntry::build200OK(Http http)
{
    std::stringstream rsp("");

    rsp << "HTTP/1.1 200 OK\r\n"
        << "Host: " << http.get_element("Host") << "\r\n"
        << "Connection: close\r\n"
        << "Content-Type: " << "text/html" << "\r\n"
        << "Content-Length: 0" << "\r\n"
        << "\r\n";
        

    return(rsp.str());
}


std::string WebClientEntry::buildIPResponse(Http http)
{
    std::stringstream rsp("");

    rsp << "HTTP/1.1 200 OK\r\n"
        << "Host: " << http.get_element("Host") << "\r\n"
        << "Connection: close\r\n"
        << "Content-Type: " << "text/html" << "\r\n"
        << "Content-Length: " << std::to_string(http.get_element("X-Forwarded-For").length()) << "\r\n"
        << "\r\n"
        << http.get_element("X-Forwarded-For");

    return(rsp.str());
}


bool WebClientEntry::buildAndSendResponse(ACE_HANDLE handle, Http http)
{
    std::string rsp("");

    std::string IP = http.get_element("X-Forwarded-For");
    if(IP.length()) {
        rsp = buildIPResponse(http);
    } else {
        rsp = build200OK(http);
    }
    
    size_t len = sendResponse(handle, rsp);

    if(len < 0) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("%D [worker:%t] %M %N:%l sendResponse on handle %u is failed\n"), handle));
        return(false);
    }

    return(true);
}

ACE_INT32 WebClientEntry::sendResponse(ACE_HANDLE handle, std::string rsp)
{
    size_t offset = 0;
    size_t ret = -1;
    size_t toBeSent = rsp.length();
    char *data = rsp.data();

    do {
        ret = ACE_OS::send(handle, (const char *)(data + offset), (toBeSent - offset), 0);

        if(ret > 0) {
            offset += ret;
        } else {
           ACE_ERROR((LM_ERROR, ACE_TEXT("%D [worker:%t] %M %N:%l send on handle %u is failed\n"), handle));
           break;
        }

    }while(toBeSent != offset);

    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [Master:%t] %M %N:%l Response sent is \n%s\n"), rsp.c_str()));
    return(toBeSent);
}

WebServer::WebServer(std::string ipStr, std::string listenPort)
{
    m_isRunning = false;
    std::string addr;
    addr.clear();

    if(ipStr.length()) {

        addr = ipStr;
        addr += ":";
        addr += listenPort;
        m_listen.set(std::stoi(listenPort), ipStr.c_str());
    } else {

        addr = "127.0.0.1";
        addr += ":";
        addr += listenPort;
        m_listen.set(std::stoi(listenPort));

    }

    int reuse_addr = 1;
    if(m_server.open(m_listen, reuse_addr)) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [Master:%t] %M %N:%l Starting of WebServer failed - opening of port %d hostname %s\n"), 
                m_listen.get_port_number(), m_listen.get_host_name()));
    }

    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [Master:%t] %M %N:%l Starting of WebServer for port %d hostname %s addr %s\n"), 
                m_listen.get_port_number(), m_listen.get_host_name(), addr.c_str()));
}


WebServer::~WebServer()
{
    for(auto it = std::begin(m_webClientEntry); it != std::end(m_webClientEntry);) {
        auto ent = it->second;
        it = m_webClientEntry.erase(it);
        delete ent;
    }
}


ACE_INT32 WebServer::handle_input(ACE_HANDLE handle)
{
    int ret_status = 0;
    ACE_SOCK_Stream peerStream;
    ACE_INET_Addr peerAddr;
    WebClientEntry *ent = nullptr;

    if(handle == m_server.get_handle()) {

        /* New Connection is initiated */
        ret_status = m_server.accept(peerStream, &peerAddr);
        if(!ret_status) {

            ACE_NEW_RETURN(ent, WebClientEntry(this), -1);
            m_webClientEntry[peerStream.get_handle()] = ent;
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [Master:%t] %M %N:%l New connection is created and handle is %d peer ip address %u (%s) peer port %d\n"), 
                                    peerStream.get_handle(),
                                    peerAddr.get_ip_address(), peerAddr.get_host_addr(), peerAddr.get_port_number()));

            /* Feed this new handle to event Handler for read/write operation. */
            ACE_Reactor::instance()->register_handler(peerStream.get_handle(), this, ACE_Event_Handler::READ_MASK);

        }

    } else {

        /* Already Connected peer/WebClient */
        auto it = m_webClientEntry.find(handle);
        if(it != std::end(m_webClientEntry)) {

            ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [Master:%t] %M %N:%l procssing on handle %d\n"), handle));
            ent = it->second;

            if(!ent->processRequest(handle)) {
                /* Either zero byte is received or some error has happened */
                close(handle);
                m_webClientEntry.erase(it);
                delete ent;
                ACE_Reactor::instance()->remove_handler(handle, ACE_Event_Handler::READ_MASK);
            }
        } else {
            /* Error case */
            ACE_ERROR((LM_ERROR, ACE_TEXT("%D [worker:%t] %M %N:%l handle_input is failed\n"), handle));
        }

    }
    
    /* Continue for either new connection or read on existing handle */
    return(0);
}


ACE_INT32 WebServer::handle_timeout(const ACE_Time_Value &tv, const void *act)
{
    /* to be implemented for Idle connection of client. */
    return(0);
}


ACE_INT32 WebServer::handle_signal(int signum, siginfo_t *s, ucontext_t *u)
{
    for(auto it = std::begin(m_webClientEntry); it != std::end(m_webClientEntry);) {
        auto ent = it->second;
        close(it->first);
        it = m_webClientEntry.erase(it);
        delete ent;
    }

    m_webClientEntry.clear();

    //m_isRunning = false;
    return(0);
}


ACE_INT32 WebServer::handle_close (ACE_HANDLE handle, ACE_Reactor_Mask mask)
{
    close(handle);
    return(0);
}


ACE_HANDLE WebServer::get_handle() const
{
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%D [Master:%t] %M %N:%l get_handle is invoked\n")));
    return(m_server.get_handle());
}


void WebServer::start()
{
    ACE_Reactor::instance()->register_handler(this, ACE_Event_Handler::ACCEPT_MASK |
                                                    ACE_Event_Handler::TIMER_MASK |
                                                    ACE_Event_Handler::SIGNAL_MASK); 
    /* subscribe for signal */
    ACE_Sig_Set ss;
    ss.empty_set();
    ss.sig_add(SIGINT);
    ss.sig_add(SIGTERM);
    ACE_Reactor::instance()->register_handler(&ss, this); 
    m_isRunning = true;
    ACE_Time_Value to(1,0);

    while(m_isRunning) {
        ACE_Reactor::instance()->handle_events(to);
    }

    stop();
}

void WebServer::stop()
{
    ACE_Reactor::instance()->remove_handler(m_server.get_handle(), ACE_Event_Handler::ACCEPT_MASK |
                                                                   ACE_Event_Handler::TIMER_MASK |
                                                                   ACE_Event_Handler::SIGNAL_MASK);

}


#endif /*__ipvx_cc__*/
