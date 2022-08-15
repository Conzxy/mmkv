#ifndef _MMKV_UTIL_TOKENIZER_H_
#define _MMKV_UTIL_TOKENIZER_H_

#include <iterator>
#include <kanon/string/string_view.h>

namespace mmkv {
namespace util {

class Tokenizer;

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
  
  explicit TokenizerIterator(Tokenizer *tokenizer, size_type start=0, size_type end=Rep::npos)
    : tokenizer_(tokenizer)
    , start_(start)
    , end_(end)
  {
  }
  
  Self& operator++() noexcept {
    Increment();
    return *this;
  }
  
  Self operator++(int) noexcept {
    Self ret(tokenizer_, start_, end_);
    Increment();
    return ret;
  }

  Rep operator*() noexcept {
    return value();
  }

  Rep value() noexcept;

  friend bool operator==(TokenizerIterator const& x, TokenizerIterator const& y) noexcept {
    return x.start_ == y.start_;
  }

  friend bool operator!=(TokenizerIterator const& x, TokenizerIterator const& y) noexcept {
    return !(x == y);
  }

 private:
  void Increment() noexcept;

  Tokenizer *tokenizer_;
  size_type start_;
  size_type end_;
};

} // tokenizer

/**
 * \brief Get all the token from the text splited by delimeter
 * 
 * The space in the left and right is ignored.
 * Besides, the count of space is ignored also.
 * That is, "A  B    C  " ==> A B C
 */
class Tokenizer {
  friend class tokenizer::TokenizerIterator;
 public:
  using Token = kanon::StringView;
  using Text = kanon::StringView;
  using iterator = tokenizer::TokenizerIterator;
  
  Tokenizer()
    : Tokenizer("")
  {
  }

  explicit Tokenizer(Text text, char delimeter = ' ')
    : text_(text)
    , deli_(delimeter)
  {
  } 

  void SetText(Text text) { text_ = text; }

  void SetDelimeter(char c) noexcept { deli_ = c; }

  iterator begin() noexcept {
    // 只有全是空格时，start == npos
    // 此时，begin() == end()
    // 因为该函数只会被调用一次，且不会导致循环
    // 因此不需要同Increment()一样判断
    auto start = text_.find_first_not_of(' ');
    return iterator(this, start, text_.find(deli_, start+1));
  }

  iterator end() noexcept {
    return iterator(this, Text::npos, Text::npos);
  }

 private:
  Text text_;
  char deli_;
};

} // util
} // mmkv

#endif // _MMKV_UTIL_TOKENIZER_H_
