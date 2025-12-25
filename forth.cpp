#include <cctype>
#include <vector>
#include <functional>
#include <iostream>
#include <ranges>
#include <stack>
#include <charconv>
#include <string>

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
      throw "No elements left in the stack";
    }
    T val = *(data.end() - 1);
    data.pop_back();
    return val;
  }
  void push(T val) {
    data.push_back(val);
  }
  void print() {
    for (const auto &i : data) {
      std::cout << i << ' ';
    }
    std::cout << '\n';
  }
};

struct Interpreter {
  Stack<int> data_stack;
};

std::unordered_map<std::string_view, std::function<int (int, int)>> word_dict {
  {"+", [](int a, int b) -> int { return a + b; }},
  {"-", [](int a, int b) -> int { return a - b; }},
  {"*", [](int a, int b) -> int { return a * b; }},
  {"/", [](int a, int b) -> int { return a / b; }},
};

void process_command(Interpreter &intrp, std::string_view s) {
  if (s == ".s") {
    intrp.data_stack.print(); 
  } else if (s == "+" || s == "-" || s == "*" || s == "/") {
    int v1 = intrp.data_stack.pop();
    int v2 = intrp.data_stack.pop();
    int res = word_dict[s](v1, v2);
    intrp.data_stack.push(res);
  }
}

void repl() {
  Interpreter intrp;
  std::string line;
  while (std::getline(std::cin, line)) {
    auto new_split =
        line | std::views::drop_while(is_space) | std::views::split(' ');

    // std::cout << std::string_view(*new_split.begin()) << '\n';
    for (const auto &i : new_split) {
      std::string_view line_v = std::string_view(i);
      if (to_int(line_v)) {
        int val = to_int(line_v).value();
        intrp.data_stack.push(val);
      } else {
        process_command(intrp, line_v);
      }
    }
  }
}

int main() { repl(); }
