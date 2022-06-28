#ifndef _MMKV_UTIL_TOKENIZER_H_
#define _MMKV_UTIL_TOKENIZER_H_

#include <iterator>
#include <kanon/string/string_view.h>

namespace mmkv {
namespace util {

namespace tokenizer {

class TokenizerIterator {
  using Rep = kanon::StringView;
  using Self = TokenizerIterator;
 public:
  using size_type = Rep::size_type;
  using iterator_category = std::forward_iterator_tag;
  using reference = Rep::reference;
  using pointer = Rep::pointer;
  using difference_type = Rep::difference_type;
  
  explicit TokenizerIterator(kanon::StringView text, size_type start=0, size_type end=Rep::npos)
    : text_(text)
    , start_(start)
    , end_(end)
  {
  }
  
  Self& operator++() noexcept {
    Increment();
    return *this;
  }
  
  Self operator++(int) noexcept {
    Self ret(text_, start_, end_);
    Increment();
    return ret;
  }

  Rep operator*() noexcept {
    return value();
  }
  
  Rep value() noexcept {
    return text_.substr_range(start_, end_);
  }  

  friend bool operator==(TokenizerIterator const& x, TokenizerIterator const& y) noexcept {
    return x.start_ == y.start_;
  }

  friend bool operator!=(TokenizerIterator const& x, TokenizerIterator const& y) noexcept {
    return !(x == y);
  }

 private:
  void Increment() noexcept {
    // 当end_ == npos时，上一个token是最后一个（上次Increment()：start_ != npos，end_ == npos）
    // 同时由于npos + 1 == 0，会导致循环第一个token
    // 如果start_ == 0表示还没有获取一个token
    // start_ = (start_ == 0 || end_ != Rep::npos) ? text_.find_first_not_of(" ", end_+1) : Rep::npos;
    //
    // begin()逻辑处理好了初值的设置，不需要判断start_ == 0
    start_ = (end_ != Rep::npos) ? text_.find_first_not_of(' ', end_+1) : Rep::npos;
    
    // 如果不判断start_ == Rep::npos
    // 将会导致end()++ --> start_+1 == 0 --> end != npos，构成循环，进而会导致下次Increment()接着循环
    // e.g. "A B    ", end_ != npos but start == npos
    end_ = start_ != Rep::npos ? text_.find(' ', start_+1) : Rep::npos;
  }

  kanon::StringView text_;
  size_type start_;
  size_type end_;
};

} // tokenizer

/**
 * \brief Get all the token from the text splited by space(' ')
 * 
 * The space in the left and right is ignored.
 * Besides, the count of space is ignored also.
 * That is, "A  B    C  " ==> A B C
 */
class Tokenizer {
 public:
  using Token = kanon::StringView;
  using Text = kanon::StringView;
  using iterator = tokenizer::TokenizerIterator;
  
  Tokenizer() = default; 

  explicit Tokenizer(Text text)
   : text_(text)
  {
  } 

  void SetText(Text text) {
    text_ = text;
  }

  iterator begin() noexcept {
    // 只有全是空格时，start == npos
    // 此时，begin() == end()
    // 因为该函数只会被调用一次，且不会导致循环
    // 因此不需要同Increment()一样判断
    auto start = text_.find_first_not_of(' ');
    return iterator(text_, start, text_.find(' ', start+1));
  }

  iterator end() noexcept {
    return iterator(text_, Text::npos, Text::npos);
  }

 private:
  Text text_;
};

} // util
} // mmkv

#endif // _MMKV_UTIL_TOKENIZER_H_
