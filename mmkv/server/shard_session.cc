#include "shard_session.h"

#include "mmkv/protocol/shard_args.h"
#include "mmkv/protocol/shard_reply.h"

#include "mmkv/util/shard_util.h"

using namespace mmkv::server;
using namespace mmkv::protocol;
using namespace std::placeholders;

ShardSession::ShardSession(TcpConnectionPtr const &conn) 
  : conn_(conn.get())
  , codec_(ShardArgs::prototype, conn)
{
  codec_.SetMessageCallback(std::bind(&ShardSession::OnMessage, this, _1, _2, _3, _4));
}

void ShardSession::OnMessage(TcpConnectionPtr const &conn,
  Buffer &buffer,
  uint32_t, TimeStamp recv_tm) {
  ShardArgs args;
  args.ParseFrom(buffer);
  args.DebugPrint();

  ShardReply reply;
  switch (args.operation) {
  case SO_GET_SHARD: {
#if SHARD_TEST
  reply.code = SC_OK;
  LOG_INFO << "Get shard operation";
  codec_.Send(conn_, &reply);
#else

#endif
  }
  break;

  case SO_REM_SHARD: {
#if SHARD_TEST
  // reply.code = SC_OK;
  LOG_INFO << "Remove shard operation";
#else

#endif
  }
  break;
  default: {
    LOG_ERROR << "Unknown ShardOperation";
  }
  }
}