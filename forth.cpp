#include <cctype>
#include <vector>
#include <functional>
#include <iostream>
#include <ranges>
#include <stack>
#include <charconv>
#include <string>
#include <print>

constexpr bool is_space(char p) noexcept {
  auto ne = [p](auto q) { return p != q; };
  return !!(" \t\n\v\r\f" | std::views::drop_while(ne));
}

std::optional<int> to_int(const std::string_view &input) {
  int out;
  const std::from_chars_result result =
      std::from_chars(input.data(), input.data() + input.size(), out);
  if (result.ec == std::errc::invalid_argument ||
      result.ec == std::errc::result_out_of_range) {
    return std::nullopt;
  }
  return out;
}

template <typename T>
class Stack {
private:
  std::vector<T> data;
public:
  T pop() {
    if (data.size() == 0) {
      throw std::runtime_error("No elements left in the stack");
    }
    T val = *(data.end() - 1);
    data.pop_back();
    return val;
  }
  void push(T val) {
    data.push_back(val);
  }
  T top() {
    if (data.size() == 0) {
      throw std::runtime_error("Empty stack!");
    }
    T val = *(data.end() - 1);
    return val;
  }
  void print() {
    for (const auto &i : data) {
      std::cout << i << ' ';
    }
    std::cout << '\n';
  }
};

using State = enum {
  STATE_INTRP,
  STATE_COMPILE
};

using WordType = enum {
  PRIMITIVE,
  COMPOSITE,
};

struct Interpreter {
  State state;
  Stack<int> data_stack;
  Interpreter(): state {STATE_INTRP} {}
  std::string_view args;
};

struct Word {
  std::string name;
  WordType word_type; 
  std::function<void (Interpreter &)> func;
  std::vector<Word> definition;

  // An empty word is quite meaningless 
  Word() = delete;
  Word(std::string _name, WordType _word_type, std::function<void (Interpreter &)> _func, std::vector<Word> _def):
    name {_name}, word_type {_word_type}, func {_func}, definition {_def} {}
};

std::pair<int, int> pop_2(Interpreter &intrp) {
  int v1 = intrp.data_stack.pop();
  int v2 = intrp.data_stack.pop();
  return std::pair {v1, v2};
}

void primitive_add(Interpreter &intrp) {
  auto vals = pop_2(intrp);
  intrp.data_stack.push(vals.first + vals.second);
}
void primitive_mul(Interpreter &intrp) {
  auto vals = pop_2(intrp);
  intrp.data_stack.push(vals.first * vals.second);
}
void primitive_sub(Interpreter &intrp) {
  auto vals = pop_2(intrp);
  intrp.data_stack.push(vals.first - vals.second);
}
void primitive_div(Interpreter &intrp) {
  auto vals = pop_2(intrp);
  intrp.data_stack.push(vals.first / vals.second);
}
void primitive_dup(Interpreter &intrp) {
  auto val = intrp.data_stack.top();
  intrp.data_stack.push(val);
}

std::unordered_map<std::string_view, Word> word_dict {
  {"+", Word("+", PRIMITIVE, primitive_add, std::vector<Word>{})},
  {"*", Word("*", PRIMITIVE, primitive_mul, std::vector<Word>{})},
  {"-", Word("-", PRIMITIVE, primitive_sub, std::vector<Word>{})},
  {"/", Word("/", PRIMITIVE, primitive_div, std::vector<Word>{})},
  {"bye", Word("bye", PRIMITIVE, [](Interpreter &) { std::exit(0); }, std::vector<Word>{})},
  {".s", Word(".s", PRIMITIVE, [](Interpreter &intrp) { intrp.data_stack.print(); }, std::vector<Word>{})},
  {"dup", Word("dup", PRIMITIVE, primitive_dup, std::vector<Word>{})},
};

auto process_command(Interpreter &intrp, auto ibegin, auto iend) {
  if (intrp.state == STATE_COMPILE) {
    std::vector<Word> def;
    std::string name = std::ranges::to<std::string>(*ibegin);
    ibegin++;
    auto i = ibegin;
    while (i != iend) {
      std::string_view s = std::string_view(*i);
      if (s == ";") {
        intrp.state = STATE_INTRP;
        i++;
        break;
      } else {
        auto res = word_dict.find(s);
        if (res != word_dict.end()) {
          Word &word = res->second;
          def.push_back(word);
        } else {
          std::cout << "Compilation failed: Word not found " << s << '\n';
          return iend;
        }
      }
      ++i;
    }
    Word word(name, COMPOSITE, nullptr, def);
    word_dict.insert({name, word});
    return i;
  } else { 
    std::string_view s = std::string_view(*ibegin); // consume a token
    ibegin++; // move to the next
    if (to_int(s)) {
      int val = to_int(s).value();
      intrp.data_stack.push(val);
    } else {
      if (s == ":") {
        intrp.state = STATE_COMPILE;
        return process_command(intrp, ibegin, iend);
      } else {
        auto res = word_dict.find(s);
        if (res != word_dict.end()) {
          Word &word = res->second;
          if (word.word_type == PRIMITIVE) {
            try {
              word.func(intrp);
            } catch (const std::runtime_error &e) {
              std::cout << e.what() << '\n';
            }
          }
        } else {
          std::cout << "Interpret failed: Word not found " << s << '\n';
        }
      } 
    }
  }
  return ibegin; 
}
#if 0
void process_command(Interpreter &INTRP, std::string_view s, auto val) {
  if (s == ".s") {
    intrp.data_stack.print(); 
  } else if (s == ":") {
    std::print("found your colon!, this is the rest: {}\n", val);
  } else {
    auto res = word_dict.find(s);
    if (res != word_dict.end()) {
      Word &word = res->second;
      if (word.word_type == PRIMITIVE) {
        try {
          word.func(intrp);
        } catch (const std::runtime_error &e) {
          std::cout << e.what() << '\n';
        }
      }
    } else {
      std::cout << "Word not found " << s << '\n';
    }
  } 
}
#endif

void repl() {
  Interpreter intrp;
  std::string line;
  while (std::getline(std::cin, line)) {
    auto new_split =
        line | std::views::drop_while(is_space) | std::views::split(' ');
    auto itr = new_split.begin();
    while (itr != new_split.end()) {
      itr = process_command(intrp, itr, new_split.end());
#if 0
      std::string_view line_v = std::string_view(*itr);
      if (to_int(line_v)) {
        int val = to_int(line_v).value();
        intrp.data_stack.push(val);
      } else {
        process_command(intrp, line_v, new_split);
      }
#endif
    }
  }
}

int main() { repl(); }
