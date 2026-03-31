/*
 * X# Standard Library - Net Module
 * ==================================
 * 12 networking functions: HTTP, TCP, UDP, URL encoding.
 */

#ifndef XS_NET_LIB_H
#define XS_NET_LIB_H

#include "../../src/runtime/runtime.h"

XsValue xs_net_httpGet(int argc, XsValue* args);
XsValue xs_net_httpPost(int argc, XsValue* args);
XsValue xs_net_tcpConnect(int argc, XsValue* args);
XsValue xs_net_tcpListen(int argc, XsValue* args);
XsValue xs_net_tcpSend(int argc, XsValue* args);
XsValue xs_net_tcpRecv(int argc, XsValue* args);
XsValue xs_net_tcpClose(int argc, XsValue* args);
XsValue xs_net_udpSend(int argc, XsValue* args);
XsValue xs_net_udpRecv(int argc, XsValue* args);
XsValue xs_net_urlEncode(int argc, XsValue* args);
XsValue xs_net_urlDecode(int argc, XsValue* args);
XsValue xs_net_tcpAccept(int argc, XsValue* args);

void xs_net_register(VM* vm);

#endif /* XS_NET_LIB_H */
