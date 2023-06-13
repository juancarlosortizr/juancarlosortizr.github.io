#include <napi.h>
#include <string>
#include <exception>
#include "game.hpp"

Game g;

Napi::String PlayMove(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string command = (std::string) info[0].ToString(); // or Utf8Value();           
  std::string board;
  try {
    board += g.playMove(command);
  } catch (const std::exception& e) {
    board += e.what();
  }                                                                                                            
  return Napi::String::New(env, board);
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set(
    Napi::String::New(env, "playMove"),
    Napi::Function::New(env, PlayMove)
  );
  return exports;
}

NODE_API_MODULE(chess, Init)
