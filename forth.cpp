#include <cctype>
#include <errno.h>
#include <fstream>
#include <vector>
#include <functional>
#include <iostream>
#include <ranges>
#include <stack>
#include <charconv>
#include <string>
#include <print>
#include <stdarg.h>

#define LOG

constexpr bool is_space(char p) noexcept {
  auto ne = [p](auto q) { return p != q; };
  return !!(" \t\n\v\r\f" | std::views::drop_while(ne));
}

template <typename... Args> 
void log(std::format_string<Args...> fmt, Args... args) {
#ifdef LOG
  std::print("LOG: ");
  std::print(fmt, args...);
#endif
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
  size_t size() {
    return data.size();
  }
  T at(int i) {
    // Bounds check handled by vector::at() so I omit it
    return data.at(i);
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

  void print() {
    std::print("{} {} ", name, word_type == PRIMITIVE ? "PRIMITIVE" : "COMPOSITE");
    for (auto itr : definition) {
      std::print("{} ", itr.name);
    }
    std::print("\n");
  }

  auto begin() {
    return name.begin();
  }
  auto end() {
    return name.end();
  }
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
  intrp.data_stack.push(vals.second / vals.first);
}
void primitive_dup(Interpreter &intrp) {
  auto val = intrp.data_stack.top();
  intrp.data_stack.push(val);
}
void primtive_word_dict(Interpreter &intrp);

void primitive_drop(Interpreter &intrp) {
  (void)intrp.data_stack.pop();
}
void primitive_over(Interpreter &intrp) {
  auto &stack = intrp.data_stack;
  if (stack.size() < 2) {
    throw std::runtime_error("Not enough elements in data stack to perform over");
  }
  auto under = stack.at(stack.size() - 2);
  stack.push(under);
}

std::unordered_map<std::string, Word> word_dict {
  {"+", Word("+", PRIMITIVE, primitive_add, std::vector<Word>{})},
  {"*", Word("*", PRIMITIVE, primitive_mul, std::vector<Word>{})},
  {"-", Word("-", PRIMITIVE, primitive_sub, std::vector<Word>{})},
  {"/", Word("/", PRIMITIVE, primitive_div, std::vector<Word>{})},
  {"bye", Word("bye", PRIMITIVE, [](Interpreter &) { std::exit(0); }, std::vector<Word>{})},
  {".s", Word(".s", PRIMITIVE, [](Interpreter &intrp) { intrp.data_stack.print(); }, std::vector<Word>{})},
  {"dup", Word("dup", PRIMITIVE, primitive_dup, std::vector<Word>{})},
  {"drop", Word("drop", PRIMITIVE, primitive_drop, std::vector<Word>{})},
  {"over", Word("over", PRIMITIVE, primitive_over, std::vector<Word>{})},
  {".w", Word(".w", PRIMITIVE, primtive_word_dict, std::vector<Word>{})},
};

void primtive_word_dict(Interpreter &intrp) {
  for (auto itr : word_dict) {
    itr.second.print();
  }
}

void execute(Interpreter &intrp, Word &word) {
  if (word.word_type == PRIMITIVE) {
    word.func(intrp);
  } else if (word.word_type == COMPOSITE) {
    for (Word &sub_word : word.definition) {
      execute(intrp, sub_word);
    }
  }
}

Word &find(std::string &key) {
  auto res = word_dict.find(key);
  if (res == word_dict.end()) {
    throw std::runtime_error("Could not find word");
  }
  return res->second;
}

auto compile(Interpreter &intrp, auto ibegin, auto iend) {
  std::vector<Word> def;
  auto sub = *ibegin;
  std::string name(sub.begin(), sub.end());
  ibegin++;
  auto i = ibegin;
  while (i != iend) {
    std::string_view s = std::string_view(*i);
    if (s == ";") {
      intrp.state = STATE_INTRP;
      i++;
      break;
    } else {
      std::string key(s);
      auto res = word_dict.find(key);
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
}

auto comment(Interpreter &intrp, auto ibegin, auto iend) {
  while (ibegin != iend) {
    if (std::string_view(*ibegin) == ")") {
      ibegin++;
      return ibegin;
    }
    ibegin++;
  }
  return ibegin;
}

auto interpret(Interpreter &intrp, auto ibegin, auto iend) {
  std::string_view s = std::string_view(*ibegin); // consume a token
  ibegin++;
  if (to_int(s)) {
    int val = to_int(s).value();
    intrp.data_stack.push(val);
  } else if (s == ":") {
    intrp.state = STATE_COMPILE;
    return compile(intrp, ibegin, iend);
  } else if (s == "(") {
    return comment(intrp, ibegin, iend);
  } else {
    std::string key(s);
    try {
      Word &word = find(key);
      execute(intrp, word);
    } catch (const std::runtime_error &e) {
      std::print("{}", e.what());
    }
  } 
  return ibegin; 
}

void repl(std::istream &in) {
  Interpreter intrp;
  std::string line;
  while (std::getline(in, line)) {
    auto new_split =
        line | std::views::drop_while(is_space) | std::views::split(' ');
    auto itr = new_split.begin();
    while (itr != new_split.end()) {
      itr = interpret(intrp, itr, new_split.end());
    }
  }
}

int main(int argc, char *argv[]) { 
  if (argc > 1) {
    std::ifstream ifp(argv[1], std::ios::in);
    if (errno != 0) {
      std::print("ERROR: {}\n", strerror(errno));
    }
    repl(ifp);
  } else {
    repl(std::cin);
  }
}
