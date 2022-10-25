#include "session.h"

#include "mmkv/protocol/router_args.h"
#include "mmkv/protocol/router_reply.h"

#include "mmkv/tracker/tracker.h"

using namespace mmkv::configd;
using namespace mmkv::server;
using namespace mmkv::protocol;

Session::Session(Tracker *t, TcpConnectionPtr const &conn)
  : conn_(conn.get())
  , codec_(RouterArgs::prototype, conn)
{
  codec_.SetMessageCallback([this, t](TcpConnectionPtr const &conn,
                                      Buffer &buffer, uint32_t, TimeStamp) {
    RouterArgs args;
    args.ParseFrom(buffer);

    RouterReply reply;
    // Get the ip address and port according to the key
    t->QueryKeyMappedAddress(args.key, reply.address, reply.port);
    codec_.Send(conn, &reply);
  });
}

