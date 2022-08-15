#include "tokenizer.h"

using namespace mmkv::util;
using namespace mmkv::util::tokenizer;

auto TokenizerIterator::value() noexcept -> Rep {
  return tokenizer_->text_.substr_range(start_, end_);
}

auto TokenizerIterator::Increment() noexcept -> void {
  // 当end_ == npos时，上一个token是最后一个（上次Increment()：start_ != npos，end_ == npos）
  // 同时由于npos + 1 == 0，会导致循环第一个token
  // 如果start_ == 0表示还没有获取一个token
  // start_ = (start_ == 0 || end_ != Rep::npos) ? text_.find_first_not_of(" ", end_+1) : Rep::npos;
  //
  // begin()逻辑处理好了初值的设置，不需要判断start_ == 0
  start_ = (end_ != Rep::npos) ? tokenizer_->text_.find_first_not_of(tokenizer_->deli_, end_+1) : Rep::npos;
  
  // 如果不判断start_ == Rep::npos
  // 将会导致end()++ --> start_+1 == 0 --> end != npos，构成循环，进而会导致下次Increment()接着循环
  // e.g. "A B    ", end_ != npos but start == npos
  end_ = start_ != Rep::npos ? tokenizer_->text_.find(tokenizer_->deli_, start_+1) : Rep::npos;
}