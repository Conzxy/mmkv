// SPDX-LICENSE-IDENTIFIER: Apache-2.0
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

  explicit TokenizerIterator(Tokenizer *tokenizer, size_type start = 0,
                             size_type end = Rep::npos)
    : tokenizer_(tokenizer)
    , start_(start)
    , end_(end)
  {
  }

  Self &operator++() noexcept
  {
    Increment();
    return *this;
  }

  Self operator++(int) noexcept
  {
    Self ret(tokenizer_, start_, end_);
    Increment();
    return ret;
  }

  Rep operator*() noexcept
  {
    return value();
  }

  KANON_INLINE Rep value() noexcept;

  friend bool operator==(TokenizerIterator const &x,
                         TokenizerIterator const &y) noexcept
  {
    return x.start_ == y.start_;
  }

  friend bool operator!=(TokenizerIterator const &x,
                         TokenizerIterator const &y) noexcept
  {
    return !(x == y);
  }

 private:
  KANON_INLINE void Increment() noexcept;

  Tokenizer *tokenizer_;
  size_type start_;
  size_type end_;
};

} // namespace tokenizer

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

  void SetText(Text text)
  {
    text_ = text;
  }

  void SetDelimeter(char c) noexcept
  {
    deli_ = c;
  }

  iterator begin() noexcept
  {
    // 只有全是空格时，start == npos
    // 此时，begin() == end()
    // 因为该函数只会被调用一次，且不会导致循环
    // 因此不需要同Increment()一样判断
    auto start = text_.find_first_not_of(' ');
    return iterator(this, start, text_.find(deli_, start + 1));
  }

  iterator end() noexcept
  {
    return iterator(this, Text::npos, Text::npos);
  }

 private:
  Text text_;
  char deli_;
};

namespace tokenizer {

auto TokenizerIterator::value() noexcept -> Rep
{
  return tokenizer_->text_.substr_range(start_, end_);
}

auto TokenizerIterator::Increment() noexcept -> void
{
  // 当end_ == npos时，上一个token是最后一个（上次Increment()：start_ !=
  // npos，end_ == npos） 同时由于npos + 1 == 0，会导致循环第一个token
  // 如果start_ == 0表示还没有获取一个token
  // start_ = (start_ == 0 || end_ != Rep::npos) ? text_.find_first_not_of(" ",
  // end_+1) : Rep::npos;
  //
  // begin()逻辑处理好了初值的设置，不需要判断start_ == 0
  start_ =
      (end_ != Rep::npos)
          ? tokenizer_->text_.find_first_not_of(tokenizer_->deli_, end_ + 1)
          : Rep::npos;

  // 如果不判断start_ == Rep::npos
  // 将会导致end()++ --> start_+1 == 0 --> end !=
  // npos，构成循环，进而会导致下次Increment()接着循环 e.g. "A B    ", end_ !=
  // npos but start == npos
  end_ = start_ != Rep::npos
             ? tokenizer_->text_.find(tokenizer_->deli_, start_ + 1)
             : Rep::npos;
}

} // namespace tokenizer
} // namespace util
} // namespace mmkv

#endif // _MMKV_UTIL_TOKENIZER_H_
